cmake_minimum_required(VERSION 2.8.9)
project(edge C)

SET(CMAKE_VERBOSE_MAKEFILE ON)

SET(PUB1_NAME pub1)
SET(PUB2_NAME pub2)
SET(PUB3_NAME pub3)
SET(SUB2_NAME sub2)


# libdds location
SET(DDS_LIB_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/libdds.so)

# dds include files
SET(DDS_HEADER_INCLUDE_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/include/)

# source codes
SET(PUB1_SOURCES pub1.c )
SET(PUB2_SOURCES pub2.c )
SET(PUB3_SOURCES pub3.c )
SET(SUB2_SOURCES edge2.c)


# cflag
SET(EXT_C_FLAGS "-m64 -pipe -Wall -fno-strict-aliasing -O3 -std=c99 -Wstrict-prototypes -Wmissing-prototypes -Dos_linux_gcc_x86 -DLITE=1 -DNDEBUG -D_GNU_SOURCE")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXT_C_FLAGS}")

add_subdirectory(generated)
add_subdirectory(threadpool)

include_directories (${DDS_HEADER_INCLUDE_DIR} )
include_directories (${edge_SOURCE_DIR}/generated)
include_directories (${edge_SOURCE_DIR}/threadpool)

# pub1
ADD_EXECUTABLE(${PUB1_NAME} ${PUB1_SOURCES})
TARGET_LINK_LIBRARIES(${PUB1_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)

# pub2
ADD_EXECUTABLE(${PUB2_NAME} ${PUB2_SOURCES})
TARGET_LINK_LIBRARIES(${PUB2_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)

# pub3
ADD_EXECUTABLE(${PUB3_NAME} ${PUB3_SOURCES})
TARGET_LINK_LIBRARIES(${PUB3_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)

# sub2
ADD_EXECUTABLE(${SUB2_NAME} ${SUB2_SOURCES})
TARGET_LINK_LIBRARIES(${SUB2_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen pool)
