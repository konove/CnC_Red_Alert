# engine_audio: src/engine/audio/ (tools/engine_layout.py).

add_library(engine_audio STATIC)

file(GLOB ENGINE_AUDIO_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_AUDIO_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_AUDIO_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_audio PRIVATE
        ${ENGINE_AUDIO_SOURCES}
)

target_sources(engine_audio PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_AUDIO_HEADERS}
)

find_package(SDL2 REQUIRED)
target_link_libraries(engine_audio
        PUBLIC engine_file engine_codec SDL2::SDL2 absl::core_headers
        PRIVATE absl::str_format
)

# Tests
include(Testing)
add_gtest(
        NAME engine_audio_test
        SOURCES audio_mixer_test.cc
        LIBS engine_audio
)
