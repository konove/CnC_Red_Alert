# base: src/base/ and strings/ (tools/engine_layout.py).

add_library(base STATIC)

file(GLOB BASE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/strings/*.cc"
)
list(FILTER BASE_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB BASE_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/strings/*.h"
)

target_sources(base PRIVATE
        ${BASE_SOURCES}
)

target_sources(base PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${BASE_HEADERS}
)

target_link_libraries(base
        PUBLIC absl::check absl::core_headers absl::span absl::str_format absl::strings
            magic_enum::magic_enum
        PRIVATE absl::log
)

# Tests
include(Testing)
add_gtest(
        NAME base_test
        SOURCES array_test.cc buffer_test.cc bytes_of_test.cc clip_test.cc enum_array_test.cc
            fixed_test.cc flags_test.cc hsv_test.cc installed_test.cc numeric_test.cc
            strings/format_test.cc strings/number_parse_test.cc strings/safe_string_test.cc
            strings/tokenizer_test.cc unaligned_test.cc
        LIBS base engine_stream
)
