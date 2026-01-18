#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "events/queue.h"

#define NUM_PRODUCERS 4
#define NUM_CONSUMERS 2
#define CMDS_PER_PRODUCER 500

typedef struct {
    command_queue *q;
    int *consumed_count;
    pthread_mutex_t *count_mutex;
} thread_data;

static void* producer_func(void *arg) {
    command_queue *q = (command_queue *)arg;
    for (int i = 0; i < CMDS_PER_PRODUCER; i++) {
        command *c = malloc(sizeof(command));
        // If queue is full, busy wait (mimics high load)
        while (!add_queue(q, c)) {
            usleep(10);
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
            // If queue is empty, check if we should exit
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
    unsigned int total_cmds = NUM_PRODUCERS * CMDS_PER_PRODUCER;
    command_queue *q = create_queue(100); // Smaller capacity to force contention
    
    int total_consumed = 0;
    pthread_mutex_t count_mutex;
    pthread_mutex_init(&count_mutex, NULL);

    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    thread_data c_data = {q, &total_consumed, &count_mutex};

    // Start consumers
    for (int i = 0; i < NUM_CONSUMERS; i++) 
        pthread_create(&consumers[i], NULL, consumer_func, &c_data);

    // Start producers
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_create(&producers[i], NULL, producer_func, q);

    // Wait for producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++)
        pthread_join(producers[i], NULL);

    // Give consumers a moment to drain the queue then shut down
    usleep(100000);
    atomic_store(&q->shutdown, true);

    for (int i = 0; i < NUM_CONSUMERS; i++)
        pthread_join(consumers[i], NULL);

    // Drain any final leftovers
    command *last;
    while ((last = remove_queue(q))) {
        free(last);
        total_consumed++;
    }

    assert_int_equal(total_consumed, total_cmds);
    
    destroy_queue(q);
    pthread_mutex_destroy(&count_mutex);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_concurrent_produce_consume),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
