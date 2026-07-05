add_library(vqa32 STATIC)

file(GLOB_RECURSE VQA32_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER VQA32_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE VQA32_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(vqa32 PRIVATE
        ${VQA32_SOURCES}
)

target_sources(vqa32 PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${VQA32_HEADERS}
)

target_link_libraries(vqa32 PRIVATE port)

find_package(SDL2 REQUIRED)
target_link_libraries(vqa32 PRIVATE SDL2::SDL2)

# Tests
include(Testing)
add_gtest(
    NAME vqa32_test
    SOURCES vqaplay_test.cc
    LIBS vqa32
)
