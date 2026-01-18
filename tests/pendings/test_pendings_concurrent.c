#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "events/pendings.h"
#include "utils.h"

#define NUM_THREADS 4
#define ACTIONS_PER_THREAD 100

typedef struct {
    waymo_event_loop *loop;
    int thread_id;
} thread_data;

static void *schedule_thread(void *arg) {
    thread_data *data = (thread_data *)arg;
    for (int i = 0; i < ACTIONS_PER_THREAD; i++) {
        struct pending_action *a = calloc(1, sizeof(struct pending_action));
        a->expiry_ms = timestamp() + (rand() % 1000);
        schedule_action(data->loop, a);
        usleep(100); // Add some jitter
    }
    return NULL;
}

static void test_concurrent_scheduling(void **state) {
    waymo_event_loop loop = {0};
    pthread_mutex_init(&loop.pending_mutex, NULL);
    
    pthread_t threads[NUM_THREADS];
    thread_data thread_data_arr[NUM_THREADS];
    
    // Create scheduling threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data_arr[i] = (thread_data){&loop, i};
        pthread_create(&threads[i], NULL, schedule_thread, &thread_data_arr[i]);
    }
    
    // Wait for all threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Verify list integrity
    pthread_mutex_lock(&loop.pending_mutex);
    struct pending_action *curr = loop.pending_head;
    int count = 0;
    uint64_t last_expiry = 0;
    
    while (curr) {
        assert_true(curr->expiry_ms >= last_expiry); // Should be sorted
        last_expiry = curr->expiry_ms;
        curr = curr->next;
        count++;
    }
    
    assert_int_equal(count, NUM_THREADS * ACTIONS_PER_THREAD);
    pthread_mutex_unlock(&loop.pending_mutex);
    
    clear_pending_actions(&loop);
    pthread_mutex_destroy(&loop.pending_mutex);
}

// Concurrent clearing
static void *clear_func(void *arg) {
    waymo_event_loop *loop = (waymo_event_loop *)arg;
    for (int i = 0; i < 10; i++) {
        clear_pending_actions(loop);
        usleep(50000); // 50ms
    }
    return NULL;
}

static void test_concurrent_clear_and_schedule(void **state) {
    waymo_event_loop loop = {0};
    pthread_mutex_init(&loop.pending_mutex, NULL);
    
    pthread_t schedule_threads[2];
    pthread_t clear_thread;
    
    // Start scheduling threads
    thread_data data = {&loop, 0};
    for (int i = 0; i < 2; i++) {
        pthread_create(&schedule_threads[i], NULL, schedule_thread, &data);
    }
    
    pthread_create(&clear_thread, NULL, clear_func, &loop);
    
    // Wait
    for (int i = 0; i < 2; i++) {
        pthread_join(schedule_threads[i], NULL);
    }
    pthread_join(clear_thread, NULL);
    
    // Should not crash
    pthread_mutex_destroy(&loop.pending_mutex);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_concurrent_scheduling),
        cmocka_unit_test(test_concurrent_clear_and_schedule),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
