# IWYU.cmake - Include-What-You-Use integration module
# This module provides functions to enable IWYU checking for CMake targets

# Set default based on STRICT_CHECKS, but allow independent override
if (NOT DEFINED ENABLE_IWYU)
    if (DEFINED STRICT_CHECKS)
        set(ENABLE_IWYU_DEFAULT ${STRICT_CHECKS})
    else ()
        set(ENABLE_IWYU_DEFAULT ON)
    endif ()
    option(ENABLE_IWYU "Enable Include-What-You-Use analysis" ${ENABLE_IWYU_DEFAULT})
endif ()

if (ENABLE_IWYU)
    # Find the IWYU executable
    find_program(IWYU_PATH
            NAMES include-what-you-use iwyu
            DOC "Path to include-what-you-use executable"
    )

    if (IWYU_PATH)
        message(STATUS "Found include-what-you-use: ${IWYU_PATH}")

        # Check if we're using a compatible compiler
        if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            # Configure IWYU command with options
            set(IWYU_COMMAND
                    "${IWYU_PATH}"
                    "-Xiwyu" "--mapping_file=${CMAKE_SOURCE_DIR}/.iwyu_mappings"
                    "-Xiwyu" "--error"
                    "-Xiwyu" "--cxx17ns"
                    "-Xiwyu" "--no_fwd_decls"
                    "-Xiwyu" "--max_line_length=120"
                    "-Xiwyu" "--no_comments"
            )

            # Add macOS SDK path for proper system header resolution
            if (APPLE)
                execute_process(
                        COMMAND xcrun --sdk macosx --show-sdk-path
                        OUTPUT_VARIABLE MACOS_SDK_PATH
                        OUTPUT_STRIP_TRAILING_WHITESPACE
                        ERROR_QUIET
                )
                if (MACOS_SDK_PATH)
                    list(APPEND IWYU_COMMAND "-isysroot" "${MACOS_SDK_PATH}")
                    message(STATUS "IWYU using macOS SDK: ${MACOS_SDK_PATH}")
                endif ()
            endif ()

            message(STATUS "IWYU enabled for all targets")
        else ()
            message(WARNING "IWYU requires Clang or GCC. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
            set(IWYU_PATH "")
        endif ()
    else ()
        message(STATUS "include-what-you-use not found. Install with: sudo apt install iwyu")
        message(STATUS "Continuing without IWYU analysis...")
    endif ()
else ()
    message(STATUS "IWYU disabled via ENABLE_IWYU=OFF")
endif ()
