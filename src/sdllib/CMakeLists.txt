add_library(sdllib STATIC)

file(GLOB_RECURSE SDLLIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER SDLLIB_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE SDLLIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(sdllib PRIVATE
        ${SDLLIB_SOURCES}
)

target_sources(sdllib PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${SDLLIB_HEADERS}
)

find_package(SDL2 REQUIRED)
# misc.cc includes engine/platform/timer.h. ww_mouse.cc calls
# engine/codec/lcw_uncompress.h to decode the animated cursor shapes.
target_link_libraries(sdllib engine_base engine_platform engine_codec SDL2::SDL2 absl::check absl::log absl::strings)

# The fading-table builder, the per-frame window blit and the LCW decoder loop
# over every palette entry, pixel or compressed byte.
optimize_in_debug(fading_table.cc pixel_buffer.cc)

# Tests
include(Testing)
add_gtest(
        NAME sdllib_test
        SOURCES font_test.cc ww_win_test.cc
        LIBS sdllib
)

# Compiles keyboard.cc directly and stubs its two hooks, so the key buffer can
# be tested without linking the event loop and mouse cursor code.
add_gtest(
        NAME sdllib_keyboard_test
        SOURCES keyboard_test.cc keyboard.cc
        LIBS engine_base SDL2::SDL2 absl::strings absl::check absl::log
)

add_gtest(
        NAME sdllib_pixel_buffer_test
        SOURCES pixel_buffer_test.cc stamp_test.cc
        LIBS sdllib
)

add_gtest(
        NAME sdllib_display_test
        SOURCES display_test.cc
        LIBS sdllib
)

# Exercises the game's key-frame/delta chain using the real bounded LCW decoder.
add_gtest(
        NAME keyframe_test
        SOURCES keyframe_test.cc ../ra/2keyfram.cc
        LIBS sdllib
)
