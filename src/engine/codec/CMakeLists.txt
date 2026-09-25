# engine_codec: src/engine/codec/ (tools/engine_layout.py).

add_library(engine_codec STATIC)

file(GLOB ENGINE_CODEC_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_CODEC_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_CODEC_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_codec PRIVATE
        ${ENGINE_CODEC_SOURCES}
)

target_sources(engine_codec PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_CODEC_HEADERS}
)

target_link_libraries(engine_codec
        PUBLIC engine_stream
        PRIVATE absl::core_headers lzo
)

# Byte- and pixel-crunching kernels: keep them fast when debugging
# everything else (cmake/OptimizeInDebug.cmake).
optimize_in_debug(lcw.cc lcw_uncompress.cc)

# Tests
include(Testing)
add_gtest(
        NAME engine_codec_test
        SOURCES aud_decoder_test.cc codec_corrupt_test.cc codec_state_test.cc
            compressed_block_test.cc lcw_comp_test.cc lcw_uncompress_test.cc stream_error_test.cc
            stream_golden_test.cc xor_delta_test.cc
        LIBS engine_codec engine_crypto absl::strings
)
