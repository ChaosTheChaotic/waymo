#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include "events/pendings.h"

static void test_clear_empty_list(void **state) {
    waymo_event_loop loop = {0};
    pthread_mutex_init(&loop.pending_mutex, NULL);
    loop.pending_head = NULL;

    // Should not crash
    clear_pending_actions(&loop);
    assert_null(loop.pending_head);
    pthread_mutex_destroy(&loop.pending_mutex);
}

static void test_rapid_scheduling(void **state) {
    waymo_event_loop loop = {0};
    pthread_mutex_init(&loop.pending_mutex, NULL);

    // Insert 1000 actions with descending expiry to force list traversal
    for (int i = 1000; i > 0; i--) {
        struct pending_action *a = calloc(1, sizeof(struct pending_action));
        a->expiry_ms = i;
        schedule_action(&loop, a);
    }

    assert_int_equal(loop.pending_head->expiry_ms, 1);
    
    clear_pending_actions(&loop);
    pthread_mutex_destroy(&loop.pending_mutex);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_clear_empty_list),
        cmocka_unit_test(test_rapid_scheduling),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
