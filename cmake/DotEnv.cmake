# Parse .env file for development configuration.
# Has NO effect in Release builds.

# The Release check must not return() from this file: apply_dotenv_settings
# below has to exist in every configuration, or the callers fail to configure.
set(DOTENV_FILE "${CMAKE_SOURCE_DIR}/.env")
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build - .env file ignored")
elseif (EXISTS "${DOTENV_FILE}")
    message(STATUS "Loading configuration from .env")
    file(STRINGS "${DOTENV_FILE}" DOTENV_LINES)
    foreach (LINE ${DOTENV_LINES})
        # Skip comments and empty lines
        if (LINE MATCHES "^#" OR LINE STREQUAL "")
            continue()
        endif ()
        # Parse KEY=VALUE
        if (LINE MATCHES "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
            set(ENV_KEY "${CMAKE_MATCH_1}")
            set(ENV_VALUE "${CMAKE_MATCH_2}")
            string(REGEX REPLACE "^[\"']|[\"']$" "" ENV_VALUE "${ENV_VALUE}")
            set("DOTENV_${ENV_KEY}" "${ENV_VALUE}" CACHE STRING "From .env")
        endif ()
    endforeach ()
endif ()

# Apply .env settings to a target.
# Has NO effect in Release builds.
function(apply_dotenv_settings TARGET_NAME)
    if (CMAKE_BUILD_TYPE STREQUAL "Release")
        return()
    endif ()

    if (DEFINED DOTENV_LANGUAGE)
        set(RA_LANGUAGE "${DOTENV_LANGUAGE}" PARENT_SCOPE)
        message(STATUS "${TARGET_NAME}: LANGUAGE=${DOTENV_LANGUAGE} from .env")
    endif ()

    if (DEFINED DOTENV_BUILD_VERSION)
        string(TOLOWER "${DOTENV_BUILD_VERSION}" VERSION_LOWER)
        if (VERSION_LOWER STREQUAL "internal")
            target_compile_definitions(${TARGET_NAME} PRIVATE INTERNAL_VERSION)
            message(STATUS "${TARGET_NAME}: INTERNAL_VERSION from .env")
        elseif (VERSION_LOWER STREQUAL "playtest")
            target_compile_definitions(${TARGET_NAME} PRIVATE PLAYTEST_VERSION)
            message(STATUS "${TARGET_NAME}: PLAYTEST_VERSION from .env")
        endif ()
    endif ()
endfunction()
