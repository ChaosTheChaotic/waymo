#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>
#include <stdlib.h>
#include "events/queue.h"

static void test_queue_overflow(void **state) {
    command_queue *q = create_queue(2);
    command *c1 = malloc(sizeof(command));
    command *c2 = malloc(sizeof(command));
    command *c3 = malloc(sizeof(command));

    assert_true(add_queue(q, c1));
    assert_true(add_queue(q, c2));
    // Should fail as capacity is 2
    assert_false(add_queue(q, c3));

    free(remove_queue(q));
    // Now should succeed
    assert_true(add_queue(q, c3));

    destroy_queue(q); // destroy_queue handles freeing remaining cmds
}

static void test_queue_shutdown_behavior(void **state) {
    command_queue *q = create_queue(5);
    command *c1 = malloc(sizeof(command));
    
    // Manual shutdown simulation
    atomic_store(&q->shutdown, true);
    assert_false(add_queue(q, c1));
    
    free(c1);
    destroy_queue(q);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_queue_overflow),
        cmocka_unit_test(test_queue_shutdown_behavior),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
