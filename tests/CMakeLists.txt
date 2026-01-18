find_package(PkgConfig REQUIRED)
pkg_check_modules(CMOCKA REQUIRED cmocka)

function(add_waymo_test TEST_NAME)
    add_executable(${TEST_NAME} ${ARGN})
    
    # Link against the object library to access internal functions
    # Link against deps for Wayland/XKB and settings for flags
    target_link_libraries(${TEST_NAME} PRIVATE 
        waymo_obj 
        waymo_deps 
        waymo_settings 
        ${CMOCKA_LIBRARIES}
    )

    target_include_directories(${TEST_NAME} PRIVATE 
        ${CMOCKA_INCLUDE_DIRS}
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )

    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_subdirectory(queue)
add_subdirectory(pendings)
