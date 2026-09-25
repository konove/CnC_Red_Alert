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

# The fading-table builder, the per-frame window blit and the LCW decoder loop
# over every palette entry, pixel or compressed byte.
optimize_in_debug(fading_table.cc pixel_buffer.cc lcw_uncompress.cc)
if (WIN32)
    # net_select.cc calls select() through Winsock.
    target_link_libraries(sdllib wsock32)
endif ()

# Tests
include(Testing)
add_gtest(
        NAME sdllib_test
        SOURCES font_test.cc xor_delta_test.cc aud_decoder_test.cc file_system_test.cc compressed_block_test.cc lcw_uncompress_test.cc
        LIBS sdllib
)

# Compiles keyboard.cc directly and stubs its two hooks, so the key buffer can
# be tested without linking the event loop and mouse cursor code.
add_gtest(
        NAME sdllib_keyboard_test
        SOURCES keyboard_test.cc keyboard.cc
        LIBS base SDL2::SDL2 absl::strings absl::check absl::log
)

add_gtest(
        NAME sdllib_pixel_buffer_test
        SOURCES pixel_buffer_test.cc stamp_test.cc
        LIBS sdllib
)

add_gtest(
        NAME sdllib_display_test
        SOURCES display_test.cc
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

# Exercises the game's key-frame/delta chain using the real bounded LCW decoder.
add_gtest(
        NAME keyframe_test
        SOURCES keyframe_test.cc ../ra/2keyfram.cc
        LIBS sdllib
)
