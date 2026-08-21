add_executable(tdsdl WIN32)

file(GLOB_RECURSE TD_SOURCES
        CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER TD_SOURCES EXCLUDE REGEX "_test\\.cc$")
list(REMOVE_ITEM TD_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/ccdde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/cdfile.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/dde.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/rawfile.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/winstub.cc"
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
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${TD_HEADERS}
)

set_target_properties(tdsdl PROPERTIES
        ENABLE_EXPORTS ON
)

if (WIN32)
    target_sources(tdsdl PRIVATE
            dde.cc
            ccdde.cc
            cc_icon.rc
    )
    target_link_libraries(tdsdl PRIVATE wsock32)
endif ()

target_compile_definitions(tdsdl PRIVATE
        PORTABLE=1 # to indicate not windows or dos sometimes
        TD=1
)
apply_dotenv_settings(tdsdl)
target_link_libraries(tdsdl PUBLIC absl::strings PRIVATE tech port sdllib vqa32 absl::algorithm absl::check absl::core_headers absl::log absl::log_initialize absl::random_random absl::span absl::status absl::statusor)

install(TARGETS tdsdl)

# Tests
include(Testing)
add_gtest(
    NAME td_heap_layout_test
    SOURCES heap_layout_test.cc
    LIBS base tech port sdllib vqa32 absl::strings absl::check absl::log
)
target_compile_definitions(td_heap_layout_test PRIVATE PORTABLE=1 TD=1)

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