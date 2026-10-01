//! Engine kernels written in Rust, and the cxx bridge the C++ engine calls
//! them through. CMake builds the C++ side of the bridge as engine_rs_cxx
//! (src/rust/CMakeLists.txt); C++ includes "engine_rs_cxx/lib.h".

pub mod lcw;

#[cxx::bridge(namespace = "engine_rs")]
mod ffi {
    extern "Rust" {
        /// Decodes an LCW stream leniently; see lcw::uncompress.
        fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize;

        /// Decodes an LCW stream stored in its own output buffer; see
        /// lcw::uncompress_in_place. Aborts if the ranges do not fit.
        fn lcw_uncompress_in_place(
            buffer: &mut [u8],
            source_start: usize,
            source_len: usize,
            dest_len: usize,
        ) -> usize;

        /// Decodes an LCW stream strictly; -1 if it is malformed.
        fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32;

        /// Compresses into one LCW stream; -1 if output is too small.
        fn lcw_compress(input: &[u8], output: &mut [u8]) -> i32;
    }
}

fn lcw_uncompress(source: &[u8], dest: &mut [u8]) -> usize {
    lcw::uncompress(source, dest)
}

fn lcw_uncompress_in_place(
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> usize {
    lcw::uncompress_in_place(buffer, source_start, source_len, dest_len)
}

/// A size as the C++ API's int result; -1 if it does not fit, which no LCW
/// buffer comes near.
fn size_or_minus_one(size: Option<usize>) -> i32 {
    size.and_then(|size| i32::try_from(size).ok()).unwrap_or(-1)
}

fn lcw_uncompress_bounded(source: &[u8], dest: &mut [u8]) -> i32 {
    size_or_minus_one(lcw::uncompress_bounded(source, dest))
}

fn lcw_compress(input: &[u8], output: &mut [u8]) -> i32 {
    size_or_minus_one(lcw::compress(input, output))
}
