add_library(vqa32 STATIC)

file(GLOB_RECURSE VQA32_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER VQA32_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE VQA32_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(vqa32 PRIVATE
        ${VQA32_SOURCES}
)

target_sources(vqa32 PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${VQA32_HEADERS}
)

target_link_libraries(vqa32 PUBLIC engine_base PRIVATE port sdllib absl::log)

find_package(SDL2 REQUIRED)
target_link_libraries(vqa32 PRIVATE SDL2::SDL2)

# Tests
include(Testing)
add_gtest(
    NAME vqa32_test
    SOURCES vqa_player_test.cc movie_test.cc vq_decoder_test.cc chunk_reader_test.cc lcw_buffer_test.cc adpcm_decoders_test.cc frame_ring_test.cc audio_ring_test.cc audio_output_test.cc movie_clock_test.cc movie_loader_test.cc movie_drawer_test.cc
    LIBS vqa32 engine_base port sdllib absl::core_headers SDL2::SDL2
)
