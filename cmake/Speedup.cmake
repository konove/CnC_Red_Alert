# Optional build accelerators: ccache for compile caching, mold for linking.
#
# Both are auto-detected and silently skipped when not installed, so this file
# needs no per-machine configuration. Turn either off explicitly with
# -DUSE_CCACHE=OFF / -DUSE_MOLD=OFF.
#
# Include this BEFORE any target is created (including FetchContent
# dependencies): the launcher and link options are captured when a target is
# defined, not when it is built.

option(USE_CCACHE "Use ccache to cache compilation results when available" ON)
option(USE_MOLD "Use the mold linker when available" ON)

if (USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE STRING "")
        message(STATUS "ccache enabled: ${CCACHE_PROGRAM}")
    else ()
        message(STATUS "ccache not found - compiling without a cache")
    endif ()
endif ()

# CMAKE_LINKER_TYPE needs CMake 3.29; -fuse-ld=mold works on GCC >= 12.1 and
# Clang >= 12, which covers every compiler this project builds with on Linux.
if (USE_MOLD AND CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND NOT MSVC)
    find_program(MOLD_PROGRAM mold)
    if (MOLD_PROGRAM)
        include(CheckCXXSourceCompiles)
        set(CMAKE_REQUIRED_LINK_OPTIONS -fuse-ld=mold)
        check_cxx_source_compiles("int main() { return 0; }" MOLD_LINKER_WORKS)
        unset(CMAKE_REQUIRED_LINK_OPTIONS)
        if (MOLD_LINKER_WORKS)
            add_link_options(-fuse-ld=mold)
            message(STATUS "mold linker enabled: ${MOLD_PROGRAM}")
        else ()
            message(STATUS "mold found but -fuse-ld=mold rejected by the compiler - using the default linker")
        endif ()
    else ()
        message(STATUS "mold not found - using the default linker")
    endif ()
endif ()
