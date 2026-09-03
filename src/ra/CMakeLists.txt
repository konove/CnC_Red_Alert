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

add_executable(rasdl WIN32)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER RA_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/adpcm.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/alloc.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/bmp8.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/comqueue.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cstraw.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/noseqcon.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/number.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/rand.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/seqconn.cc"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/bufferx.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/comqueue.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/crew.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cstraw.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/externs.h" # Removed since it is crashing IWYU.
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/memcheck.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/noseqcon.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullmgr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/screen.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/seqconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/turret.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/w95trace.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wspipx.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wspudp.h"
)

target_sources(rasdl PRIVATE
        ${RA_SOURCES}
)

target_sources(rasdl PUBLIC
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
    target_link_libraries(rasdl PRIVATE wsock32)
endif ()

apply_dotenv_settings(rasdl)
apply_language_setting(rasdl)
target_link_libraries(rasdl PUBLIC absl::strings PRIVATE
        tech
        port
        sdllib
        vqa32
        absl::check
        absl::core_headers
        absl::log
        absl::log_initialize
        absl::status
        absl::statusor
        absl::str_format) # mpgdll wwipx32

install(TARGETS rasdl)

# Tests
include(Testing)
add_gtest(
    NAME ra_mission_id_test
    SOURCES mission_id_test.cc mission_id.cc
    LIBS absl::strings
)

add_gtest(
    NAME ra_dib_test
    SOURCES dib_test.cc dib.cc
    LIBS base
)

add_gtest(
    NAME ra_wolapi_iids_test
    SOURCES wolapi/wolapi_iids_test.cc wolapi/wolapi_iids.cc
    LIBS port
)

add_gtest(
    NAME ra_winbits_test
    SOURCES winbits_test.cc winbits.cc dib.cc
    LIBS base tech port sdllib vqa32 absl::strings absl::check absl::log
)
apply_language_setting(ra_winbits_test)

add_gtest(
    NAME ra_heap_layout_test
    SOURCES heap_layout_test.cc
    LIBS base tech port sdllib vqa32 absl::strings absl::check absl::log
)
apply_language_setting(ra_heap_layout_test)

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
