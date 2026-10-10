//! SHA-1 block compression: the part of the hash that runs once per 64 bytes
//! of input. The C++ SHAEngine handles padding and partial blocks.
//!
//! The compression itself is the `sha1` crate's, which uses the SHA
//! extensions of x86 and aarch64 CPUs that have them and portable code
//! elsewhere; the result is the same either way.
#![forbid(unsafe_code)]

use sha1::Digest;

/// The five 32-bit words SHA-1 accumulates, A first.
pub type State = [u32; 5];

pub const BLOCK_SIZE: usize = 64;

/// Compresses the whole blocks of `blocks` into `state`; a trailing partial
/// block is ignored.
pub fn compress(state: &mut State, blocks: &[u8]) {
    sha1::block_api::compress(state, blocks.as_chunks::<BLOCK_SIZE>().0);
}

/// Returns the SHA-1 digest of `message`. For the few places in the crate
/// that hash a short message whole; the engine's streams pad in C++.
pub fn digest(message: &[u8]) -> [u8; 20] {
    sha1::Sha1::digest(message).into()
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

    #[test]
    fn known_digests() {
        let mut state = INITIAL;
        compress(&mut state, &padded(b""));
        assert_eq!(
            state,
            [
                0xda39_a3ee,
                0x5e6b_4b0d,
                0x3255_bfef,
                0x9560_1890,
                0xafd8_0709
            ]
        );
        let mut state = INITIAL;
        compress(&mut state, &padded(b"abc"));
        assert_eq!(
            state,
            [
                0xa999_3e36,
                0x4706_816a,
                0xba3e_2571,
                0x7850_c26c,
                0x9cd0_d89d
            ]
        );
    }

    #[test]
    fn several_blocks_chain() {
        // FIPS 180-1's two-block message.
        let message = b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
        let mut blocks = [0u8; 2 * BLOCK_SIZE];
        blocks[..message.len()].copy_from_slice(message);
        blocks[message.len()] = 0x80;
        blocks[120..].copy_from_slice(&(message.len() as u64 * 8).to_be_bytes());
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

    #[test]
    fn a_partial_block_is_ignored() {
        let mut state = INITIAL;
        compress(&mut state, &[0x55; BLOCK_SIZE - 1]);
        assert_eq!(state, INITIAL);
        compress(&mut state, &[]);
        assert_eq!(state, INITIAL);
        // The whole blocks before it are still compressed.
        let mut whole = INITIAL;
        compress(&mut whole, &[0x55; BLOCK_SIZE]);
        compress(&mut state, &[0x55; BLOCK_SIZE + 9]);
        assert_eq!(state, whole);
    }

    #[test]
    fn digests_whole_messages() {
        let hex = |digest: [u8; 20]| digest.map(|byte| format!("{byte:02x}")).concat();
        assert_eq!(hex(digest(b"")), "da39a3ee5e6b4b0d3255bfef95601890afd80709");
        assert_eq!(
            hex(digest(b"abc")),
            "a9993e364706816aba3e25717850c26c9cd0d89d"
        );
    }
}
