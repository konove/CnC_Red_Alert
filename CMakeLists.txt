cmake_minimum_required(VERSION 3.14)
project(CnCRedAlert)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to enable strict checks (clang-tidy + extra warnings)
# Disable for quick iteration: cmake -DSTRICT_CHECKS=OFF ..
# Declared before Speedup so it can skip the clang-tidy cache when the checks
# themselves are off.
option(STRICT_CHECKS "Enable clang-tidy and extra compiler warnings" ON)

# ccache / ctcache / mold. Must come before the first target is created so the
# fetched dependencies below are built through them too.
include(Speedup)

# Fetch Abseil
include(FetchContent)
FetchContent_Declare(
        abseil-cpp
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20260107.0
        SYSTEM
)
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_BUILD_TESTING OFF)
FetchContent_MakeAvailable(abseil-cpp)

# GoogleTest for unit testing
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.17.0
        SYSTEM
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# magic_enum: compile-time enum reflection (enum_values, enum_count, enum_name).
FetchContent_Declare(
        magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
        GIT_TAG v0.9.7
        SYSTEM
)
FetchContent_MakeAvailable(magic_enum)
# The game's enums are array indices 0..N-1, sometimes preceded by a negative
# "none" value. Reflecting from 0 makes enum_count/enum_values see exactly the
# index values, so nothing needs FIRST/COUNT sentinels.
target_compile_definitions(magic_enum INTERFACE MAGIC_ENUM_RANGE_MIN=0)

# Option to enable AddressSanitizer for debugging memory issues
option(ENABLE_ASAN "Enable AddressSanitizer for memory debugging" OFF)

if (ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif ()

if (STRICT_CHECKS)
    # Resolve the binary at configure time so a missing clang-tidy is one clear
    # message rather than a failure on every compile. Override with
    # -DCLANG_TIDY_PROGRAM=/path/to/clang-tidy-NN to pin a version.
    find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy DOC "Path to clang-tidy")
    if (NOT CLANG_TIDY_PROGRAM)
        message(FATAL_ERROR "STRICT_CHECKS is ON but clang-tidy was not found. "
                "Install it or configure with -DSTRICT_CHECKS=OFF.")
    endif ()
    set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_PROGRAM}"
            "--header-filter=${CMAKE_SOURCE_DIR}/src/(base|port|sdllib|tech|winvq|ra|td)/.*"
    )
    # Ninja decides what is stale from the command line and the declared inputs,
    # and .clang-tidy is neither: editing it leaves every object untouched and
    # the build reports "no work to do". Hash the config into the tidy
    # invocation so any edit changes the command, and re-run CMake when the file
    # changes so the hash is refreshed. The -D is never referenced by any code;
    # it exists only to carry the hash into the command line.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
            "${CMAKE_SOURCE_DIR}/.clang-tidy")
    file(SHA256 "${CMAKE_SOURCE_DIR}/.clang-tidy" CLANG_TIDY_CONFIG_HASH)
    list(APPEND CMAKE_CXX_CLANG_TIDY
            "--extra-arg=-DCLANG_TIDY_CONFIG_HASH=${CLANG_TIDY_CONFIG_HASH}")

    # ctcache takes the clang-tidy invocation as its arguments, so it goes in
    # front of the list rather than replacing anything in it. Set by Speedup.
    if (CTCACHE_PROGRAM)
        set(CMAKE_CXX_CLANG_TIDY "${CTCACHE_PROGRAM}" ${CMAKE_CXX_CLANG_TIDY})
    endif ()
    message(STATUS "Strict checks enabled (clang-tidy + warnings)")
else ()
    message(STATUS "Strict checks disabled (quick build mode)")
endif ()

# Include What You Use integration (automatically enabled/disabled by STRICT_CHECKS)
include(cmake/IWYU.cmake)

# Development .env file support (disabled in Release builds)
include(cmake/DotEnv.cmake)

if (ENABLE_IWYU AND IWYU_PATH)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND}")
endif ()

if (MSVC)
    add_compile_options(
            /wd4996 # yes, this code is old and uses "unsafe" functions
            /wd4068 /wd4081 # old #pragmas for another compiler
            /wd4661 # missing template definitions, 10000s of these
    )
    # <windows.h> defines min/max macros that break std::min/std::max, and
    # VK_* key codes that collide with the SDL-based ones in sdllib/keyboard.h.
    add_compile_definitions(NOMINMAX NOVIRTUALKEYCODES)
else ()
    if (STRICT_CHECKS)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            # GCC warnings
            add_compile_options(
                    -Wall
                    -Wextra
                    -Wpedantic
                    -Wcast-align
                    -Wcast-qual
                    -Wconversion
                    -Wdouble-promotion
                    -Wduplicated-branches
                    -Wduplicated-cond
                    -Wformat=2
                    -Wimplicit-fallthrough=5
                    -Wlogical-op
                    -Wmissing-declarations
                    -Wnoexcept
                    -Wnoexcept-type
                    -Wnon-virtual-dtor
                    -Wnull-dereference
                    -Wold-style-cast
                    -Woverloaded-virtual
                    -Wredundant-decls
                    -Wshadow=compatible-local
                    -Wsign-conversion
                    -Wsuggest-override
                    -Wswitch-default
                    -Wswitch-enum
                    -Wundef
                    -Wuninitialized
                    -Wuseless-cast
                    -Wzero-as-null-pointer-constant
            )
        elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            add_compile_options(
                    -Weverything
                    -Wno-c++98-compat
                    -Wno-c++98-compat-pedantic
                    -Wno-unsafe-buffer-usage
                    -Wno-sign-conversion
                    -Wno-old-style-cast
                    -Wno-padded
                    # -Wswitch-enum wants every enumerator listed, while GCC's
                    # -Wswitch-default wants a default label. Clang flags that
                    # combination as redundant; the GCC requirement wins.
                    -Wno-covered-switch-default
            )
        endif ()
    else ()
        # Disable all warnings for quick build
        add_compile_options(-w)
    endif ()
    # workarounds
    add_compile_definitions(__cdecl= __far= __stdcall= cdecl= far=)
endif ()

# Use Chromium-style includes: all paths relative to src root
include_directories(${CMAKE_SOURCE_DIR}/src)

# Testing support
include(GoogleTest)
enable_testing()

add_subdirectory(src/base)
add_subdirectory(src/port)
add_subdirectory(src/sdllib)
add_subdirectory(src/winvq)
add_subdirectory(third_party)
add_subdirectory(src/tech)

add_subdirectory(src/ra)
add_subdirectory(src/td)
