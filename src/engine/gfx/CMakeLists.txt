# engine_gfx: src/engine/gfx/ (tools/engine_layout.py).

add_library(engine_gfx STATIC)

file(GLOB ENGINE_GFX_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_GFX_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_GFX_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_gfx PRIVATE
        ${ENGINE_GFX_SOURCES}
)

target_sources(engine_gfx PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_GFX_HEADERS}
)

target_link_libraries(engine_gfx
        PUBLIC engine_file absl::core_headers
        PRIVATE absl::check absl::strings
)

# Byte- and pixel-crunching kernels: keep them fast when debugging
# everything else (cmake/OptimizeInDebug.cmake).
optimize_in_debug(fading_table.cc pixel_buffer.cc)

# Tests
include(Testing)
add_gtest(
        NAME engine_gfx_test
        SOURCES font_test.cc glow_pulse_test.cc pixel_buffer_test.cc rgb_test.cc stamp_test.cc
            wsa_animation_test.cc
        LIBS engine_gfx
)

# Exercises the game's key-frame/delta chain using the real bounded LCW decoder.
add_gtest(
        NAME keyframe_test
        SOURCES keyframe_test.cc ../../ra/2keyfram.cc
        LIBS engine_gfx
)
