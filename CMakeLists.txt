cmake_minimum_required(VERSION 3.25)

project("waymo" VERSION 1.0.0 LANGUAGES C CXX)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  message(FATAL_ERROR "Building on Windows is not supported.")
endif()

if(CMAKE_C_STANDARD_LATEST GREATER_EQUAL 17)
  set(CMAKE_C_STANDARD 17)
elseif(CMAKE_C_STANDARD_LATEST GREATER_EQUAL 11)
  set(CMAKE_C_STANDARD 11)
else()
  message(FATAL_ERROR "Compiler does not support C17 or C11 (for atomics). Build aborted.")
endif()
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

option(USE_CCACHE "Try to use ccache for faster builds" ON)
option(USE_LTO "Use linktime optimizations if available" ON)
option(BUILD_SHARED "Build a shared library for the system" ON)
option(BUILD_STATIC "Build a static library for the system" ON)
option(GENERATE_PROTOCOLS "Generate Wayland protocol files if needed" ON)
option(MARCH_NATIVE "Build library optimized for native arch" OFF)
option(DO_INSTALL "Try to install library after it is build" OFF)
option(PKG_CONFIG "Install pkg-config files" ON)
option(BUILD_PYTHON "Build the python library" OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(USE_CCACHE AND CCACHE_PROGRAM)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "ccache is found and is being used")
else()
  message(WARNING "ccache is not found, if building multiple times it is recomended to increase build speeds")
endif()

if(NOT USE_CCACHE)
  message(STATUS "ccache disabled")
endif()

enable_testing()

add_library(waymo_settings INTERFACE)

target_compile_options(waymo_settings INTERFACE -Wall -Wpedantic -Wno-strict-prototypes)

include(CheckCCompilerFlag)
check_c_compiler_flag("-O3" HAS_O3)
check_c_compiler_flag("-march=native" HAS_MARCH_NATIVE)
if(HAS_O3)
  target_compile_options(waymo_settings INTERFACE -O3)
endif()

if (HAS_MARCH_NATIVE AND MARCH_NATIVE)
  target_compile_options(waymo_settings INTERFACE -march=native)
endif()

if(USE_LTO)
  message(STATUS "LTO option is enabled")

  include(CheckIPOSupported)

  check_ipo_supported(RESULT ltos OUTPUT ltoerr)
  if(ltos)
    set_target_properties(waymo_settings PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "LTO (IPO) is supported and enabled")
  else()
    message(STATUS "LTO is enabled but not supported. Keeping disabled")
  endif()
else()
  message(STATUS "LTO is disabled in the options")
endif()

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "src/*.c")

set(GEN_PROTO_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/proto")
set(GEN_PROTO_DIR_H "${GEN_PROTO_DIR}/include")
set(PROTO_XML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")

set(PROTO_K_X "${PROTO_XML_DIR}/wvk.xml")
set(PROTO_K_C "${GEN_PROTO_DIR}/wvk.c")
set(PROTO_K_H "${GEN_PROTO_DIR_H}/wvk.h")

set(PROTO_P_X "${PROTO_XML_DIR}/wvp.xml")
set(PROTO_P_C "${GEN_PROTO_DIR}/wvp.c")
set(PROTO_P_H "${GEN_PROTO_DIR_H}/wvp.h")

file(MAKE_DIRECTORY "${GEN_PROTO_DIR}")
file(MAKE_DIRECTORY "${GEN_PROTO_DIR_H}")

if(GENERATE_PROTOCOLS)
  find_program(WAYLAND_SCANNER NAMES wayland-scanner REQUIRED)

  set(PROTO_FILES_TO_GENERATE)
  foreach(PROTO_FILE IN ITEMS "${PROTO_K_C}" "${PROTO_K_H}" "${PROTO_P_C}" "${PROTO_P_H}")
    if(NOT EXISTS "${PROTO_FILE}")
      list(APPEND PROTO_FILES_TO_GENERATE "${PROTO_FILE}")
    endif()
  endforeach()

  if(PROTO_FILES_TO_GENERATE)
    add_custom_command(
      OUTPUT "${PROTO_K_C}" "${PROTO_K_H}"
      COMMAND ${WAYLAND_SCANNER} private-code "${PROTO_K_X}" "${PROTO_K_C}"
      COMMAND ${WAYLAND_SCANNER} client-header "${PROTO_K_X}" "${PROTO_K_H}"
      DEPENDS "${PROTO_K_X}"
      COMMENT "Generating keyboard protocol files"
      VERBATIM
    )

    add_custom_command(
      OUTPUT "${PROTO_P_C}" "${PROTO_P_H}"
      COMMAND ${WAYLAND_SCANNER} private-code "${PROTO_P_X}" "${PROTO_P_C}"
      COMMAND ${WAYLAND_SCANNER} client-header "${PROTO_P_X}" "${PROTO_P_H}"
      DEPENDS "${PROTO_P_X}"
      COMMENT "Generating pointer protocol files"
      VERBATIM
    )
  else()
    message(STATUS "Protocol files already exist. Skipping generation")
  endif()
else()
  message(STATUS "Protocol generation disabled. Using existing files")
endif()

add_library(waymo_deps INTERFACE)
target_link_libraries(waymo_deps INTERFACE "${WAYLAND_CLIENT_LIBRARIES}" "${XKBCOMMON_LIBRARIES}")
target_include_directories(waymo_deps INTERFACE
  "${WAYLAND_CLIENT_INCLUDE_DIRS}"
  "${XKBCOMMON_INCLUDE_DIRS}"
)

add_library(waymo_obj OBJECT ${SOURCE_FILES} "${PROTO_K_C}" "${PROTO_P_C}")
target_compile_options(waymo_obj PUBLIC "${WAYLAND_CLIENT_CFLAGS_OTHER}" "${XKBCOMMON_CFLAGS_OTHER}")
set_property(TARGET waymo_obj PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(
  waymo_obj
  PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${GEN_PROTO_DIR_H}"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/private"
  PRIVATE
  "${WAYLAND_CLIENT_INCLUDE_DIRS}"
  "${XKBCOMMON_INCLUDE_DIRS}"
)
target_link_libraries(waymo_obj PRIVATE waymo_settings PUBLIC waymo_deps)

if(BUILD_TESTING)
  include(CTest)
  add_subdirectory(tests)
endif()

add_executable(waymo_cli "${CMAKE_CURRENT_SOURCE_DIR}/bindings/bash/waymo.c")
target_link_libraries(waymo_cli PRIVATE waymo_obj waymo_deps waymo_settings)
set_target_properties(waymo_cli PROPERTIES OUTPUT_NAME "waymo" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(BUILD_SHARED)
  message(STATUS "Build shared enabled, building the shared library")
  add_library(waymo_shared SHARED)
  target_link_libraries(waymo_shared PUBLIC waymo_obj waymo_deps PRIVATE waymo_settings)
  set_target_properties(waymo_shared PROPERTIES OUTPUT_NAME "waymo" LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/shared" RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/shared")
else()
  message(STATUS "Build shared disabled, skipping the shared library build")
endif()

if(BUILD_STATIC)
  message(STATUS "Build static enabled, building the static library")
  add_library(waymo_static STATIC)
  target_link_libraries(waymo_static PUBLIC waymo_obj waymo_deps PRIVATE waymo_settings)
  set_target_properties(waymo_static PROPERTIES OUTPUT_NAME "waymo" ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/static")
else()
  message(STATUS "Build static disabled, skipping the static library build")
endif()

if(DO_INSTALL)
  if(BUILD_SHARED OR BUILD_STATIC)
    # Install headers
    install(DIRECTORY include/ DESTINATION include
      FILES_MATCHING PATTERN "*.h"
    )

    # Install libraries
    if(BUILD_SHARED)
      install(TARGETS waymo_shared
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
      )
    endif()
    
    if(BUILD_STATIC)
      install(TARGETS waymo_static
        ARCHIVE DESTINATION lib
      )
    endif()

    if(PKG_CONFIG)
      include(GNUInstallDirs)
      configure_file("${CMAKE_CURRENT_SOURCE_DIR}/waymo.pc.in" "${CMAKE_CURRENT_BINARY_DIR}/waymo.pc" @ONLY)
      install(FILES "${CMAKE_CURRENT_BINARY_DIR}/waymo.pc" DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig")
    endif()
  endif()
  install(TARGETS waymo_cli RUNTIME DESTINATION bin)
endif()

if(BUILD_PYTHON)
  find_package(Python COMPONENTS Interpreter Development REQUIRED)
  find_package(nanobind CONFIG REQUIRED)
  nanobind_add_module(
      waymo_python
      bindings/python/bindings.cc
  )
  
  set_target_properties(waymo_python PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
  
  target_link_libraries(waymo_python PRIVATE 
      waymo_static 
      ${WAYLAND_CLIENT_LIBRARIES} 
      ${XKBCOMMON_LIBRARIES}
  )

  install(
        TARGETS waymo_python
        LIBRARY DESTINATION .
        RUNTIME DESTINATION .
  )
  
  nanobind_add_stub(
      waymo_python_stub
      MODULE waymo_python
      OUTPUT waymo_python.pyi
      DEPENDS waymo_python
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/waymo_python.pyi DESTINATION .)
endif()
