# Build language for Red Alert. Chosen with -DRA_LANGUAGE=<english|german|french>
# or, in non-Release builds, with LANGUAGE=<...> in .env (see .env.example).
set(RA_LANGUAGE "english" CACHE STRING "Red Alert build language: english, german or french")
set_property(CACHE RA_LANGUAGE PROPERTY STRINGS english german french)

# Only ra/core/config.h reads these definitions; the code selects on
# config::kBuildLanguage. English is the default and needs no definition.
function(apply_language_setting TARGET_NAME)
    string(TOLOWER "${RA_LANGUAGE}" LANGUAGE_LOWER)
    if (LANGUAGE_LOWER STREQUAL "german")
        target_compile_definitions(${TARGET_NAME} PRIVATE GERMAN=1)
    elseif (LANGUAGE_LOWER STREQUAL "french")
        target_compile_definitions(${TARGET_NAME} PRIVATE FRENCH=1)
    elseif (NOT LANGUAGE_LOWER STREQUAL "english")
        message(FATAL_ERROR "RA_LANGUAGE must be english, german or french, not '${RA_LANGUAGE}'")
    endif ()
endfunction()

add_executable(rasdl WIN32 startup/startup.cc)
add_library(ra_engine OBJECT)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER RA_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.cc"
)

file(GLOB_RECURSE RA_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM RA_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/net/nullconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/net/nullmgr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/net/wspudp.h"
)

target_sources(ra_engine PRIVATE
        ${RA_SOURCES}
)

target_sources(ra_engine PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${RA_HEADERS}
)

set_target_properties(rasdl PROPERTIES
        ENABLE_EXPORTS ON
)

if (WIN32)
    target_sources(rasdl PRIVATE
            cc_icon.rc
    )
    target_link_libraries(ra_engine PUBLIC wsock32)
endif ()

apply_dotenv_settings(ra_engine)
apply_dotenv_settings(rasdl)
apply_language_setting(ra_engine)
apply_language_setting(rasdl)
# The window icon; cc_icon.rc gives the Windows executable its file icon.
embed_file(ra_engine redalert.ico kRedAlertIcon)
# magic_enum is PUBLIC because the public header core/defines.h includes it; the
# interface-header-set verification compiles that header standalone.
# startup/sdlstub.cc includes SDL headers directly, so ra_engine links SDL2 itself
# rather than relying on it arriving transitively through engine_window/
# engine_video.
find_package(SDL2 REQUIRED)
target_link_libraries(ra_engine PUBLIC game_core game_sim game_ui absl::strings magic_enum::magic_enum
        engine_net
        engine_window
        engine_video
        SDL2::SDL2
        absl::check
        absl::core_headers
        absl::log
        absl::log_initialize
        absl::no_destructor
        absl::str_format) # mpgdll wwipx32
target_link_libraries(rasdl PRIVATE ra_engine)

install(TARGETS rasdl)

# What a test that builds a real Game compiles in: GameFixture and the text
# table it can install; a test of a dialog adds UiFixture and its font.
set(RA_GAME_TEST_UTIL
    startup/game_test_util.cc
    ${CMAKE_SOURCE_DIR}/src/engine/file/string_table_test_util.cc
    ${CMAKE_SOURCE_DIR}/src/engine/file/game_files_test_util.cc
)
set(RA_UI_TEST_UTIL
    startup/ui_test_util.cc
    ${CMAKE_SOURCE_DIR}/src/engine/gfx/font_test_util.cc
)

# Tests
include(Testing)
add_gtest(
    NAME ra_profile_test
    SOURCES data/profile_test.cc data/profile.cc
    LIBS engine_file absl::strings absl::check absl::log magic_enum::magic_enum
)

# Carries out trigger actions in a real Game.
add_gtest(
    NAME ra_trigger_action_test
    SOURCES ai/trigger_action_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_trigger_action_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_trigger_action_test)
apply_language_setting(ra_trigger_action_test)

add_gtest(
    NAME ra_waypoint_name_test
    SOURCES ai/waypoint_name_test.cc ai/waypoint_name.cc
    LIBS absl::strings
)

# Reads, writes, saves, detaches and draws trigger types in a real Game.
add_gtest(
    NAME ra_trigger_type_test
    SOURCES ai/trigger_type_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_trigger_type_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_trigger_type_test)
apply_language_setting(ra_trigger_type_test)

# The real engine, with the startup helpers but no game entry point.
add_gtest(
    NAME ra_game_test
    SOURCES startup/game_test.cc objects/target_test.cc objects/fuse_test.cc
            objects/mission_test.cc objects/radio_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_game_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_game_test)
apply_language_setting(ra_game_test)

# Builds type objects, a house and a vehicle in a real Game.
add_gtest(
    NAME ra_carryover_test
    SOURCES ai/carryover_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_carryover_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_carryover_test)
apply_language_setting(ra_carryover_test)

# Plays a score through SDL's dummy audio device in a real Game.
add_gtest(
    NAME ra_focus_test
    SOURCES startup/focus_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_focus_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_focus_test)
apply_language_setting(ra_focus_test)

# Reads warhead sections into the type heap of a real Game.
add_gtest(
    NAME ra_warhead_test
    SOURCES objects/warhead_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_warhead_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_warhead_test)
apply_language_setting(ra_warhead_test)

# Records a kill between two vehicles in a real Game.
add_gtest(
    NAME ra_techno_test
    SOURCES objects/techno_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_techno_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_techno_test)
apply_language_setting(ra_techno_test)

# Delivers teams and launches an air strike in a real Game.
add_gtest(
    NAME ra_reinforcements_test
    SOURCES ai/reinforcements_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_reinforcements_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_reinforcements_test)
apply_language_setting(ra_reinforcements_test)

# A team working through its script, with a real Game.
add_gtest(
    NAME ra_team_test
    SOURCES ai/team_test.cc ${RA_GAME_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_team_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_team_test)
apply_language_setting(ra_team_test)

# The text table lookup, the random picks and the coordinate helpers in a real
# Game.
add_gtest(
    NAME ra_core_helpers_test
    SOURCES
        data/text_test.cc
        core/random_pick_test.cc
        core/coord_test.cc
        ${RA_GAME_TEST_UTIL}
        startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_core_helpers_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_core_helpers_test)
apply_language_setting(ra_core_helpers_test)

add_gtest(
    NAME ra_drop_test
    SOURCES ui/drop_test.cc ui/tlist_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_drop_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_drop_test)
apply_language_setting(ra_drop_test)

add_gtest(
    NAME ra_options_test
    SOURCES data/options_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_options_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_options_test)
apply_language_setting(ra_options_test)

add_gtest(
    NAME ra_special_options_test
    SOURCES core/special_flags_test.cc screens/special_options_test.cc
            ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL} startup/startup.cc
    LIBS ra_engine base
)
target_compile_definitions(ra_special_options_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_special_options_test)
apply_language_setting(ra_special_options_test)

add_gtest(
    NAME ra_difficulty_test
    SOURCES screens/difficulty_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine base
)
target_compile_definitions(ra_difficulty_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_difficulty_test)
apply_language_setting(ra_difficulty_test)

add_gtest(
    NAME ra_text_style_test
    SOURCES ui/text_style_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_text_style_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_text_style_test)
apply_language_setting(ra_text_style_test)

# Prints with TextPrinter onto a page of its own, in UiFixture's block font.
add_gtest(
    NAME ra_text_printer_test
    SOURCES ui/text_printer_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_text_printer_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_text_printer_test)
apply_language_setting(ra_text_printer_test)

# Draws a player colour swatch onto a page of its own.
add_gtest(
    NAME ra_color_box_test
    SOURCES ui/color_box_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine
)
target_compile_definitions(ra_color_box_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_color_box_test)
apply_language_setting(ra_color_box_test)

add_gtest(
    NAME ra_shape_draw_test
    SOURCES assets/shape_draw_test.cc startup/startup.cc
    LIBS ra_engine
)
target_compile_definitions(ra_shape_draw_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_shape_draw_test)
apply_language_setting(ra_shape_draw_test)

add_gtest(
    NAME ra_assets_test
    SOURCES assets/assets_test.cc assets/assets.cc
    LIBS base engine_file magic_enum::magic_enum absl::strings absl::check absl::log
)
apply_language_setting(ra_assets_test)

add_gtest(
    NAME ra_debug_state_test
    SOURCES debug/debug_state_test.cc
    LIBS base
)

add_gtest(
    NAME ra_palettes_test
    SOURCES assets/palettes_test.cc
    LIBS base engine_gfx magic_enum::magic_enum
)
apply_language_setting(ra_palettes_test)

add_gtest(
    NAME ra_palette_test
    SOURCES assets/palette_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_palette_test PRIVATE RA_NO_ENTRY_POINT=1)

add_gtest(
    NAME ra_fading_table_builders_test
    SOURCES core/fading_table_builders_test.cc startup/startup.cc
    LIBS ra_engine
)
target_compile_definitions(ra_fading_table_builders_test PRIVATE RA_NO_ENTRY_POINT=1)

add_gtest(
    NAME ra_screen_test
    SOURCES assets/screen_test.cc assets/screen.cc
    LIBS base engine_window
)

add_gtest(
    NAME ra_mission_id_test
    SOURCES data/mission_id_test.cc data/mission_id.cc
    LIBS absl::strings
)

add_gtest(
    NAME ra_hall_of_fame_test
    SOURCES screens/hall_of_fame_test.cc screens/hall_of_fame.cc
    LIBS base
)

# Plays the scenario editor's trigger dialog on a headless screen.
add_gtest(
    NAME ra_trigger_type_dialog_test
    SOURCES editor/trigger_type_dialog_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_trigger_type_dialog_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_trigger_type_dialog_test)
apply_language_setting(ra_trigger_type_dialog_test)

# Plays the scenario editor's team type dialog on a headless screen.
add_gtest(
    NAME ra_team_type_dialog_test
    SOURCES editor/team_type_dialog_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_team_type_dialog_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_team_type_dialog_test)
apply_language_setting(ra_team_type_dialog_test)

# Reads and writes team types in a real Game.
add_gtest(
    NAME ra_team_type_test
    SOURCES ai/team_type_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_team_type_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_team_type_test)
apply_language_setting(ra_team_type_test)

# Plays the scenario editor's trigger and team lists on a headless screen.
add_gtest(
    NAME ra_map_editor_lists_test
    SOURCES editor/map_editor_lists_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL}
            startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_map_editor_lists_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_map_editor_lists_test)
apply_language_setting(ra_map_editor_lists_test)

# Rolls the credits in a real Game on a headless screen.
add_gtest(
    NAME ra_credits_roll_test
    SOURCES screens/credits_roll_test.cc ${RA_GAME_TEST_UTIL} ${RA_UI_TEST_UTIL} startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_credits_roll_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_credits_roll_test)
apply_language_setting(ra_credits_roll_test)

add_gtest(
    NAME ra_credits_text_test
    SOURCES screens/credits_text_test.cc screens/credits_text.cc
    LIBS base
)

add_gtest(
    NAME ra_dib_test
    SOURCES assets/dib_test.cc assets/dib.cc
    LIBS base absl::core_headers
)

add_gtest(
    NAME ra_wolapi_iids_test
    SOURCES wol/wolapi_iids_test.cc wol/wolapi_iids.cc
    LIBS engine_platform
)

add_gtest(
    NAME ra_winbits_test
    SOURCES assets/winbits_test.cc assets/winbits.cc assets/dib.cc
    LIBS base engine_gfx absl::strings absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_winbits_test)

add_gtest(
    NAME ra_packed_test
    SOURCES core/packed_test.cc core/coordinate_test.cc core/cell_test.cc
    LIBS base engine_stream game_core absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_packed_test)

add_gtest(
    NAME ra_crate_test
    SOURCES objects/crate_test.cc
    LIBS base engine_stream absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_crate_test)

# Compiles screens/intro.cc directly; the dialog, movie player and title page are stubs.
add_gtest(
    NAME ra_intro_test
    SOURCES screens/intro_test.cc screens/intro.cc
    LIBS base engine_window game_ui absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_intro_test)

add_gtest(
    NAME ra_saveglobals_test
    SOURCES saveload/saveglobals_test.cc saveload/saveglobals.cc
    # SDL for the scancodes engine/window/keyboard.h names its keys by.
    LIBS base engine_net engine_stream SDL2::SDL2 absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_saveglobals_test)

