add_library(vqa32 STATIC)

file(GLOB_RECURSE VQA32_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

file(GLOB_RECURSE VQA32_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(vqa32 PRIVATE
        ${VQA32_SOURCES}
)

target_sources(vqa32 PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES ${VQA32_HEADERS}
)

target_link_libraries(vqa32 PRIVATE port)

find_package(SDL2 REQUIRED)
target_link_libraries(vqa32 PRIVATE SDL2::SDL2)

# Enable IWYU for all targets in this directory
enable_iwyu_for_all_targets()
