# engine_file: src/engine/file/ (tools/engine_layout.py).

add_library(engine_file STATIC)

file(GLOB ENGINE_FILE_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_FILE_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_FILE_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_file PRIVATE
        ${ENGINE_FILE_SOURCES}
)

target_sources(engine_file PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_FILE_HEADERS}
)

target_link_libraries(engine_file
        PUBLIC engine_codec engine_crypto absl::core_headers absl::no_destructor
        PRIVATE absl::check absl::strings
)

# Tests
include(Testing)
add_gtest(
        NAME engine_file_test
        SOURCES disk_file_test.cc disk_stream_test.cc game_file_test.cc mix_archive_test.cc
            profile_buffer_test.cc search_paths_test.cc
        LIBS engine_file absl::strings
)
