add_executable(rasdl WIN32)

file(GLOB_RECURSE RA_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(REMOVE_ITEM RA_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/adpcm.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/alloc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/bmp8.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cominit.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/comqueue.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/confdlg.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cstraw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/descdlg.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibfile.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/dibutil.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/dpmi.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/ipx95.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/key.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/keyframe.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mplib.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mplpc.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrd.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/mpmgrw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/noseqcon.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/number.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/rand.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/seqconn.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/stub.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/surface.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tarcom.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/tooltip.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/turret.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/w95trace.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cpp"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/tarcom.h"
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
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES ${RA_HEADERS}
)

set_target_properties(rasdl PROPERTIES
        ENABLE_EXPORTS ON
)

if (WIN32)
    target_sources(rasdl PRIVATE
            dde.cpp
            ccdde.cpp
            cc_icon.rc
    )
    target_link_libraries(rasdl PRIVATE wsock32)
endif ()

target_compile_definitions(rasdl PRIVATE
        ENGLISH=1  # TODO: also french/german
        PORTABLE=1 # to indicate not windows or dos sometimes
)
target_link_libraries(rasdl PRIVATE tech jshell port sdllib vqa32 absl::check absl::log absl::status absl::statusor) # mpgdll wwipx32

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
