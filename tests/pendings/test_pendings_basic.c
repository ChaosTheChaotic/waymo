#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include "events/pendings.h"

// Mocking the loop/context for basic list logic
static void test_schedule_order(void **state) {
    waymo_event_loop loop = {0};
    pthread_mutex_init(&loop.pending_mutex, NULL);

    struct pending_action *a1 = calloc(1, sizeof(struct pending_action));
    struct pending_action *a2 = calloc(1, sizeof(struct pending_action));
    
    a1->expiry_ms = 2000;
    a2->expiry_ms = 1000;

    // Schedule later one first
    schedule_action(&loop, a1);
    schedule_action(&loop, a2);

    // Head should be the one expiring sooner (a2)
    assert_ptr_equal(loop.pending_head, a2);
    assert_ptr_equal(loop.pending_head->next, a1);

    clear_pending_actions(&loop);
    pthread_mutex_destroy(&loop.pending_mutex);
}

static void test_handle_timer_expiry_empty(void **state) {
    waymo_event_loop loop = {0};
    waymoctx ctx = {0}; // Mock this properly
    pthread_mutex_init(&loop.pending_mutex, NULL);
    
    // Should not crash on empty list
    handle_timer_expiry(&loop, &ctx);
    assert_null(loop.pending_head);
    
    pthread_mutex_destroy(&loop.pending_mutex);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_schedule_order),
	cmocka_unit_test(test_handle_timer_expiry_empty)
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
