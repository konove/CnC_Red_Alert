# Command line tools for inspecting game data. Not built into either game.

add_executable(mixdump mixdump.cc)

# sdllib supplies the IO_* layer that tech's DiskStream is written against.
target_link_libraries(mixdump PRIVATE tech sdllib absl::str_format)
