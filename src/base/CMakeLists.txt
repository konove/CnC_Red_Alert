# What is left of src/base until phase B's stream step moves seek_origin.h to
# src/engine/stream/; everything else went to engine_base (src/engine/base/).
add_library(base INTERFACE)

file(GLOB_RECURSE BASE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(base PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${BASE_HEADERS}
)
