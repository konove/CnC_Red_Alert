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
        PRIVATE absl::check absl::core_headers engine_rs_cxx lzo
)

# Tests
include(Testing)
add_gtest(
        NAME engine_codec_test
        SOURCES aud_decoder_differential_test.cc aud_decoder_reference_test_util.cc
            aud_decoder_test.cc codec_corrupt_test.cc codec_state_test.cc
            compressed_block_test.cc lcw_comp_test.cc lcw_uncompress_test.cc
            stream_error_test.cc stream_golden_test.cc xor_delta_differential_test.cc
            xor_delta_reference_test_util.cc xor_delta_test.cc
        LIBS engine_codec engine_crypto base absl::strings
)
