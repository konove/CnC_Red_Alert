add_executable(rasdl WIN32)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER RA_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/adpcm.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/alloc.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/bmp8.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/comqueue.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cstraw.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibfile.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibutil.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/key.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/mplib.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/mplpc.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrd.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrw.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/noseqcon.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/number.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/rand.cc"
        # rawolapi.cc compiles clean under the strict checks, but cannot link
        # yet: it calls ten functions that wolapiob.cc, wol_gsup.cc and
        # wol_dnld.cc define, and those three are still guarded out. Remove
        # this line together with their guards.
        "${CMAKE_CURRENT_SOURCE_DIR}/rawolapi.cc"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/comqueue.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/crew.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/cstraw.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibapi.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibutil.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/externs.h" # Removed since it is crashing IWYU.
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/key.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/memcheck.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/movie.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrd.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrw.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/noseqcon.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/nullmgr.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/screen.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/seqconn.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/tcpip.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/tenmgr.h"
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
            dde.cc
            ccdde.cc
            cc_icon.rc
    )
    target_link_libraries(rasdl PRIVATE wsock32)
endif ()

target_compile_definitions(rasdl PRIVATE
        ENGLISH=1  # TODO: also french/german
        PORTABLE=1 # to indicate not windows or dos sometimes
)
apply_dotenv_settings(rasdl)
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

#	The Westwood Online event sinks. They compile clean under the strict
#	checks but cannot be linked into rasdl yet: they call ten functions that
#	wolapiob.cc, wol_gsup.cc and wol_dnld.cc define, and those three are still
#	guarded out. An object library builds them -- so clang-tidy, IWYU and
#	-Weverything keep reading them -- without anything having to link the
#	result. Fold these sources back into rasdl once those guards come off.
add_library(ra_wol_unlinked OBJECT rawolapi.cc)
target_compile_definitions(ra_wol_unlinked PRIVATE ENGLISH=1 PORTABLE=1)
target_link_libraries(ra_wol_unlinked PRIVATE tech port sdllib vqa32
        absl::check absl::log absl::strings)

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
target_compile_definitions(ra_winbits_test PRIVATE ENGLISH=1 PORTABLE=1)

add_gtest(
    NAME ra_heap_layout_test
    SOURCES heap_layout_test.cc
    LIBS base tech port sdllib vqa32 absl::strings absl::check absl::log
)
target_compile_definitions(ra_heap_layout_test PRIVATE ENGLISH=1 PORTABLE=1)

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
