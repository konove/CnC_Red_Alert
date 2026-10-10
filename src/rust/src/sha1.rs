//! SHA-1 block compression: the part of the hash that runs once per 64 bytes
//! of input. The C++ SHAEngine handles padding and partial blocks.
//!
//! This is the one kernel module that is not `forbid(unsafe_code)`: the x86
//! SHA extensions are reached through `core::arch`, and calling a function
//! compiled for CPU features the build does not assume is `unsafe`. That one
//! call, in `compress_hardware`, is the module's only unsafe code.
#![deny(unsafe_code)]

/// The five 32-bit words SHA-1 accumulates, A first.
pub type State = [u32; 5];

pub const BLOCK_SIZE: usize = 64;

/// Compresses the whole blocks of `blocks` into `state`; a trailing partial
/// block is ignored. Uses the x86 SHA extensions when the CPU has them, which
/// are several times faster than the portable code; the result is identical.
pub fn compress(state: &mut State, blocks: &[u8]) {
    if hardware_available() {
        compress_hardware(state, blocks);
    } else {
        compress_portable(state, blocks);
    }
}

/// As `compress`, in portable code.
pub fn compress_portable(state: &mut State, blocks: &[u8]) {
    const ROUND_CONSTANTS: [u32; 4] = [0x5a82_7999, 0x6ed9_eba1, 0x8f1b_bcdc, 0xca62_c1d6];
    for block in blocks.as_chunks::<BLOCK_SIZE>().0 {
        // The message schedule, of which a round reads only the last 16
        // words: word `i` lives at `i % 16`.
        let mut schedule = [0u32; 16];
        for (word, bytes) in schedule.iter_mut().zip(block.as_chunks::<4>().0) {
            *word = u32::from_be_bytes(*bytes);
        }
        let [mut a, mut b, mut c, mut d, mut e] = *state;
        for round in 0..80 {
            let word = if round < 16 {
                schedule[round]
            } else {
                let word = (schedule[(round + 13) % 16]
                    ^ schedule[(round + 8) % 16]
                    ^ schedule[(round + 2) % 16]
                    ^ schedule[round % 16])
                    .rotate_left(1);
                schedule[round % 16] = word;
                word
            };
            let function = match round / 20 {
                0 => d ^ (b & (c ^ d)),
                2 => (b & c) | (d & (b | c)),
                _ => b ^ c ^ d,
            };
            let next = a
                .rotate_left(5)
                .wrapping_add(function)
                .wrapping_add(e)
                .wrapping_add(word)
                .wrapping_add(ROUND_CONSTANTS[round / 20]);
            e = d;
            d = c;
            c = b.rotate_left(30);
            b = a;
            a = next;
        }
        for (sum, word) in state.iter_mut().zip([a, b, c, d, e]) {
            *sum = sum.wrapping_add(word);
        }
    }
}

/// Returns whether `compress_hardware` can run on this CPU.
pub fn hardware_available() -> bool {
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        // The standard library caches what CPUID said.
        std::arch::is_x86_feature_detected!("sha")
            && std::arch::is_x86_feature_detected!("sse4.1")
            && std::arch::is_x86_feature_detected!("ssse3")
    }
    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
    {
        false
    }
}

/// As `compress`, with the x86 SHA extensions.
///
/// Panics unless `hardware_available()`: that is a caller bug.
pub fn compress_hardware(state: &mut State, blocks: &[u8]) {
    assert!(hardware_available(), "no SHA extensions on this CPU");
    #[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
    {
        // SAFETY: the CPU has every feature `x86::compress` is compiled for,
        // as the assertion above has just checked.
        #[allow(unsafe_code)]
        unsafe {
            x86::compress(state, blocks);
        }
    }
    #[cfg(not(any(target_arch = "x86", target_arch = "x86_64")))]
    {
        let _ = (state, blocks);
    }
}

#[cfg(any(target_arch = "x86", target_arch = "x86_64"))]
mod x86 {
    #[cfg(target_arch = "x86")]
    use std::arch::x86::*;
    #[cfg(target_arch = "x86_64")]
    use std::arch::x86_64::*;

    use super::{BLOCK_SIZE, State};

    /// Reinterprets a word's bits for the intrinsics, which take `i32`.
    fn lane(word: u32) -> i32 {
        word.cast_signed()
    }

    /// Runs four rounds with round function `function`, 0..=3. The selector
    /// of sha1rnds4 must be an immediate, hence one call per value.
    #[target_feature(enable = "sha,sse2,ssse3,sse4.1")]
    fn four_rounds(abcd: __m128i, e: __m128i, function: usize) -> __m128i {
        match function {
            0 => _mm_sha1rnds4_epu32::<0>(abcd, e),
            1 => _mm_sha1rnds4_epu32::<1>(abcd, e),
            2 => _mm_sha1rnds4_epu32::<2>(abcd, e),
            _ => _mm_sha1rnds4_epu32::<3>(abcd, e),
        }
    }

    /// Intel's reference sequence for the SHA extensions. `abcd` holds A..D
    /// with A in the top lane; `e` alternates between two registers from one
    /// group of four rounds to the next, and `msg` holds the four
    /// message-schedule registers, which rotate through the groups.
    #[target_feature(enable = "sha,sse2,ssse3,sse4.1")]
    pub fn compress(state: &mut State, blocks: &[u8]) {
        let [a, b, c, d, e] = *state;
        let mut abcd = _mm_set_epi32(lane(a), lane(b), lane(c), lane(d));
        let mut e = [_mm_set_epi32(lane(e), 0, 0, 0), _mm_setzero_si128()];
        for block in blocks.as_chunks::<BLOCK_SIZE>().0 {
            let abcd_save = abcd;
            let e_save = e[0];
            // SHA-1 reads its input big-endian, the first word of each
            // sixteen bytes in the top lane.
            let (words, _) = block.as_chunks::<4>();
            let word = |index: usize| lane(u32::from_be_bytes(words[index]));
            let mut msg = [0, 4, 8, 12]
                .map(|at| _mm_set_epi32(word(at), word(at + 1), word(at + 2), word(at + 3)));
            // Group `group` runs rounds 4 * group to 4 * group + 3, and also
            // advances the message schedule for the groups after it; the
            // conditions skip exactly the steps whose results no later group
            // reads. A macro and not a loop: as a loop the compiler unrolled
            // only the first three groups and kept the registers of the rest
            // in memory, a third slower.
            macro_rules! groups {
                ($($group:literal)*) => {$({
                    const GROUP: usize = $group;
                    let current = msg[GROUP % 4];
                    let this_e = if GROUP == 0 {
                        _mm_add_epi32(e[0], current)
                    } else {
                        _mm_sha1nexte_epu32(e[GROUP % 2], current)
                    };
                    e[(GROUP + 1) % 2] = abcd;
                    if GROUP >= 3 && GROUP <= 18 {
                        msg[(GROUP + 1) % 4] = _mm_sha1msg2_epu32(msg[(GROUP + 1) % 4], current);
                    }
                    abcd = four_rounds(abcd, this_e, GROUP / 5);
                    if GROUP >= 1 && GROUP <= 16 {
                        msg[(GROUP + 3) % 4] = _mm_sha1msg1_epu32(msg[(GROUP + 3) % 4], current);
                    }
                    if GROUP >= 2 && GROUP <= 17 {
                        msg[(GROUP + 2) % 4] = _mm_xor_si128(msg[(GROUP + 2) % 4], current);
                    }
                })*};
            }
            groups!(0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19);
            // Round 79 leaves E in e[0], which still needs the saved E added
            // the way sha1nexte does it.
            e[0] = _mm_sha1nexte_epu32(e[0], e_save);
            abcd = _mm_add_epi32(abcd, abcd_save);
        }
        *state = [
            _mm_extract_epi32::<3>(abcd),
            _mm_extract_epi32::<2>(abcd),
            _mm_extract_epi32::<1>(abcd),
            _mm_extract_epi32::<0>(abcd),
            _mm_extract_epi32::<3>(e[0]),
        ]
        .map(i32::cast_unsigned);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    const INITIAL: State = [
        0x6745_2301,
        0xefcd_ab89,
        0x98ba_dcfe,
        0x1032_5476,
        0xc3d2_e1f0,
    ];

    /// The padded single block of a message shorter than 56 bytes.
    fn padded(message: &[u8]) -> [u8; BLOCK_SIZE] {
        let mut block = [0; BLOCK_SIZE];
        block[..message.len()].copy_from_slice(message);
        block[message.len()] = 0x80;
        block[56..].copy_from_slice(&(message.len() as u64 * 8).to_be_bytes());
        block
    }

    fn digest(compress: fn(&mut State, &[u8]), message: &[u8]) -> State {
        let mut state = INITIAL;
        compress(&mut state, &padded(message));
        state
    }

    /// Every implementation this CPU can run.
    fn implementations() -> Vec<fn(&mut State, &[u8])> {
        let mut all: Vec<fn(&mut State, &[u8])> = vec![compress, compress_portable];
        if hardware_available() {
            all.push(compress_hardware);
        }
        all
    }

    #[test]
    fn known_digests() {
        for compress in implementations() {
            assert_eq!(
                digest(compress, b""),
                [
                    0xda39_a3ee,
                    0x5e6b_4b0d,
                    0x3255_bfef,
                    0x9560_1890,
                    0xafd8_0709
                ]
            );
            assert_eq!(
                digest(compress, b"abc"),
                [
                    0xa999_3e36,
                    0x4706_816a,
                    0xba3e_2571,
                    0x7850_c26c,
                    0x9cd0_d89d
                ]
            );
        }
    }

    #[test]
    fn several_blocks_chain() {
        // FIPS 180-1's two-block message.
        let message = b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        let mut blocks = [0u8; 2 * BLOCK_SIZE];
        blocks[..message.len()].copy_from_slice(message);
        blocks[message.len()] = 0x80;
        blocks[120..].copy_from_slice(&(message.len() as u64 * 8).to_be_bytes());
        for compress in implementations() {
            let mut state = INITIAL;
            compress(&mut state, &blocks);
            assert_eq!(
                state,
                [
                    0x8498_3e44,
                    0x1c3b_d26e,
                    0xbaae_4aa1,
                    0xf951_29e5,
                    0xe546_70f1
                ]
            );
            // One block at a time comes to the same.
            let mut split = INITIAL;
            compress(&mut split, &blocks[..BLOCK_SIZE]);
            compress(&mut split, &blocks[BLOCK_SIZE..]);
            assert_eq!(split, state);
        }
    }

    #[test]
    fn hardware_matches_portable() {
        if !hardware_available() {
            return;
        }
        let mut seed = 1u32;
        let mut blocks = [0u8; 9 * BLOCK_SIZE];
        for byte in &mut blocks {
            seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
            *byte = (seed >> 24) as u8;
        }
        let (mut portable, mut hardware) = (INITIAL, INITIAL);
        compress_portable(&mut portable, &blocks);
        compress_hardware(&mut hardware, &blocks);
        assert_eq!(hardware, portable);
    }

    #[test]
    fn a_partial_block_is_ignored() {
        for compress in implementations() {
            let mut state = INITIAL;
            compress(&mut state, &[0x55; BLOCK_SIZE - 1]);
            assert_eq!(state, INITIAL);
            compress(&mut state, &[]);
            assert_eq!(state, INITIAL);
        }
    }
}
