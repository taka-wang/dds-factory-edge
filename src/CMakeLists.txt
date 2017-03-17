
SET(SUB2_NAME sub2)

# libdds location
SET(DDS_LIB_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/lib/linux_gcc_x86/libdds.so)

# dds include files
SET(DDS_HEADER_INCLUDE_DIR /opt/Prismtech/Vortex_v2/Device/VortexLite/2.0.4/include/)

# source codes
SET(SUB2_SOURCES edge2.c)

include_directories (${DDS_HEADER_INCLUDE_DIR} )
include_directories (${edge_SOURCE_DIR}/src/generated)
include_directories (${edge_SOURCE_DIR}/src/threadpool)
include_directories (${edge_SOURCE_DIR}/src/c-ini-parser/src)

# sub2
ADD_EXECUTABLE(${SUB2_NAME} ${SUB2_SOURCES})
TARGET_LINK_LIBRARIES(${SUB2_NAME} LINK_PUBLIC ${DDS_LIB_DIR} rt pthread gen pool iniparser)
