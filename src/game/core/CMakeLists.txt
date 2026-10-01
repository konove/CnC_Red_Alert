# game_core: src/game/core/ (tools/engine_layout.py).

add_library(game_core STATIC)

file(GLOB GAME_CORE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER GAME_CORE_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB GAME_CORE_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(game_core PRIVATE
        ${GAME_CORE_SOURCES}
)

target_sources(game_core PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${GAME_CORE_HEADERS}
)

target_link_libraries(game_core
        PUBLIC base absl::check
        PRIVATE engine_window
)

# Tests
include(Testing)
add_gtest(
        NAME game_core_test
        SOURCES path_overlap_test.cc
        LIBS game_core
)
