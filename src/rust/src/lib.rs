//! Engine kernels written in Rust, and the cxx bridge the C++ engine calls
//! them through. CMake builds the C++ side of the bridge as engine_rs_cxx
//! (src/rust/CMakeLists.txt); C++ includes "engine_rs_cxx/lib.h".
//!
//! The decoders fill a buffer C++ allocated: a vector Rust returned would
//! have to be copied into a C++ one. C++ wraps them into functions that
//! return the vector (engine/codec/lcw.h, lcw_uncompress.h).

pub mod adpcm;
pub mod lcw;
pub mod sha1;
pub mod vq;
pub mod westwood_audio;
pub mod xor_delta;

#[cxx::bridge(namespace = "engine_rs")]
mod ffi {
    /// adpcm::State, as C++ sees it.
    struct ImaAdpcmState {
        predictor: i16,
        step_index: u8,
    }

    extern "Rust" {
        /// Decodes IMA ADPCM into two samples per byte; see
        /// adpcm::decode_samples.
        fn ima_adpcm_decode_samples(state: &mut ImaAdpcmState, source: &[u8], dest: &mut [i16]);

        /// Decodes IMA ADPCM into little-endian sample bytes; see
        /// adpcm::decode_bytes.
        fn ima_adpcm_decode_bytes(
            state: &mut ImaAdpcmState,
            source: &[u8],
            dest: &mut [u8],
        ) -> bool;

        /// Decodes IMA ADPCM stored in its own output buffer; see
        /// adpcm::decode_bytes_in_place. Aborts if the ranges do not fit.
        fn ima_adpcm_decode_bytes_in_place(
            state: &mut ImaAdpcmState,
            buffer: &mut [u8],
            source_start: usize,
            source_len: usize,
            dest_len: usize,
        ) -> bool;

        /// Returns the number of samples a Westwood 8-bit block decodes to,
        /// or -1 if it is malformed, as cxx has no Option; see
        /// westwood_audio::decoded_len.
        fn westwood_audio_decoded_len(block: &[u8]) -> isize;

        /// Decodes a Westwood 8-bit block into `dest`, which must be the
        /// length westwood_audio_decoded_len gave. Returns whether it fitted.
        fn westwood_audio_decode(block: &[u8], dest: &mut [u8]) -> bool;

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

        /// Compresses whole 64-byte blocks into a SHA-1 state with the
        /// fastest code the CPU runs; see sha1::compress.
        fn sha1_compress(state: &mut [u32; 5], blocks: &[u8]);

        /// As sha1_compress, in portable code.
        fn sha1_compress_portable(state: &mut [u32; 5], blocks: &[u8]);

        /// Returns whether sha1_compress_hardware can run on this CPU.
        fn sha1_hardware_available() -> bool;

        /// As sha1_compress, with the x86 SHA extensions. Aborts unless
        /// sha1_hardware_available().
        fn sha1_compress_hardware(state: &mut [u32; 5], blocks: &[u8]);

        /// Decodes a VQ movie frame; see vq::decode_frame.
        #[allow(clippy::too_many_arguments)]
        fn vq_decode_frame(
            codebook: &[u8],
            pointers: &[u8],
            buffer: &mut [u8],
            blocks_per_row: usize,
            block_rows: usize,
            stride: usize,
            block_height: usize,
            solid_marker: u8,
        );

        /// Applies an XOR delta to the rows of an image; see
        /// xor_delta::apply.
        fn xor_delta_apply(
            target: &mut [u8],
            delta: &[u8],
            width: usize,
            stride: usize,
            copy: bool,
        );
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

fn xor_delta_apply(target: &mut [u8], delta: &[u8], width: usize, stride: usize, copy: bool) {
    xor_delta::apply(target, delta, width, stride, copy);
}

/// Runs `decode` on the bridge's copy of the state and stores the result.
fn with_adpcm_state<R>(
    state: &mut ffi::ImaAdpcmState,
    decode: impl FnOnce(&mut adpcm::State) -> R,
) -> R {
    let mut inner = adpcm::State {
        predictor: state.predictor,
        step_index: state.step_index,
    };
    let result = decode(&mut inner);
    state.predictor = inner.predictor;
    state.step_index = inner.step_index;
    result
}

fn ima_adpcm_decode_samples(state: &mut ffi::ImaAdpcmState, source: &[u8], dest: &mut [i16]) {
    with_adpcm_state(state, |state| adpcm::decode_samples(state, source, dest));
}

fn ima_adpcm_decode_bytes(state: &mut ffi::ImaAdpcmState, source: &[u8], dest: &mut [u8]) -> bool {
    with_adpcm_state(state, |state| adpcm::decode_bytes(state, source, dest))
}

fn ima_adpcm_decode_bytes_in_place(
    state: &mut ffi::ImaAdpcmState,
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> bool {
    with_adpcm_state(state, |state| {
        adpcm::decode_bytes_in_place(state, buffer, source_start, source_len, dest_len)
    })
}

fn westwood_audio_decoded_len(block: &[u8]) -> isize {
    // A slice is never longer than isize::MAX, and a block decodes to at most
    // 64 samples a byte, so only an absurd block could fail to fit.
    westwood_audio::decoded_len(block)
        .and_then(|len| isize::try_from(len).ok())
        .unwrap_or(-1)
}

fn westwood_audio_decode(block: &[u8], dest: &mut [u8]) -> bool {
    westwood_audio::decode_into(block, dest) == Some(dest.len())
}

#[allow(clippy::too_many_arguments)]
fn vq_decode_frame(
    codebook: &[u8],
    pointers: &[u8],
    buffer: &mut [u8],
    blocks_per_row: usize,
    block_rows: usize,
    stride: usize,
    block_height: usize,
    solid_marker: u8,
) {
    vq::decode_frame(
        codebook,
        pointers,
        buffer,
        blocks_per_row,
        block_rows,
        stride,
        block_height,
        solid_marker,
    );
}

fn sha1_compress(state: &mut [u32; 5], blocks: &[u8]) {
    sha1::compress(state, blocks);
}

fn sha1_compress_portable(state: &mut [u32; 5], blocks: &[u8]) {
    sha1::compress_portable(state, blocks);
}

fn sha1_hardware_available() -> bool {
    sha1::hardware_available()
}

fn sha1_compress_hardware(state: &mut [u32; 5], blocks: &[u8]) {
    sha1::compress_hardware(state, blocks);
}
