cmake_minimum_required(VERSION 3.30)
project(waymo_napi)

if(NOT CMAKE_JS_INC)
  message(FATAL_ERROR "This project must be built using cmake-js")
endif()

execute_process(
  COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

add_library(${PROJECT_NAME} SHARED wayscript.cc)

target_compile_definitions(${PROJECT_NAME} PRIVATE NAPI_VERSION=3)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_JS_INC}
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  "${CMAKE_CURRENT_BINARY_DIR}/../../generated/proto/include" 
  ${NODE_ADDON_API_DIR}
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
