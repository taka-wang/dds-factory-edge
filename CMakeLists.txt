cmake_minimum_required(VERSION 2.8)
project(edge C)

SET(CMAKE_VERBOSE_MAKEFILE ON)
# cflag
SET(EXT_C_FLAGS "-m64 -pipe -Wall -fno-strict-aliasing -O3 -std=c99 -Wstrict-prototypes -Wmissing-prototypes -Dos_linux_gcc_x86 -DLITE=1 -DNDEBUG -D_GNU_SOURCE")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXT_C_FLAGS}")

SET(CMAKE_INSTALL_RPATH "/opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/") 
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
SET(CMAKE_INSTALL_PREFIX "/usr/")

add_subdirectory(src/generated)
#add_subdirectory(src/threadpool)
add_subdirectory(src/c-ini-parser)
add_subdirectory(test)
add_subdirectory(src)