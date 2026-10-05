//! Engine kernels written in Rust, and the cxx bridge the C++ engine calls
//! them through. CMake builds the C++ side of the bridge as engine_rs_cxx
//! (src/rust/CMakeLists.txt); C++ includes "engine_rs_cxx/lib.h".
//!
//! The decoders fill a buffer C++ allocated: a vector Rust returned would
//! have to be copied into a C++ one. C++ wraps them into functions that
//! return the vector (engine/codec/lcw.h, lcw_uncompress.h).

pub mod lcw;

#[cxx::bridge(namespace = "engine_rs")]
mod ffi {
    extern "Rust" {
        /// Decodes an LCW stream leniently; see lcw::uncompress_into.
        fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize;

        /// Decodes an LCW stream stored in its own output buffer; see
        /// lcw::uncompress_in_place. Aborts if the ranges do not fit.
        fn lcw_uncompress_in_place(
            buffer: &mut [u8],
            source_start: usize,
            source_len: usize,
            dest_len: usize,
        ) -> usize;

        /// Decodes an LCW stream strictly; see lcw::uncompress_bounded_into.
        /// Returns -1 if it is malformed, as cxx has no Option.
        fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32;

        /// Compresses into one LCW stream; see lcw::compress.
        fn lcw_compress(input: &[u8]) -> Vec<u8>;
    }
}

fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize {
    lcw::uncompress_into(source, dest)
}

fn lcw_uncompress_in_place(
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> usize {
    lcw::uncompress_in_place(buffer, source_start, source_len, dest_len)
}

fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32 {
    // A size that does not fit is -1 too; no LCW buffer comes near it.
    lcw::uncompress_bounded_into(source, dest)
        .and_then(|size| i32::try_from(size).ok())
        .unwrap_or(-1)
}

fn lcw_compress(input: &[u8]) -> Vec<u8> {
    lcw::compress(input)
}
