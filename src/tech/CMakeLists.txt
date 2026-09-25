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
# glow_pulse.h includes engine/platform/ftimer.h; game_file_vqa_io.h and
# audio_mixer.h include engine/stream/byte_stream.h and friends.
# audio_mixer.cc, pcx_file.cc, game_file_vqa_io.cc and wsa_animation.cc still
# use engine/file privately; no public tech header exposes engine/crypto or
# engine/file any more. Both stay PUBLIC anyway: dozens of ra/ and td/
# translation units include those headers directly and, until they link the
# engine libraries themselves, reach them only through tech's PUBLIC link.
find_package(SDL2 REQUIRED)
target_link_libraries(tech
        PUBLIC engine_base engine_platform engine_stream engine_crypto engine_file absl::strings SDL2::SDL2
        PRIVATE sdllib port vqa32 engine_codec
)

# Tests
include(Testing)
add_gtest(
    NAME tech_test
    SOURCES
        packet_test.cc
        rgb_test.cc glow_pulse_test.cc wsa_animation_test.cc audio_mixer_test.cc
    LIBS tech
)
