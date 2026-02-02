add_executable(rasdl WIN32)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
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
        "${CMAKE_CURRENT_SOURCE_DIR}/seqconn.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/stub.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/tooltip.cc"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/tooltip.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/turret.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/w95trace.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wolapi/wolapi.h"
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
target_link_libraries(rasdl PRIVATE
        tech
        port
        sdllib
        vqa32
        absl::check
        absl::log
        absl::log_initialize
        absl::status
        absl::statusor) # mpgdll wwipx32

install(TARGETS rasdl)

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
