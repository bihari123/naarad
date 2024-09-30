# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/media/tarun/1Tb/code/my_projects/naarad/third_party/bpftool/src"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src/bpftool-build"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/tmp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src/bpftool-stamp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src"
  "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src/bpftool-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src/bpftool-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/bpftool/src/bpftool-stamp${cfgdir}") # cfgdir has leading slash
endif()
