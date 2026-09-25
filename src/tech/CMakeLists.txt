add_library(tech STATIC)

file(GLOB_RECURSE TECH_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER TECH_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE TECH_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(tech PRIVATE
        ${TECH_SOURCES}
)

target_sources(tech PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${TECH_HEADERS}
)

# audio_mixer.h shows SDL audio types to whoever includes it.
# glow_pulse.h includes engine/platform/ftimer.h. disk_file.h, game_file.h and
# many other public headers include engine/stream/byte_stream.h and friends.
find_package(SDL2 REQUIRED)
target_link_libraries(tech
        PUBLIC engine_base engine_platform engine_stream absl::strings SDL2::SDL2
        PRIVATE sdllib port vqa32 lzo
)

# RSA bignum, Blowfish, SHA-1 and LCW run over every byte the game loads or
# saves; keep them fast when debugging everything else.
optimize_in_debug(mp.cc blowfish.cc sha1_compress.cc lcw.cc)

# Tests
include(Testing)
add_gtest(
    NAME tech_test
    SOURCES codec_state_test.cc
        blowfish_test.cc codec_corrupt_test.cc lcw_comp_test.cc mix_archive_test.cc mp_test.cc
        packet_test.cc disk_file_test.cc stream_golden_test.cc blowfish_stream_test.cc pk_stream_test.cc random_source_test.cc stream_error_test.cc
        search_paths_test.cc disk_stream_test.cc game_file_test.cc
        sha1_compress_test.cc rgb_test.cc glow_pulse_test.cc wsa_animation_test.cc audio_mixer_test.cc
        key_phrase_hash_test.cc
    LIBS tech
)
