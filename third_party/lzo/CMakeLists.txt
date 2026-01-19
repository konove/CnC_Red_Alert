add_library(lzo STATIC
    lzo1x_c.cpp
    lzo1x_d.cpp
)

# Use SYSTEM to suppress warnings in consumers
target_include_directories(lzo SYSTEM PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Disable all warnings for third-party code
target_compile_options(lzo PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-w>
    $<$<CXX_COMPILER_ID:MSVC>:/w>
)

# Exclude from clang-tidy and IWYU
set_target_properties(lzo PROPERTIES
    CXX_CLANG_TIDY ""
    CXX_INCLUDE_WHAT_YOU_USE ""
)
