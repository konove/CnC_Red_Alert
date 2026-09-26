# engine_window: src/engine/window/ (tools/engine_layout.py).

add_library(engine_window STATIC)

file(GLOB ENGINE_WINDOW_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_WINDOW_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_WINDOW_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_window PRIVATE
        ${ENGINE_WINDOW_SOURCES}
)

target_sources(engine_window PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_WINDOW_HEADERS}
)

find_package(SDL2 REQUIRED)
target_link_libraries(engine_window
        # keyboard.h names its keys by SDL scancode, so SDL is public.
        PUBLIC engine_gfx absl::core_headers SDL2::SDL2
        PRIVATE absl::log absl::str_format
)

# PresentScaledFrame's per-pixel palette expansion is a byte-crunching kernel:
# keep it fast when debugging everything else (cmake/OptimizeInDebug.cmake).
optimize_in_debug(display.cc)

# Tests
include(Testing)
add_gtest(
        NAME engine_window_test
        SOURCES display_test.cc ww_win_test.cc
        LIBS engine_window SDL2::SDL2
)

# Compiles keyboard.cc directly and stubs its two hooks, so the key buffer can
# be tested without linking the event loop and mouse cursor code.
add_gtest(
        NAME engine_window_keyboard_test
        SOURCES keyboard_test.cc keyboard.cc
        LIBS base SDL2::SDL2 absl::strings absl::check absl::log
)
