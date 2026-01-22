add_library(port STATIC)

file(GLOB_RECURSE PORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER PORT_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE PORT_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(port PRIVATE
        ${PORT_SOURCES}
)

target_sources(port PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${PORT_HEADERS}
)

target_compile_definitions(port PUBLIC PORTABLE=1)

# Tests
include(Testing)
add_gtest(
    NAME port_test
    SOURCES safe_string_test.cc
    LIBS port
)
