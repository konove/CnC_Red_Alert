cmake_minimum_required(VERSION 3.14)
project(CnCRedAlert)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

# Fetch Abseil for CHECK/DCHECK macros
include(FetchContent)
FetchContent_Declare(
        abseil-cpp
        GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
        GIT_TAG 20240722.0
)
set(ABSL_PROPAGATE_CXX_STD ON)
set(ABSL_BUILD_TESTING OFF)
FetchContent_MakeAvailable(abseil-cpp)

# Option to enable strict checks (clang-tidy + extra warnings)
# Disable for quick iteration: cmake -DSTRICT_CHECKS=OFF ..
option(STRICT_CHECKS "Enable clang-tidy and extra compiler warnings" ON)

# Option to enable AddressSanitizer for debugging memory issues
option(ENABLE_ASAN "Enable AddressSanitizer for memory debugging" OFF)

if (ENABLE_ASAN)
    message(STATUS "AddressSanitizer enabled")
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address)
endif ()

if (STRICT_CHECKS)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    message(STATUS "Strict checks enabled (clang-tidy + warnings)")
else ()
    message(STATUS "Strict checks disabled (quick build mode)")
endif ()

# Include What You Use integration (automatically enabled/disabled by STRICT_CHECKS)
include(cmake/IWYU.cmake)

if (ENABLE_IWYU AND IWYU_PATH)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND}")
endif ()

if (MSVC)
    add_compile_options(
            /wd4996 # yes, this code is old and uses "unsafe" functions
            /wd4068 /wd4081 # old #pragmas for another compiler
            /wd4661 # missing template definitions, 10000s of these
    )
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
            add_compile_options(-Weverything)
        endif ()
    else ()
        # Disable all warnings for quick build
        add_compile_options(-w)
    endif ()
    # workarounds
    add_compile_definitions(__cdecl= __far= __stdcall= cdecl= far=)
endif ()

add_compile_definitions(WIN32) # hopefully easier than DOS

# Use Chromium-style includes: all paths relative to project root
include_directories(${CMAKE_SOURCE_DIR})

add_subdirectory(port)
add_subdirectory(sdllib)
add_subdirectory(winvq)
add_subdirectory(jshell)
add_subdirectory(tech)

add_subdirectory(ra)
add_subdirectory(td)
