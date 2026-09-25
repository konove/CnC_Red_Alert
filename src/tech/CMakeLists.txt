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
# mix_archive.h's public MixArchive::Register() takes a const PKey*, so
# engine_crypto is PUBLIC.
find_package(SDL2 REQUIRED)
target_link_libraries(tech
        PUBLIC engine_base engine_platform engine_stream engine_crypto absl::strings SDL2::SDL2
        PRIVATE sdllib port vqa32 engine_codec
)

# Tests
include(Testing)
add_gtest(
    NAME tech_test
    SOURCES
        mix_archive_test.cc
        packet_test.cc disk_file_test.cc
        search_paths_test.cc disk_stream_test.cc game_file_test.cc
        rgb_test.cc glow_pulse_test.cc wsa_animation_test.cc audio_mixer_test.cc
    LIBS tech
)
