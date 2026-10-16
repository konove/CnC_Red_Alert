# engine_crypto: src/engine/crypto/ (tools/engine_layout.py).

add_library(engine_crypto STATIC)

file(GLOB ENGINE_CRYPTO_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cc"
)
list(FILTER ENGINE_CRYPTO_SOURCES EXCLUDE REGEX "_test(_util)?\\.cc$")

file(GLOB ENGINE_CRYPTO_HEADERS CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

target_sources(engine_crypto PRIVATE
        ${ENGINE_CRYPTO_SOURCES}
)

target_sources(engine_crypto PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${CMAKE_SOURCE_DIR}/src
        FILES ${ENGINE_CRYPTO_HEADERS}
)

target_link_libraries(engine_crypto
        PUBLIC engine_stream absl::check absl::core_headers
        PRIVATE absl::strings engine_rs_cxx
)

# Tests
include(Testing)
add_gtest(
        NAME engine_crypto_test
        SOURCES blowfish_stream_test.cc blowfish_test.cc key_phrase_hash_test.cc
            pk_stream_test.cc pk_test.cc random_source_test.cc sha1_compress_test.cc
        LIBS engine_crypto base absl::check absl::str_format absl::strings
)
