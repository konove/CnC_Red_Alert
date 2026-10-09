# game_ui: src/game/ui/ (tools/engine_layout.py).

add_library(game_ui STATIC)

file(GLOB GAME_UI_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER GAME_UI_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB GAME_UI_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(game_ui PRIVATE
        ${GAME_UI_SOURCES}
)

target_sources(game_ui PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${GAME_UI_HEADERS}
)

target_link_libraries(game_ui
        PUBLIC game_core engine_window base absl::core_headers
        PRIVATE absl::check
)

# Tests
include(Testing)
add_gtest(
        NAME game_ui_test
        SOURCES checkbox_test.cc gadget_list_test.cc link_test.cc
        LIBS game_ui
)
