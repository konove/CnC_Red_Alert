# game_sim: src/game/sim/ (tools/engine_layout.py).

add_library(game_sim STATIC)

file(GLOB GAME_SIM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER GAME_SIM_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB GAME_SIM_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(game_sim PRIVATE
        ${GAME_SIM_SOURCES}
)

target_sources(game_sim PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${GAME_SIM_HEADERS}
)

target_link_libraries(game_sim
        PUBLIC game_core
)
if (WIN32)
    # utracker.cc converts counts to network byte order with htonl().
    target_link_libraries(game_sim PRIVATE wsock32)
endif ()

# Tests
include(Testing)
add_gtest(
        NAME game_sim_test
        SOURCES direction_test.cc door_test.cc facing_test.cc utracker_test.cc
        LIBS game_sim engine_stream
)
