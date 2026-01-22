add_library(tech STATIC)

file(GLOB_RECURSE TECH_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

file(GLOB_RECURSE TECH_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(tech PRIVATE
        ${TECH_SOURCES}
)

target_sources(tech PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${TECH_HEADERS}
)

target_link_libraries(tech sdllib port vqa32 lzo)
target_compile_definitions(tech PUBLIC WINSOCK_IPX=1)
