# engine_video: src/engine/video/ (tools/engine_layout.py).

add_library(engine_video STATIC)

file(GLOB ENGINE_VIDEO_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_VIDEO_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_VIDEO_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_video PRIVATE
        ${ENGINE_VIDEO_SOURCES}
)

target_sources(engine_video PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_VIDEO_HEADERS}
)

find_package(SDL2 REQUIRED)
target_link_libraries(engine_video
        PUBLIC engine_file engine_audio engine_vqa SDL2::SDL2 absl::core_headers
)
