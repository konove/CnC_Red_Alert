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
# display.h and ww_mouse.h include engine/gfx/pixel_surface.h and
# engine/gfx/pixel_buffer.h.
target_link_libraries(sdllib engine_base engine_platform engine_codec engine_gfx SDL2::SDL2 absl::check absl::log absl::strings)

# Tests
include(Testing)
add_gtest(
        NAME sdllib_test
        SOURCES ww_win_test.cc
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
        NAME sdllib_display_test
        SOURCES display_test.cc
        LIBS sdllib
)
