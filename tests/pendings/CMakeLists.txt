# Test for basic pendings operations
add_waymo_test(test_pendings_basic test_pendings_basic.c)

# Test for edge cases or stress testing
add_waymo_test(test_pendings_stress test_pendings_stress.c)

# Test for issues with thread safety
add_waymo_test(test_pendings_concurrent test_pendings_concurrent.c)
