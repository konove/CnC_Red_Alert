add_library(jshell STATIC)

file(GLOB_RECURSE JSHELL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

file(GLOB_RECURSE JSHELL_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(jshell PRIVATE
        ${JSHELL_SOURCES}
)

target_sources(jshell PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}
        FILES ${JSHELL_HEADERS}
)

target_link_libraries(jshell sdllib port)
