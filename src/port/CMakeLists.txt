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

target_link_libraries(port
        PUBLIC engine_base absl::check absl::log absl::span absl::str_format absl::strings
)
# inet_ntop (inet_text.cc) is in Winsock 2, not the wsock32 the games link.
if(WIN32)
    target_link_libraries(port PUBLIC ws2_32)
endif()

# Tests
include(Testing)
add_gtest(
    NAME port_test
    SOURCES inet_text_test.cc profile_buffer_test.cc socket_bytes_test.cc
    LIBS port engine_base
)
