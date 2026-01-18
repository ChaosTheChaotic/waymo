# Test for basic queue operations
add_waymo_test(test_queue_basic test_queue_basic.c)

# Test for edge cases or stress testing
add_waymo_test(test_queue_stress test_queue_stress.c)

# Test for thread safety issues
add_waymo_test(test_queue_concurrent test_queue_concurrent.c)
