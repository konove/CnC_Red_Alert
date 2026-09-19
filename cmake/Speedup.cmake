# Optional build accelerators: ccache for compile caching, ctcache for
# clang-tidy caching, mold for linking.
#
# All three are auto-detected and silently skipped when not installed, so this
# file needs no per-machine configuration. Turn any of them off explicitly with
# -DUSE_CCACHE=OFF / -DUSE_CTCACHE=OFF / -DUSE_MOLD=OFF.
#
# Include this BEFORE any target is created (including FetchContent
# dependencies): the launcher and link options are captured when a target is
# defined, not when it is built.

option(USE_CCACHE "Use ccache to cache compilation results when available" ON)
option(USE_CTCACHE "Use clang-tidy-cache to cache clang-tidy results when available" ON)
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

# clang-tidy runs as its own pass in front of the compiler, so ccache never
# caches it: even a fully cached rebuild pays the whole analysis cost again.
# ctcache (https://github.com/matus-chochlik/ctcache) wraps clang-tidy and
# skips the re-analysis when the preprocessed source and .clang-tidy are
# unchanged. Only useful when the checks actually run, hence the STRICT_CHECKS
# guard. The top-level CMakeLists.txt puts CTCACHE_COMMAND in front of
# CMAKE_CXX_CLANG_TIDY.
#
# The cache is shared by every build dir, so a state one strict dir has
# analyzed is a hit in the other (build-strict and CLion's strict profile).
# That needs the hash to be independent of the build dir, which ctcache
# supports through environment variables, set here with `cmake -E env`:
#   CTCACHE_DIR    one cache in ~/.cache/ctcache (survives reboots, unlike
#                  ctcache's /tmp default); override with -DCTCACHE_DIR.
#   CTCACHE_STRIP  removes this build dir from the hashed arguments and, with
#                  CTCACHE_STRIP_SRC, from the preprocessed source: the _deps
#                  -isystem paths and any __FILE__ inside _deps.
#   CTCACHE_EXCLUDE_HASH_REGEX  leaves out -O*, -g* and the color-diagnostics
#                  flags CLion adds. They cannot change tidy's findings beyond
#                  what the hashed preprocessed source already captures
#                  (-O0 and -O1+ differ in __OPTIMIZE__). No `$` in the regex:
#                  the command runs through `sh -c`.
# Sharing still needs the same build type family (optimized or not) and the
# same .env in both dirs, since both change the preprocessed source.
if (USE_CTCACHE AND STRICT_CHECKS)
    find_program(CTCACHE_PROGRAM NAMES clang-tidy-cache ctcache)
    if (CTCACHE_PROGRAM)
        if (DEFINED ENV{CTCACHE_DIR})
            set(_ctcache_default_dir "$ENV{CTCACHE_DIR}")
        else ()
            set(_ctcache_default_dir "$ENV{HOME}/.cache/ctcache")
        endif ()
        set(CTCACHE_DIR "${_ctcache_default_dir}" CACHE PATH
                "clang-tidy-cache directory, shared by all build dirs")
        # CTCACHE_STRIP is a regex; make the path's . and + literal.
        string(REGEX REPLACE "([.+])" "[\\1]" _ctcache_strip "${CMAKE_BINARY_DIR}/")
        set(CTCACHE_COMMAND
                "${CMAKE_COMMAND}" -E env
                "CTCACHE_DIR=${CTCACHE_DIR}"
                "CTCACHE_STRIP=${_ctcache_strip}"
                "CTCACHE_STRIP_SRC=1"
                "CTCACHE_EXCLUDE_HASH_REGEX=-O|-g|-f(no-)?color-diagnostics|-fdiagnostics-color"
                "${CTCACHE_PROGRAM}")
        message(STATUS "clang-tidy cache enabled: ${CTCACHE_PROGRAM} (${CTCACHE_DIR})")
    else ()
        message(STATUS "clang-tidy-cache not found - clang-tidy re-runs on every build")
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
