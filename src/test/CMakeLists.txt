
SET(PUB1_NAME pub1)
SET(PUB2_NAME pub2)
SET(PUB3_NAME pub3)

# libdds location
SET(DDS_LIB_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/libdds.so)

# dds include files
SET(DDS_HEADER_INCLUDE_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/include/)

# source codes
SET(PUB1_SOURCES pub1.c share.c)
SET(PUB2_SOURCES pub2.c share.c)
SET(PUB3_SOURCES pub3.c share.c)

include_directories (${DDS_HEADER_INCLUDE_DIR} )
include_directories (${edge_SOURCE_DIR}/src/generated)
include_directories (${edge_SOURCE_DIR}/src/test)

# pub1
ADD_EXECUTABLE(${PUB1_NAME} ${PUB1_SOURCES})
TARGET_LINK_LIBRARIES(${PUB1_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)

# pub2
ADD_EXECUTABLE(${PUB2_NAME} ${PUB2_SOURCES})
TARGET_LINK_LIBRARIES(${PUB2_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)

# pub3
ADD_EXECUTABLE(${PUB3_NAME} ${PUB3_SOURCES})
TARGET_LINK_LIBRARIES(${PUB3_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen)
