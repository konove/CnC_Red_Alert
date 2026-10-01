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
list(FILTER RA_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/startup/startup.cc"
)

file(GLOB_RECURSE RA_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM RA_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/objects/crew.h"
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

# Tests
include(Testing)
add_gtest(
    NAME ra_ini_number_test
    SOURCES data/ini_number_test.cc data/ini.cc data/profile_buffer_test.cc data/profile.cc
    LIBS engine_file absl::strings absl::check absl::log magic_enum::magic_enum
)

add_gtest(
    NAME ra_taction_test
    SOURCES ai/taction_test.cc
    LIBS base absl::check absl::log magic_enum::magic_enum
)

# The real engine, with the startup helpers but no game entry point.
add_gtest(
    NAME ra_game_test
    SOURCES startup/game_test.cc startup/startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_game_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_game_test)
apply_language_setting(ra_game_test)

add_gtest(
    NAME ra_drop_test
    SOURCES ui/drop_test.cc startup/startup.cc
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
    NAME ra_assets_test
    SOURCES assets/assets_test.cc assets/assets.cc data/ini.cc
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
    NAME ra_search_test
    SOURCES core/search_test.cc
    LIBS base absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_search_test)

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



if (EMSCRIPTEN)
    set_target_properties(rasdl PROPERTIES
            SUFFIX ".html"
            LINK_FLAGS "-sENVIRONMENT=web -sASYNCIFY -sASYNCIFY_STACK_SIZE=8192 -sFORCE_FILESYSTEM -sINITIAL_MEMORY=32mb"
    )
    # this does not include the files, that would require something like
    # file_packager files.data --js-output=files.js --preload *.mix REDALERT.INI
    # (which is huge if you include the movies)
    # and adding files.js to the shell html
endif ()

