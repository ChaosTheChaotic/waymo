cmake_minimum_required(VERSION 3.30)
project(waymo_napi)

if(NOT CMAKE_JS_INC)
  message(FATAL_ERROR "This project must be built using cmake-js")
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_library(${PROJECT_NAME} SHARED wayscript.cc)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_JS_INC}
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  "${CMAKE_CURRENT_BINARY_DIR}/../../generated/proto/include" 
  "${CMAKE_CURRENT_SOURCE_DIR}/../../node_modules/node-addon-api"
  ${WAYLAND_CLIENT_INCLUDE_DIRS}
  ${XKBCOMMON_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
  "${CMAKE_CURRENT_SOURCE_DIR}/../../lib/static/libwaymo.a"
  ${WAYLAND_CLIENT_LIBRARIES}
  ${XKBCOMMON_LIBRARIES}
  ${CMAKE_JS_LIB}
)

set_target_properties(${PROJECT_NAME} PROPERTIES 
  PREFIX "" 
  SUFFIX ".node"
)
