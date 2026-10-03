add_executable(tdsdl WIN32 startup/startup.cc)
add_library(td_engine OBJECT)

file(GLOB_RECURSE TD_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER TD_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM TD_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/ccdde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/dde.cc"
)

file(GLOB_RECURSE TD_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM TD_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/dde.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/net/phone.h"
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
            startup/dde.cc
            startup/ccdde.cc
    )
    target_sources(tdsdl PRIVATE cc_icon.rc)
    target_link_libraries(td_engine PUBLIC wsock32)
endif ()

apply_dotenv_settings(td_engine)
apply_dotenv_settings(tdsdl)
# The window icon; cc_icon.rc gives the Windows executable its file icon.
embed_file(td_engine conquer.ico kConquerIcon)
# startup/sdlstub.cc includes SDL headers directly, so td_engine links SDL2 itself
# rather than relying on it arriving transitively through engine_window/
# engine_video.
find_package(SDL2 REQUIRED)
target_link_libraries(td_engine PUBLIC game_core game_sim game_ui engine_net engine_window engine_video SDL2::SDL2 absl::strings absl::algorithm absl::check absl::core_headers absl::log absl::log_initialize absl::no_destructor absl::random_random absl::span)
target_link_libraries(tdsdl PRIVATE td_engine)

install(TARGETS tdsdl)

# Tests
include(Testing)
# Real engine serializers, with startup helpers but no game entry point.
add_gtest(
    NAME td_archive_roundtrip_test
    SOURCES saveload/archive_roundtrip_test.cc data/scenario_ini_test.cc data/options_test.cc startup/setup_options_test.cc assets/pcx_test.cc data/number_parse_test.cc net/queue_alignment_test.cc ui/list_test.cc startup/startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_archive_roundtrip_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_archive_roundtrip_test)

add_gtest(
    NAME td_text_style_test
    SOURCES ui/text_style_test.cc startup/startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_text_style_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_text_style_test)

# The serializers' value tests. They derive from and construct AbstractClass,
# FlyClass and StageClass, whose vtables live with their out-of-line
# destructors in the engine, so the test links the engine without its entry
# point.
add_gtest(
    NAME td_saveload_test
    SOURCES saveload/savevalues_test.cc startup/startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_saveload_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_saveload_test)

add_gtest(
    NAME td_game_test
    SOURCES startup/game_test.cc core/target_test.cc startup/startup.cc
    LIBS td_engine absl::core_headers
)
target_compile_definitions(td_game_test PRIVATE TD_NO_ENTRY_POINT=1)
apply_dotenv_settings(td_game_test)

add_gtest(
    NAME td_assets_test
    SOURCES assets/assets_test.cc assets/assets.cc
    # SDL for the scancodes engine/window/keyboard.h names its keys by.
    LIBS base engine_file SDL2::SDL2 magic_enum::magic_enum absl::check absl::log absl::strings
)

add_gtest(
    NAME td_debug_state_test
    SOURCES debug/debug_state_test.cc
    LIBS base
)

add_gtest(
    NAME td_palettes_test
    SOURCES assets/palettes_test.cc
    LIBS base
)

add_gtest(
    NAME td_screen_test
    SOURCES assets/screen_test.cc assets/screen.cc
    LIBS base engine_window
)

add_gtest(
    NAME td_support_test
    SOURCES core/support_test.cc core/support.cc
    LIBS base engine_gfx absl::check
)


add_gtest(
    NAME td_coord_inline_test
    SOURCES core/coord_inline_test.cc core/coordinate_test.cc core/cell_test.cc
    LIBS base engine_stream absl::check absl::log
)

# Real cell array from the engine, without a game entry point.
add_gtest(
    NAME td_cell_adjacent_test
    SOURCES objects/cell_adjacent_test.cc startup/startup.cc
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
