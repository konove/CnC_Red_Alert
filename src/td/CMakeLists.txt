add_executable(tdsdl WIN32 startup.cc)
add_library(td_engine OBJECT)

file(GLOB_RECURSE TD_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER TD_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM TD_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/startup.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cc"
)

file(GLOB_RECURSE TD_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM TD_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/externs.h" # Removed since it is crashing IWYU.
        "${CMAKE_CURRENT_SOURCE_DIR}/phone.h"
)

target_sources(td_engine PRIVATE
        ${TD_SOURCES}
)

target_sources(td_engine PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${TD_HEADERS}
)

set_target_properties(tdsdl PROPERTIES
        ENABLE_EXPORTS ON
)

if (WIN32)
    target_sources(td_engine PRIVATE
            dde.cc
            ccdde.cc
    )
    target_sources(tdsdl PRIVATE cc_icon.rc)
    target_link_libraries(td_engine PUBLIC wsock32)
endif ()

target_compile_definitions(td_engine PUBLIC TD=1)
apply_dotenv_settings(td_engine)
apply_dotenv_settings(tdsdl)
# sdlstub.cc includes SDL headers directly, so td_engine links SDL2 itself
# rather than relying on it arriving transitively through engine_window/
# engine_video.
find_package(SDL2 REQUIRED)
target_link_libraries(td_engine PUBLIC engine_net engine_window engine_video SDL2::SDL2 absl::strings absl::algorithm absl::check absl::core_headers absl::log absl::log_initialize absl::random_random absl::span)
target_link_libraries(tdsdl PRIVATE td_engine)

install(TARGETS tdsdl)

# Tests
include(Testing)
# Real engine serializers, with startup helpers but no game entry point.
add_gtest(
    NAME td_archive_roundtrip_test
    SOURCES archive_roundtrip_test.cc pcx_test.cc number_parse_test.cc queue_alignment_test.cc list_test.cc startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_archive_roundtrip_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_archive_roundtrip_test)

add_gtest(
    NAME td_saveload_test
    SOURCES savevalues_test.cc cellvalues.cc scorevalues.cc door.cc flasher.cc fuse.cc super.cc fixedmath.cc rand.cc
    LIBS engine_base engine_stream
)

target_compile_definitions(td_saveload_test PRIVATE TD=1)

add_gtest(
    NAME td_game_test
    SOURCES game_test.cc startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_game_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_game_test)

add_gtest(
    NAME td_assets_test
    SOURCES assets_test.cc assets.cc
    LIBS engine_base engine_file magic_enum::magic_enum absl::check absl::log absl::strings
)

add_gtest(
    NAME td_debug_state_test
    SOURCES debug_state_test.cc
    LIBS engine_base
)

add_gtest(
    NAME td_palettes_test
    SOURCES palettes_test.cc
    LIBS engine_base
)

add_gtest(
    NAME td_screen_test
    SOURCES screen_test.cc screen.cc
    LIBS engine_base engine_window
)

add_gtest(
    NAME td_heap_test
    SOURCES heap_test.cc heap.cc
    LIBS engine_base engine_stream
)
target_compile_definitions(td_heap_test PRIVATE TD=1)

add_gtest(
    NAME td_fixedmath_test
    SOURCES fixedmath_test.cc fixedmath.cc
)
target_compile_definitions(td_fixedmath_test PRIVATE TD=1)

add_gtest(
    NAME td_connect_test
    SOURCES connect_test.cc connect.cc combuf.cc noseqcon.cc
    LIBS engine_base absl::check absl::log
)
target_compile_definitions(td_connect_test PRIVATE TD=1)

add_gtest(
    NAME td_coord_inline_test
    SOURCES coord_inline_test.cc
    LIBS engine_base absl::check absl::log
)
target_compile_definitions(td_coord_inline_test PRIVATE TD=1)

add_gtest(
    NAME td_path_overlap_test
    SOURCES path_overlap_test.cc
    LIBS engine_base absl::check absl::log
)
target_compile_definitions(td_path_overlap_test PRIVATE TD=1)

# Real cell array from the engine, without a game entry point.
add_gtest(
    NAME td_cell_adjacent_test
    SOURCES cell_adjacent_test.cc startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_cell_adjacent_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_cell_adjacent_test)

if (EMSCRIPTEN)
    set_target_properties(tdsdl PROPERTIES
            SUFFIX ".html"
            LINK_FLAGS "-sENVIRONMENT=web -sASYNCIFY -sASYNCIFY_STACK_SIZE=8192 -sFORCE_FILESYSTEM -sINITIAL_MEMORY=32mb"
    )
    # this does not include the files, that would require something like
    # file_packager files.data --js-output=files.js --preload *.mix
    # (which is huge if you include the movies)
    # and adding files.js to the shell html
endif ()
