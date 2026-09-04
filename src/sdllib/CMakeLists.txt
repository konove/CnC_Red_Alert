add_library(sdllib STATIC)

file(GLOB_RECURSE SDLLIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER SDLLIB_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE SDLLIB_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(sdllib PRIVATE
        ${SDLLIB_SOURCES}
)

target_sources(sdllib PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${SDLLIB_HEADERS}
)

find_package(SDL2 REQUIRED)
target_link_libraries(sdllib base SDL2::SDL2 absl::check absl::log absl::strings)
if (WIN32)
    # net_select.cc calls select() through Winsock.
    target_link_libraries(sdllib wsock32)
endif ()

# Tests
include(Testing)
add_gtest(
        NAME sdllib_test
        SOURCES font_test.cc
        LIBS sdllib
)

# optional serial port support
find_library(LIBSERIALPORT_LIBRARIES serialport)
find_path(LIBSERIALPORT_INCLUDE_DIR libserialport.h)
if (LIBSERIALPORT_LIBRARIES AND LIBSERIALPORT_INCLUDE_DIR)
    message("Found libserialport")
    message("[MODEM NOISES]")

    target_compile_definitions(sdllib PRIVATE LIBSERIALPORT)
    target_include_directories(sdllib PRIVATE ${LIBSERIALPORT_INCLUDE_DIR})
    target_link_libraries(sdllib ${LIBSERIALPORT_LIBRARIES})
endif ()
