add_executable(tdsdl WIN32)

file(GLOB_RECURSE TD_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
list(REMOVE_ITEM TD_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cdfile.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/rawfile.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cpp"
)

file(GLOB_RECURSE TD_HEADERS
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)
list(REMOVE_ITEM TD_HEADERS
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/externs.h" # Removed since it is crashing IWYU.
        "${CMAKE_CURRENT_SOURCE_DIR}/phone.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/screen.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/session.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/wwfile.h"
)

target_sources(tdsdl PRIVATE
        ${TD_SOURCES}
)

target_sources(tdsdl PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES ${TD_HEADERS}
)

set_target_properties(tdsdl PROPERTIES
        ENABLE_EXPORTS ON
)

if (WIN32)
    target_sources(tdsdl PRIVATE
            dde.cpp
            ccdde.cpp
            cc_icon.rc
    )
    target_link_libraries(tdsdl PRIVATE wsock32)
endif ()

target_compile_definitions(tdsdl PRIVATE
        PORTABLE=1 # to indicate not windows or dos sometimes
        TD=1
)
target_link_libraries(tdsdl PRIVATE tech port sdllib vqa32 absl::check absl::log)

install(TARGETS tdsdl)

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