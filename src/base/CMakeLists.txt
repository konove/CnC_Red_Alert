add_library(base INTERFACE)

file(GLOB_RECURSE BASE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(base PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${BASE_HEADERS}
)

target_link_libraries(base INTERFACE absl::check magic_enum::magic_enum)

# Tests
include(Testing)
add_gtest(
        NAME base_test
        SOURCES enum_array_test.cc hsv_test.cc numeric_test.cc
        LIBS base absl::core_headers
)
