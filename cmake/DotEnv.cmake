# Parse .env file for development configuration.
# Has NO effect in Release builds.

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release build - .env file ignored")
    return()
endif ()

set(DOTENV_FILE "${CMAKE_SOURCE_DIR}/.env")
if (EXISTS "${DOTENV_FILE}")
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

    if (DEFINED DOTENV_BUILD_VERSION)
        string(TOLOWER "${DOTENV_BUILD_VERSION}" VERSION_LOWER)
        if (VERSION_LOWER STREQUAL "internal")
            target_compile_definitions(${TARGET_NAME} PRIVATE INTERNAL_VERSION)
            # TD uses CHEAT_KEYS/SCENARIO_EDITOR directly (RA uses config::kScenarioEditorEnabled)
            target_compile_definitions(${TARGET_NAME} PRIVATE CHEAT_KEYS)
            if (TARGET_NAME STREQUAL "tdsdl")
                target_compile_definitions(${TARGET_NAME} PRIVATE SCENARIO_EDITOR)
            endif ()
            message(STATUS "${TARGET_NAME}: INTERNAL_VERSION from .env")
        elseif (VERSION_LOWER STREQUAL "playtest")
            target_compile_definitions(${TARGET_NAME} PRIVATE PLAYTEST_VERSION)
            # TD: playtest gets limited cheats
            target_compile_definitions(${TARGET_NAME} PRIVATE VIRGIN_CHEAT_KEYS)
            message(STATUS "${TARGET_NAME}: PLAYTEST_VERSION from .env")
        endif ()
    endif ()
endfunction()
