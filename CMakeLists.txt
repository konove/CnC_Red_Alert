cmake_minimum_required(VERSION 3.12)
project(CnCRedAlert)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option to enable strict checks (clang-tidy + extra warnings)
# Disable for quick iteration: cmake -DSTRICT_CHECKS=OFF ..
option(STRICT_CHECKS "Enable clang-tidy and extra compiler warnings" ON)

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
        # Full set of warnings for strict mode
        add_compile_options(-Wall -Wextra -Wpedantic -Wshadow=compatible-local -Wformat=2 -Wconversion -Wsign-conversion -Wcast-align -Wcast-qual -Wundef -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wswitch-enum -Wswitch-default -Wnull-dereference -Woverloaded-virtual -Wnon-virtual-dtor -Wsuggest-override -Wold-style-cast -Wuseless-cast -Wdouble-promotion -Wimplicit-fallthrough=5 -Wzero-as-null-pointer-constant -Wnoexcept -Wnoexcept-type -Wmissing-declarations -Wredundant-decls)
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

add_subdirectory(ra)
add_subdirectory(td)
