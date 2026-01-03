add_library(port STATIC
        string.cpp
)

target_compile_definitions(port PUBLIC PORTABLE=1)
