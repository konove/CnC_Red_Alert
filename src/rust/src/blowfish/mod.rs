//! The Blowfish block cipher, which encrypts the index of a MIX archive and
//! Red Alert's saved games. Blocks are 8 bytes, read as two big-endian
//! words; a key is 1 to 56 bytes.
//!
//! Setting up a key costs as much as encrypting about 4 KiB: the key is
//! mixed into the sub-key table, and then the cipher is run over its own
//! tables until every entry has been replaced.
#![forbid(unsafe_code)]

mod tables;

/// Feistel rounds; 16 is the standard count.
const ROUNDS: usize = 16;

pub const BLOCK_SIZE: usize = 8;
pub const MAX_KEY_LEN: usize = 56;

/// The tables a key expands to.
pub struct Blowfish {
    /// The sub-keys in the order encryption applies them, and reversed, the
    /// order decryption does.
    p_encrypt: [u32; ROUNDS + 2],
    p_decrypt: [u32; ROUNDS + 2],
    s: [[u32; 256]; 4],
}

impl Blowfish {
    /// Expands `key`, or returns None if it is empty or longer than
    /// MAX_KEY_LEN.
    pub fn new(key: &[u8]) -> Option<Blowfish> {
        let mut cipher = Blowfish {
            p_encrypt: tables::P_INIT,
            p_decrypt: tables::P_INIT,
            s: tables::S_INIT,
        };
        cipher.set_key(key).then_some(cipher)
    }

    /// Replaces the key with `key`. Returns false, leaving the tables in no
    /// usable state, if it is empty or longer than MAX_KEY_LEN.
    pub fn set_key(&mut self, key: &[u8]) -> bool {
        if key.is_empty() || key.len() > MAX_KEY_LEN {
            return false;
        }
        self.p_encrypt = tables::P_INIT;
        self.s = tables::S_INIT;
        // The key, repeated as often as it takes, is XORed onto the sub-keys
        // a big-endian word at a time.
        let mut key_bytes = key.iter().cycle();
        for sub_key in &mut self.p_encrypt {
            let word = [(); 4].map(|()| *key_bytes.next().expect("the key is not empty"));
            *sub_key ^= u32::from_be_bytes(word);
        }
        // Every table entry is then replaced by the cipher's own output,
        // which is fed back in as the next input. The tables change while
        // they are in use, which is the point.
        let mut block = (0, 0);
        for index in (0..ROUNDS + 2).step_by(2) {
            block = crypt(&self.p_encrypt, &self.s, block);
            self.p_encrypt[index] = block.0;
            self.p_encrypt[index + 1] = block.1;
        }
        for sbox in 0..self.s.len() {
            for index in (0..256).step_by(2) {
                block = crypt(&self.p_encrypt, &self.s, block);
                self.s[sbox][index] = block.0;
                self.s[sbox][index + 1] = block.1;
            }
        }
        self.p_decrypt = self.p_encrypt;
        self.p_decrypt.reverse();
        true
    }

    /// Encrypts the whole 8-byte blocks of `source` into `dest` and copies a
    /// trailing partial block as it is.
    ///
    /// Panics if `dest` is shorter than `source`: that is a caller bug.
    pub fn encrypt(&self, source: &[u8], dest: &mut [u8]) {
        process(&self.p_encrypt, &self.s, source, dest);
    }

    /// As `encrypt`, in the buffer the data is in.
    pub fn encrypt_in_place(&self, data: &mut [u8]) {
        process_in_place(&self.p_encrypt, &self.s, data);
    }

    /// Decrypts the whole 8-byte blocks of `source` into `dest` and copies a
    /// trailing partial block as it is.
    ///
    /// Panics if `dest` is shorter than `source`: that is a caller bug.
    pub fn decrypt(&self, source: &[u8], dest: &mut [u8]) {
        process(&self.p_decrypt, &self.s, source, dest);
    }

    /// As `decrypt`, in the buffer the data is in.
    pub fn decrypt_in_place(&self, data: &mut [u8]) {
        process_in_place(&self.p_decrypt, &self.s, data);
    }
}

impl Drop for Blowfish {
    /// Leaves no key material behind. The black box keeps the compiler from
    /// dropping the writes as dead.
    fn drop(&mut self) {
        self.p_encrypt.fill(0);
        self.p_decrypt.fill(0);
        self.s.as_flattened_mut().fill(0);
        std::hint::black_box(self);
    }
}

/// The round function: each byte of `half` picks an entry of one S-box.
fn feistel(s: &[[u32; 256]; 4], half: u32) -> u32 {
    // A shift a byte, the top one first. `to_be_bytes` would swap the word
    // around first, which adds to the chain from one round to the next.
    let byte = |shift: u32| usize::from((half >> shift) as u8);
    (s[0][byte(24)].wrapping_add(s[1][byte(16)]) ^ s[2][byte(8)]).wrapping_add(s[3][byte(0)])
}

/// Runs one block, as its left and right words, through the rounds with the
/// sub-keys in the order `p` gives them: encryption or decryption.
fn crypt(
    p: &[u32; ROUNDS + 2],
    s: &[[u32; 256]; 4],
    (mut left, mut right): (u32, u32),
) -> (u32, u32) {
    // Two rounds a step, which saves exchanging the halves after each.
    for pair in p[..ROUNDS].as_chunks::<2>().0 {
        left ^= pair[0];
        right ^= feistel(s, left) ^ pair[1];
        left ^= feistel(s, right);
    }
    // The halves come out exchanged, which undoes the last round's exchange.
    (right ^ p[ROUNDS + 1], left ^ p[ROUNDS])
}

fn crypt_bytes(
    p: &[u32; ROUNDS + 2],
    s: &[[u32; 256]; 4],
    block: &[u8; BLOCK_SIZE],
) -> [u8; BLOCK_SIZE] {
    let (left, right) = block.split_at(4);
    let words = (
        u32::from_be_bytes(left.try_into().expect("half a block")),
        u32::from_be_bytes(right.try_into().expect("half a block")),
    );
    let (left, right) = crypt(p, s, words);
    let mut out = [0; BLOCK_SIZE];
    out[..4].copy_from_slice(&left.to_be_bytes());
    out[4..].copy_from_slice(&right.to_be_bytes());
    out
}

fn process(p: &[u32; ROUNDS + 2], s: &[[u32; 256]; 4], source: &[u8], dest: &mut [u8]) {
    let dest = &mut dest[..source.len()];
    let (blocks, tail) = source.as_chunks::<BLOCK_SIZE>();
    let (out_blocks, out_tail) = dest.as_chunks_mut::<BLOCK_SIZE>();
    for (block, out) in blocks.iter().zip(out_blocks) {
        *out = crypt_bytes(p, s, block);
    }
    out_tail.copy_from_slice(tail);
}

fn process_in_place(p: &[u32; ROUNDS + 2], s: &[[u32; 256]; 4], data: &mut [u8]) {
    for block in data.as_chunks_mut::<BLOCK_SIZE>().0 {
        *block = crypt_bytes(p, s, block);
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    /// Eric Young's test vectors, as distributed with the cipher: key,
    /// plain text, cipher text.
    const VECTORS: [(u64, u64, u64); 6] = [
        (0x0000000000000000, 0x0000000000000000, 0x4ef997456198dd78),
        (0xffffffffffffffff, 0xffffffffffffffff, 0x51866fd5b85ecb8a),
        (0x3000000000000000, 0x1000000000000001, 0x7d856f9a613063f2),
        (0x1111111111111111, 0x1111111111111111, 0x2466dd878b963c9d),
        (0x0123456789abcdef, 0x1111111111111111, 0x61f9c3802281b096),
        (0xfedcba9876543210, 0x0123456789abcdef, 0x0aceab0fc6a0a28d),
    ];

    #[test]
    fn reference_vectors() {
        for (key, plain, cipher) in VECTORS {
            let blowfish = Blowfish::new(&key.to_be_bytes()).expect("a key");
            let mut out = [0; 8];
            blowfish.encrypt(&plain.to_be_bytes(), &mut out);
            assert_eq!(out, cipher.to_be_bytes(), "key {key:016x}");
            blowfish.decrypt(&cipher.to_be_bytes(), &mut out);
            assert_eq!(out, plain.to_be_bytes(), "key {key:016x}");
        }
    }

    #[test]
    fn variable_key_lengths() {
        // From the same vectors' set-key test: the first n bytes of one key
        // encrypting one block.
        let key = [
            0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5, 0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b, 0x3c, 0x2d,
            0x1e, 0x0f, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
        ];
        let plain = 0xfedcba9876543210u64.to_be_bytes();
        for (len, cipher) in [
            (1, 0xf9ad597c49db005eu64),
            (2, 0xe91d21c1d961a6d6),
            (8, 0xe87a244e2cc85e82),
            (16, 0x93142887ee3be15c),
            (24, 0x05044b62fa52d080),
        ] {
            let mut out = [0; 8];
            Blowfish::new(&key[..len])
                .expect("a key")
                .encrypt(&plain, &mut out);
            assert_eq!(out, cipher.to_be_bytes(), "{len}-byte key");
        }
    }

    #[test]
    fn rejects_keys_of_no_or_too_many_bytes() {
        assert!(Blowfish::new(&[]).is_none());
        assert!(Blowfish::new(&[1; MAX_KEY_LEN]).is_some());
        assert!(Blowfish::new(&[1; MAX_KEY_LEN + 1]).is_none());
    }

    #[test]
    fn a_new_key_replaces_the_old_one() {
        let (key, plain, cipher) = VECTORS[4];
        let mut blowfish = Blowfish::new(b"some other key").expect("a key");
        assert!(blowfish.set_key(&key.to_be_bytes()));
        let mut out = [0; 8];
        blowfish.encrypt(&plain.to_be_bytes(), &mut out);
        assert_eq!(out, cipher.to_be_bytes());
        assert!(!blowfish.set_key(&[]));
    }

    #[test]
    fn a_partial_block_passes_through() {
        let blowfish = Blowfish::new(b"key").expect("a key");
        let plain: Vec<u8> = (0..19).collect();
        let mut cipher = [0xee; 21];
        blowfish.encrypt(&plain, &mut cipher);
        assert_ne!(cipher[..16], plain[..16]);
        assert_eq!(cipher[16..], [16, 17, 18, 0xee, 0xee]);
        let mut round_trip = [0; 19];
        blowfish.decrypt(&cipher[..19], &mut round_trip);
        assert_eq!(round_trip[..], plain[..]);
    }

    #[test]
    fn in_place_matches_separate_buffers() {
        let blowfish = Blowfish::new(b"another key").expect("a key");
        let plain: Vec<u8> = (0..43).map(|byte| byte * 5).collect();
        let mut want = vec![0; plain.len()];
        blowfish.encrypt(&plain, &mut want);
        let mut data = plain.clone();
        blowfish.encrypt_in_place(&mut data);
        assert_eq!(data, want);
        blowfish.decrypt_in_place(&mut data);
        assert_eq!(data, plain);
    }

    #[test]
    #[should_panic]
    fn a_short_destination_is_a_caller_bug() {
        Blowfish::new(b"key")
            .expect("a key")
            .encrypt(&[0; 16], &mut [0; 8]);
    }
}
