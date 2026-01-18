#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <poll.h>
#include "events/queue.h"
#include "unistd.h"

static void test_queue_create_destroy(void **state) {
    command_queue *q = create_queue(10);
    assert_non_null(q);
    assert_int_equal(q->max_capacity, 10);
    assert_int_equal(q->num_commands, 0);
    assert_true(q->fd >= 0);
    destroy_queue(q);
}

static void test_queue_add_remove(void **state) {
    command_queue *q = create_queue(5);
    command *cmd = malloc(sizeof(command));
    
    assert_true(add_queue(q, cmd));
    assert_int_equal(q->num_commands, 1);
    
    command *removed = remove_queue(q);
    assert_ptr_equal(cmd, removed);
    assert_int_equal(q->num_commands, 0);
    
    free(removed);
    destroy_queue(q);
}

static void test_queue_eventfd_signaling(void **state) {
    command_queue *q = create_queue(5);
    command *cmd = malloc(sizeof(command));
    
    // 1. Verify FD is initially quiet
    struct pollfd pfd = {
        .fd = q->fd,
        .events = POLLIN,
    };
    
    int ret = poll(&pfd, 1, 0); // Non-blocking check
    assert_int_equal(ret, 0);   // Should timeout immediately (no data)

    // 2. Add item to queue
    assert_true(add_queue(q, cmd));

    // 3. Verify FD is now readable
    ret = poll(&pfd, 1, 100);   // 100ms timeout
    assert_int_equal(ret, 1);   // Should return 1 event
    assert_int_equal(pfd.revents & POLLIN, POLLIN);

    // 4. Verify we can read the signal (uint64_t for eventfd)
    uint64_t count = 0;
    ssize_t s = read(q->fd, &count, sizeof(count));
    assert_int_equal(s, sizeof(uint64_t));
    assert_int_equal(count, 1); // 1 addition = value 1

    // Cleanup
    free(remove_queue(q));
    destroy_queue(q);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_queue_create_destroy),
        cmocka_unit_test(test_queue_add_remove),
	cmocka_unit_test(test_queue_eventfd_signaling),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
