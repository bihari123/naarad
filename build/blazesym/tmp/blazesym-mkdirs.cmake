# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/media/tarun/1Tb/code/my_projects/naarad/third_party/blazesym"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src/blazesym-build"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/tmp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src/blazesym-stamp"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src"
  "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src/blazesym-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src/blazesym-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/media/tarun/1Tb/code/my_projects/naarad/build/blazesym/src/blazesym-stamp${cfgdir}") # cfgdir has leading slash
endif()
