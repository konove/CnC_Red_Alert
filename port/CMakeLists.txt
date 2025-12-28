add_library(port STATIC
    string.cpp
)

target_include_directories(port PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_compile_definitions(port PUBLIC PORTABLE=1)

# Enable IWYU for all targets in this directory
enable_iwyu_for_all_targets()