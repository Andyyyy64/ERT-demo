# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/andy/esp-idf/esp-idf/components/bootloader/subproject"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/tmp"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/src/bootloader-stamp"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/src"
  "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/andy/PROJECT/labo/ERT-DEMO/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
