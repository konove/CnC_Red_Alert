# Command line tools for inspecting game data. Not built into either game.

# Opening an installation's data: the search path, the archives and their key.
add_library(tool_game_data STATIC game_data.cc)
target_link_libraries(tool_game_data PUBLIC tech PRIVATE engine_base)

add_executable(mixdump mixdump.cc)
target_link_libraries(mixdump PRIVATE tool_game_data tech absl::str_format)

# Plays one VQA movie in a window, the way the games do.
find_package(SDL2 REQUIRED)
add_executable(vqaplay vqaplay.cc)
target_link_libraries(vqaplay PRIVATE tool_game_data tech vqa32 engine_base
                      absl::str_format absl::strings SDL2::SDL2)
