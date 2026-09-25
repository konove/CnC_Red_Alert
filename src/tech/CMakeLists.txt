add_library(tech STATIC)

file(GLOB_RECURSE TECH_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
list(FILTER TECH_SOURCES EXCLUDE REGEX "_test\\.cc$")

file(GLOB_RECURSE TECH_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(tech PRIVATE
        ${TECH_SOURCES}
)

target_sources(tech PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${TECH_HEADERS}
)

# tech now holds only the two VQA movie adapters. mixer_vqa_audio.h shows
# AudioMixer (engine/audio) and SDL audio types to whoever includes it, and
# calls AttachExtraCallback/DetachExtraCallback, which live in engine_audio's
# .cc, so engine_audio is PUBLIC. game_file_vqa_io.h includes
# engine/stream/byte_stream.h and friends; game_file_vqa_io.cc uses
# engine/file privately. No public tech header exposes engine/crypto or
# engine/file any more. Both stay PUBLIC anyway: dozens of ra/ and td/
# translation units include tech's headers directly and, until they link the
# engine libraries themselves, reach engine_crypto, engine_file and
# engine_audio only through tech's PUBLIC link.
find_package(SDL2 REQUIRED)
target_link_libraries(tech
        PUBLIC engine_base engine_platform engine_stream engine_crypto engine_file engine_audio absl::strings SDL2::SDL2
        PRIVATE sdllib vqa32 engine_codec
)
