# Build language for Red Alert. Chosen with -DRA_LANGUAGE=<english|german|french>
# or, in non-Release builds, with LANGUAGE=<...> in .env (see .env.example).
set(RA_LANGUAGE "english" CACHE STRING "Red Alert build language: english, german or french")
set_property(CACHE RA_LANGUAGE PROPERTY STRINGS english german french)

# Only ra/config.h reads these definitions; the code selects on
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

add_executable(rasdl WIN32 startup.cc)
add_library(ra_engine OBJECT)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER RA_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/startup.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/adpcm.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/alloc.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/bmp8.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dtable.cc"  # ADPCM tables for adpcm.cc
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/itable.cc"  # ADPCM tables for adpcm.cc
        "${CMAKE_CURRENT_SOURCE_DIR}/number.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/rand.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/stub.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/turret.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/w95trace.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cc"
)

file(GLOB_RECURSE RA_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM RA_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/bmp8.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/crew.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/externs.h" # Removed since it is crashing IWYU.
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/memcheck.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullmgr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/turret.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/w95trace.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wspipx.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wspudp.h"
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
# magic_enum is PUBLIC because the public header defines.h includes it; the
# interface-header-set verification compiles that header standalone.
target_link_libraries(ra_engine PUBLIC absl::strings magic_enum::magic_enum
        tech
        port
        sdllib
        vqa32
        absl::check
        absl::core_headers
        absl::log
        absl::log_initialize
        absl::str_format) # mpgdll wwipx32
target_link_libraries(rasdl PRIVATE ra_engine)

install(TARGETS rasdl)

# Tests
include(Testing)
add_gtest(
    NAME ra_ini_number_test
    SOURCES ini_number_test.cc ini.cc profile_buffer_test.cc profile.cc
    LIBS tech port sdllib absl::strings absl::check absl::log magic_enum::magic_enum
)

# The real engine, with the startup helpers but no game entry point.
add_gtest(
    NAME ra_game_test
    SOURCES game_test.cc startup.cc
    LIBS ra_engine absl::core_headers
)
target_compile_definitions(ra_game_test PRIVATE RA_NO_ENTRY_POINT=1)
apply_dotenv_settings(ra_game_test)
apply_language_setting(ra_game_test)

add_gtest(
    NAME ra_assets_test
    SOURCES assets_test.cc assets.cc ini.cc
    LIBS base port sdllib tech magic_enum::magic_enum absl::strings absl::check absl::log
)
apply_language_setting(ra_assets_test)

add_gtest(
    NAME ra_debug_state_test
    SOURCES debug_state_test.cc
    LIBS base
)

add_gtest(
    NAME ra_palettes_test
    SOURCES palettes_test.cc
    LIBS base tech magic_enum::magic_enum
)
apply_language_setting(ra_palettes_test)

add_gtest(
    NAME ra_screen_test
    SOURCES screen_test.cc screen.cc
    LIBS base sdllib
)

add_gtest(
    NAME ra_mission_id_test
    SOURCES mission_id_test.cc mission_id.cc
    LIBS absl::strings
)

add_gtest(
    NAME ra_count_up_test
    SOURCES count_up_test.cc
)

add_gtest(
    NAME ra_hall_of_fame_test
    SOURCES hall_of_fame_test.cc hall_of_fame.cc
    LIBS base
)

add_gtest(
    NAME ra_dib_test
    SOURCES dib_test.cc dib.cc
    LIBS base absl::core_headers
)

add_gtest(
    NAME ra_wolapi_iids_test
    SOURCES wolapi/wolapi_iids_test.cc wolapi/wolapi_iids.cc
    LIBS port
)

add_gtest(
    NAME ra_winbits_test
    SOURCES winbits_test.cc winbits.cc dib.cc
    LIBS base tech port sdllib vqa32 absl::strings absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_winbits_test)

add_gtest(
    NAME ra_heap_test
    SOURCES heap_test.cc heap.cc
    LIBS base tech absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_heap_test)

add_gtest(
    NAME ra_crate_test
    SOURCES crate_test.cc
    LIBS base tech absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_crate_test)

# Compiles intro.cc directly; the dialog, movie player and mouse are stubs.
add_gtest(
    NAME ra_intro_test
    SOURCES intro_test.cc intro.cc
    LIBS base tech sdllib absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_intro_test)

add_gtest(
    NAME ra_saveglobals_test
    SOURCES saveglobals_test.cc saveglobals.cc ipxaddr.cc
    LIBS base tech sdllib absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_saveglobals_test)

add_gtest(
    NAME ra_fixed_point_test
    SOURCES fixed_point_test.cc
    LIBS base tech absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_fixed_point_test)

add_gtest(
    NAME ra_connect_test
    SOURCES connect_test.cc connect.cc combuf.cc
    LIBS base port absl::check absl::log
)


add_gtest(
    NAME ra_path_overlap_test
    SOURCES path_overlap_test.cc
    LIBS base absl::check absl::log magic_enum::magic_enum
)
apply_language_setting(ra_path_overlap_test)

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

add_gtest(
    NAME ra_utracker_test
    SOURCES utracker_test.cc utracker.cc
    LIBS base
)
