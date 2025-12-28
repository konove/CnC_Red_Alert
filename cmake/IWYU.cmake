# IWYU.cmake - Include-What-You-Use integration module
# This module provides functions to enable IWYU checking for CMake targets

option(ENABLE_IWYU "Enable Include-What-You-Use analysis" ON)

if(ENABLE_IWYU)
    # Find the IWYU executable
    find_program(IWYU_PATH
        NAMES include-what-you-use iwyu
        DOC "Path to include-what-you-use executable"
    )

    if(IWYU_PATH)
        message(STATUS "Found include-what-you-use: ${IWYU_PATH}")

        # Check if we're using a compatible compiler
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            # Configure IWYU command with options
            set(IWYU_COMMAND
                "${IWYU_PATH}"
                "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/.iwyu_mappings"
                # "-Xiwyu" "--error_always"
                "-Xiwyu" "--cxx17ns"
                "-Xiwyu" "--no_fwd_decls"
                "-Xiwyu" "--max_line_length=120"
                # "-Xiwyu" "--no_comments"
            )
            message(STATUS "IWYU enabled for all targets")
        else()
            message(WARNING "IWYU requires Clang or GCC. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
            set(IWYU_PATH "")
        endif()
    else()
        message(STATUS "include-what-you-use not found. Install with: sudo apt install iwyu")
        message(STATUS "Continuing without IWYU analysis...")
    endif()
else()
    message(STATUS "IWYU disabled via ENABLE_IWYU=OFF")
endif()

# Function to enable IWYU for a specific target
function(enable_iwyu TARGET_NAME)
    if(ENABLE_IWYU AND IWYU_PATH)
        set_property(TARGET ${TARGET_NAME}
            PROPERTY CXX_INCLUDE_WHAT_YOU_USE "${IWYU_COMMAND}"
        )
        message(STATUS "  IWYU enabled for target: ${TARGET_NAME}")
    endif()
endfunction()

# Function to enable IWYU for all targets in current directory
function(enable_iwyu_for_all_targets)
    if(ENABLE_IWYU AND IWYU_PATH)
        get_property(all_targets DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY BUILDSYSTEM_TARGETS)
        foreach(target ${all_targets})
            enable_iwyu(${target})
        endforeach()
    endif()
endfunction()
