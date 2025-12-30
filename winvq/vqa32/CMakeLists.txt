add_library(vqa32 STATIC
    audio.cpp
    config.cpp
    drawer.cpp
    dstream.cpp
    loader.cpp
    task.cpp
    unvqbuff.cpp
    vqmstub.cpp
)

target_include_directories(vqa32 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(vqa32 PRIVATE port)

find_package(SDL2 REQUIRED)
target_link_libraries(vqa32 PRIVATE SDL2::SDL2)

# Enable IWYU for all targets in this directory
enable_iwyu_for_all_targets()
