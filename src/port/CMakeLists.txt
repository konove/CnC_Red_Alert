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

target_link_libraries(port PUBLIC absl::check absl::log absl::span absl::str_format absl::strings)

# Tests
include(Testing)
add_gtest(
    NAME port_test
    SOURCES bytes_of_test.cc env_test.cc format_test.cc inet_text_test.cc random_seed_test.cc profile_buffer_test.cc safe_string_test.cc sleep_test.cc socket_bytes_test.cc tokenizer_test.cc unaligned_test.cc
    LIBS port base
)

add_gtest(
    NAME port_win32_test
    SOURCES win32/win32_com_test.cc win32/win32_system_test.cc
    LIBS port
)
