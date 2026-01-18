#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdatomic.h>
#include "events/queue.h"

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 2
#define CMDS_PER_PRODUCER 500

typedef struct {
    command_queue *q;
    int *consumed_count;
    pthread_mutex_t *count_mutex;
} thread_data;

// Returns false if shutdown detected, true if added
static bool spin_add(command_queue *q, command *c) {
    while (!add_queue(q, c)) {
        if (atomic_load(&q->shutdown)) return false;
        usleep(10); 
    }
    return true;
}

static void* producer_func(void *arg) {
    command_queue *q = (command_queue *)arg;
    for (int i = 0; i < CMDS_PER_PRODUCER; i++) {
        command *c = malloc(sizeof(command));
        if (!spin_add(q, c)) {
            free(c);
            break;
        }
    }
    return NULL;
}

static void* consumer_func(void *arg) {
    thread_data *data = (thread_data *)arg;
    int local_count = 0;
    
    while (true) {
        command *c = remove_queue(data->q);
        if (c) {
            free(c);
            local_count++;
        } else {
            if (atomic_load(&data->q->shutdown)) break;
            usleep(50); 
        }
    }

    pthread_mutex_lock(data->count_mutex);
    *(data->consumed_count) += local_count;
    pthread_mutex_unlock(data->count_mutex);
    return NULL;
}

static void test_concurrent_produce_consume(void **state) {
    // Use a TINY capacity to force maximum contention on locks
    command_queue *q = create_queue(2); 
    
    int total_consumed = 0;
    pthread_mutex_t count_mutex;
    pthread_mutex_init(&count_mutex, NULL);

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    thread_data c_data = {q, &total_consumed, &count_mutex};

    for (int i = 0; i < NUM_CONSUMERS; i++) 
        pthread_create(&consumers[i], NULL, consumer_func, &c_data);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_create(&producers[i], NULL, producer_func, q);

    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(producers[i], NULL);

    // Allow consumers to drain
    usleep(100000); 
    atomic_store(&q->shutdown, true);

    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(consumers[i], NULL);

    // Drain leftovers
    command *last;
    while ((last = remove_queue(q))) {
        free(last);
        total_consumed++;
    }

    assert_int_equal(total_consumed, NUM_PRODUCERS * CMDS_PER_PRODUCER);
    
    destroy_queue(q);
    pthread_mutex_destroy(&count_mutex);
}

// This ensures that even with thread scheduling jitter, 
// a single stream remains strictly ordered.

#define SEQUENCE_LENGTH 1000

// We wrap the command to include a sequence number for verification
typedef struct {
    command cmd;
    int sequence_id;
} sequenced_command;

static void* ordered_producer(void *arg) {
    command_queue *q = (command_queue *)arg;
    for (int i = 0; i < SEQUENCE_LENGTH; i++) {
        sequenced_command *sc = malloc(sizeof(sequenced_command));
        sc->sequence_id = i;
        if (!spin_add(q, (command*)sc)) {
            free(sc);
            break;
        }
    }
    return NULL;
}

static void* ordered_consumer(void *arg) {
    command_queue *q = (command_queue *)arg;
    int expected_seq = 0;
    
    while (expected_seq < SEQUENCE_LENGTH) {
        sequenced_command *sc = (sequenced_command*)remove_queue(q);
        if (sc) {
            // CRITICAL CHECK: Did we skip a number or go backwards?
            if (sc->sequence_id != expected_seq) {
                // Return -1 to signal failure to main thread
                free(sc);
                return (void*)-1;
            }
            free(sc);
            expected_seq++;
        } else {
            usleep(10);
        }
    }
    return NULL;
}

static void test_fifo_integrity(void **state) {
    command_queue *q = create_queue(10);
    
    pthread_t p, c;
    pthread_create(&p, NULL, ordered_producer, q);
    pthread_create(&c, NULL, ordered_consumer, q);
    
    pthread_join(p, NULL);
    
    void *result;
    pthread_join(c, &result);
    
    // If result is non-null (specifically -1), the order check failed
    assert_null(result);
    
    destroy_queue(q);
}

// Simulate producers flooding the queue while the main thread shuts it down.
static void* spam_producer(void *arg) {
    command_queue *q = (command_queue *)arg;
    while (!atomic_load(&q->shutdown)) {
        command *c = malloc(sizeof(command));
        // We don't care if it succeeds or fails, just checking for crashes
        if (!add_queue(q, c)) {
            free(c);
            usleep(1); // Yield slightly to allow shutdown to progress
        }
    }
    return NULL;
}

static void test_shutdown_race(void **state) {
    command_queue *q = create_queue(5);
    pthread_t producers[4];
    
    // Start spamming
    for(int i=0; i<4; i++) {
        pthread_create(&producers[i], NULL, spam_producer, q);
    }
    
    usleep(10000); // Let them build up speed
    
    // Trigger shutdown concurrently
    atomic_store(&q->shutdown, true);
    
    for(int i=0; i<4; i++) {
        pthread_join(producers[i], NULL);
    }
    
    // Cleanup whatever made it in
    command *c;
    while ((c = remove_queue(q))) free(c);
    
    destroy_queue(q);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_concurrent_produce_consume),
        cmocka_unit_test(test_fifo_integrity),
        cmocka_unit_test(test_shutdown_race),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
