# SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(naarad)
# Find the Threads package
find_package(Threads REQUIRED)

# Find the ncurses package
find_package(Curses REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Tell cmake where to find BpfObject module
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/tools/cmake)

# Build vendored libbpf
include(ExternalProject)
ExternalProject_Add(libbpf
  PREFIX libbpf
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libbpf/src
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make
    BUILD_STATIC_ONLY=1
    OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
    DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
    INCLUDEDIR=
    LIBDIR=
    UAPIDIR=
    install install_uapi_headers
  BUILD_IN_SOURCE TRUE
  INSTALL_COMMAND ""
  STEP_TARGETS build
)

ExternalProject_Add(bpftool
  PREFIX bpftool
  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/bpftool/src
  CONFIGURE_COMMAND ""
  BUILD_COMMAND make bootstrap
    OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
  BUILD_IN_SOURCE TRUE
  INSTALL_COMMAND ""
  STEP_TARGETS build
)

find_program(CARGO_EXISTS cargo)
if(CARGO_EXISTS)
  ExternalProject_Add(blazesym
    PREFIX blazesym
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/blazesym
    CONFIGURE_COMMAND ""
    BUILD_COMMAND cargo build --package=blazesym-c --release
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
    STEP_TARGETS build
  )
endif()

# Set BpfObject input parameters -- note this is usually not necessary unless
# you're in a highly vendored environment (like libbpf-bootstrap)
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
  set(ARCH "x86")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
  set(ARCH "arm")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
  set(ARCH "arm64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64le")
  set(ARCH "powerpc")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
  set(ARCH "mips")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
  set(ARCH "riscv")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch64")
  set(ARCH "loongarch")
endif()

set(BPFOBJECT_BPFTOOL_EXE ${CMAKE_CURRENT_BINARY_DIR}/bpftool/bootstrap/bpftool)
set(BPFOBJECT_VMLINUX_H ${CMAKE_CURRENT_SOURCE_DIR}/vmlinux/${ARCH}/vmlinux.h)
set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.a)
find_package(BpfObject REQUIRED)

# Change this line to look in the src directory using a relative path
file(GLOB apps src/*.bpf.c)

if(NOT CARGO_EXISTS)
  list(REMOVE_ITEM apps ${CMAKE_CURRENT_SOURCE_DIR}/src/profile.bpf.c)
endif()

# foreach(app ${apps})
#   get_filename_component(app_stem ${app} NAME_WE)
#   # Build object skeleton and depend skeleton on libbpf build
#   bpf_object(${app_stem} src/${app_stem}.bpf.c)
#   add_dependencies(${app_stem}_skel libbpf-build bpftool-build)
#   add_executable(${app_stem} src/${app_stem}.c)
#   target_link_libraries(${app_stem} ${app_stem}_skel)
#   if(${app_stem} STREQUAL profile)
#     target_include_directories(${app_stem} PRIVATE
#       ${CMAKE_CURRENT_SOURCE_DIR}/third_party/blazesym/capi/include)
#     target_link_libraries(${app_stem}
#       ${CMAKE_CURRENT_SOURCE_DIR}/third_party/blazesym/target/release/libblazesym_c.a -lpthread -lrt -ldl)
#   endif()
# endforeach()
# Gather all source files from include directory
file(GLOB_RECURSE INCLUDE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c"
)

# Build BPF object skeleton and depend skeleton on libbpf build
bpf_object(file_monitor src/file_monitor.bpf.c)
add_dependencies(file_monitor_skel libbpf-build bpftool-build)

# Build the main executable
add_executable(naarad 
    src/file_monitor.c
    ${INCLUDE_SOURCES}
)

# Link the BPF skeleton and any other necessary libraries
target_link_libraries(naarad 
   
   PRIVATE file_monitor_skel
   Threads::Threads
   atomic
        ${CURSES_LIBRARIES}

    # Add any other libraries here if needed
)
