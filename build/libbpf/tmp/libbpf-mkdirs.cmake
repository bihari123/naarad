# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/media/tarun/1Tb/code/my_projects/naarad/third_party/libbpf/src"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src/libbpf-build"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/tmp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src/libbpf-stamp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src"
  "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src/libbpf-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src/libbpf-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/libbpf/src/libbpf-stamp${cfgdir}") # cfgdir has leading slash
endif()
