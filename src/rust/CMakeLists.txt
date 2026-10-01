# engine_rs: the Rust kernels (Cargo.toml), and engine_rs_cxx, the C++ side of
# their cxx bridge (src/lib.rs). C++ links engine_rs_cxx and includes
# "engine_rs_cxx/lib.h" and "rust/cxx.h".

corrosion_import_crate(
        MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml
        CRATES engine_rs
        CRATE_TYPES staticlib
        LOCKED
)

corrosion_add_cxxbridge(engine_rs_cxx
        CRATE engine_rs
        FILES lib.rs
)

# Generated code: the project's checks do not apply to it, and its headers are
# system headers to every includer, so -Weverything stays quiet and
# clang-tidy-cache keeps seeing silent preprocesses.
set_target_properties(engine_rs_cxx PROPERTIES
        CXX_CLANG_TIDY ""
        CXX_INCLUDE_WHAT_YOU_USE ""
        SYSTEM TRUE
)
target_compile_options(engine_rs_cxx PRIVATE $<IF:$<CXX_COMPILER_ID:MSVC>,/w,-w>)

set(ENGINE_RS_MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)

# Cargo runs rustc, clippy-driver and rustfmt from PATH, which need not hold
# the toolchain Corrosion found; put that toolchain first.
cmake_path(GET Rust_CARGO_CACHED PARENT_PATH ENGINE_RS_TOOLCHAIN_BIN)
set(ENGINE_RS_CARGO ${CMAKE_COMMAND} -E env
        --modify PATH=path_list_prepend:${ENGINE_RS_TOOLCHAIN_BIN}
        ${Rust_CARGO_CACHED})

# The crate's own tests, in a target directory of their own so they never wait
# on the lock of the build Corrosion runs.
add_test(NAME engine_rs_cargo_test
        COMMAND ${ENGINE_RS_CARGO} test --manifest-path ${ENGINE_RS_MANIFEST} --locked
            --target-dir ${CMAKE_CURRENT_BINARY_DIR}/cargo-test
)

# Strict builds hold the Rust code to clippy and rustfmt, as they hold C++ to
# clang-tidy. A stamp keeps it from re-running until a source changes.
if (STRICT_CHECKS)
    file(GLOB_RECURSE ENGINE_RS_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.rs)
    set(ENGINE_RS_LINT_STAMP ${CMAKE_CURRENT_BINARY_DIR}/lint.stamp)
    add_custom_command(
            OUTPUT ${ENGINE_RS_LINT_STAMP}
            COMMAND ${ENGINE_RS_CARGO} fmt --manifest-path ${ENGINE_RS_MANIFEST} --check
            COMMAND ${ENGINE_RS_CARGO} clippy --manifest-path ${ENGINE_RS_MANIFEST} --locked
                --all-targets --target-dir ${CMAKE_CURRENT_BINARY_DIR}/cargo-clippy
                -- -D warnings
            COMMAND ${CMAKE_COMMAND} -E touch ${ENGINE_RS_LINT_STAMP}
            DEPENDS ${ENGINE_RS_SOURCES} ${ENGINE_RS_MANIFEST}
                ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.lock
            COMMENT "rustfmt and clippy on src/rust"
            VERBATIM
    )
    add_custom_target(engine_rs_lint ALL DEPENDS ${ENGINE_RS_LINT_STAMP})
endif ()
