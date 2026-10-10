//! The RSA-style public-key step of the engine: a block is raised to a key's
//! exponent under its modulus. It wraps the Blowfish key at the front of an
//! encrypted MIX archive.
//!
//! Numbers cross the bridge as the C++ engine always held them: 2048-bit
//! registers, least significant byte first. What a register means follows
//! the C++ bignum this replaced, quirks included, because keys and key
//! blocks are file formats:
//!
//! - A DER integer whose top bit is set is negative and is sign-extended to
//!   the whole register, which then reads as a very large unsigned number.
//! - `der_encode` looks only at the low 512 bits of a register.
//! - Raising to a power gives 1, not an error, when the exponent is zero,
//!   the modulus is zero, or the block or the exponent is not below the
//!   modulus.
#![forbid(unsafe_code)]

use crate::sha1;
use num_bigint::BigUint;

pub const NUMBER_BYTES: usize = 256;

/// A 2048-bit register, least significant byte first.
pub type Number = [u8; NUMBER_BYTES];

/// The exponent of the public half of every key pair.
pub const FAST_EXPONENT: u32 = 65537;

/// The longest encoding `der_encode` produces: tag, length, a sign byte and
/// the 64 bytes it looks at.
pub const MAX_DER_BYTES: usize = 67;

/// Decodes the DER integer at the front of `input` into `number`. Leaves
/// `number` as it was if `input` is not a tag 2, a length in at most two
/// bytes and that many bytes of a number that fits the register.
pub fn der_decode(input: &[u8], number: &mut Number) {
    let [2, length, rest @ ..] = input else {
        return;
    };
    let mut count = usize::from(*length);
    let mut rest = rest;
    if count & 0x80 != 0 {
        // The long form: the low bits count the bytes of the length.
        let length_bytes = count & 0x7f;
        if length_bytes == 0 || length_bytes > 2 {
            return;
        }
        let Some((length, after)) = rest.split_at_checked(length_bytes) else {
            return;
        };
        count = length
            .iter()
            .fold(0, |count, byte| count * 256 + usize::from(*byte));
        rest = after;
    }
    if count == 0 || count > NUMBER_BYTES {
        return;
    }
    let Some(digits) = rest.get(..count) else {
        return;
    };
    // Most significant byte first, and signed: a set top bit fills the rest
    // of the register with ones.
    number.fill(if digits[0] & 0x80 != 0 { 0xff } else { 0 });
    for (out, digit) in number.iter_mut().zip(digits.iter().rev()) {
        *out = *digit;
    }
}

/// Encodes `number` as a DER integer and returns the buffer and the number
/// of bytes of it used.
///
/// Only the low 64 bytes of the register are encoded, with the sign of the
/// whole register: the C++ encoder started its search for the top byte at
/// the register's length in 32-bit digits instead of in bytes. A number of
/// more than 512 bits comes out cut short.
pub fn der_encode(number: &Number) -> ([u8; MAX_DER_BYTES], usize) {
    const ENCODED_BYTES: usize = NUMBER_BYTES / 4;
    let negative = number[NUMBER_BYTES - 1] & 0x80 != 0;
    let filler = if negative { 0xff } else { 0 };
    // The top byte that is not just sign extension; byte 0 if none is.
    let top = (1..ENCODED_BYTES)
        .rev()
        .find(|&at| number[at] != filler)
        .unwrap_or(0);
    let mut encoded = [0; MAX_DER_BYTES];
    let mut len = 2;
    // A byte of sign in front if the top byte's own top bit says otherwise.
    if (number[top] & 0x80 != 0) != negative {
        encoded[len] = filler;
        len += 1;
    }
    for &digit in number[..=top].iter().rev() {
        encoded[len] = digit;
        len += 1;
    }
    encoded[0] = 2;
    // At most 65, so the length is always the short form.
    encoded[1] = (len - 2) as u8;
    (encoded, len)
}

/// The number of bits in `number` up to and including the highest one set.
pub fn bit_count(number: &Number) -> usize {
    bit_len(&BigUint::from_bytes_le(number))
}

/// Returns `base ^ exponent mod modulus`, or 1 where the C++ bignum refused:
/// a zero exponent, a zero modulus, or a base or exponent that is not below
/// the modulus.
///
/// One difference: the C++ aborted on a modulus of 32 bits or fewer, which
/// its reduction could not prepare. This computes the power.
pub fn exp_mod(base: &Number, exponent: &Number, modulus: &Number) -> Number {
    let base = BigUint::from_bytes_le(base);
    let exponent = BigUint::from_bytes_le(exponent);
    let modulus = BigUint::from_bytes_le(modulus);
    let refused = exponent == BigUint::ZERO
        || modulus == BigUint::ZERO
        || base >= modulus
        || exponent >= modulus;
    let power = if refused {
        BigUint::from(1u32)
    } else {
        base.modpow(&exponent, &modulus)
    };
    to_number(&power)
}

/// Encrypts or decrypts one block: `input`, least significant byte first, is
/// raised to `exponent` under `modulus`, and the low bytes of the result
/// fill `output`. Bytes of either past the register's 256 are ignored.
pub fn crypt_block(exponent: &Number, modulus: &Number, input: &[u8], output: &mut [u8]) {
    let mut block = [0; NUMBER_BYTES];
    for (out, byte) in block.iter_mut().zip(input) {
        *out = *byte;
    }
    let result = exp_mod(&block, exponent, modulus);
    for (out, byte) in output.iter_mut().zip(result) {
        *out = byte;
    }
}

/// A key pair: a block encrypted with one exponent is decrypted with the
/// other, both under `modulus`.
pub struct KeyPair {
    pub modulus: Number,
    /// FAST_EXPONENT.
    pub fast_exponent: Number,
    pub slow_exponent: Number,
}

/// The primes' sizes `generate` accepts: the modulus must outgrow the fast
/// exponent and fit the register.
pub const PRIME_BITS: std::ops::RangeInclusive<usize> = 32..=NUMBER_BYTES * 4;

/// Makes a key pair whose modulus is the product of two primes of
/// `prime_bits` bits each, or returns None if `prime_bits` is outside
/// PRIME_BITS. All randomness is derived from `seed`: the same seed gives
/// the same pair, so the seed must be as unpredictable as the key should be.
pub fn generate(seed: &[u8], prime_bits: usize) -> Option<KeyPair> {
    if !PRIME_BITS.contains(&prime_bits) {
        return None;
    }
    let mut random = SeededRandom::new(seed);
    loop {
        let p = random_prime(&mut random, prime_bits);
        let q = random_prime(&mut random, prime_bits);
        if p == q {
            continue;
        }
        let modulus = &p * &q;
        // The slow exponent d undoes the fast one e: e * d = 1 mod phi,
        // where phi = (p - 1)(q - 1). With e a small prime, d is
        // (1 + k * phi) / e for the k below e that makes the division
        // exact, which needs no more than arithmetic with small numbers.
        let phi = (&p - 1u32) * (&q - 1u32);
        let phi_mod_e = u64::from(rem_small(&phi, FAST_EXPONENT));
        let e = u64::from(FAST_EXPONENT);
        // random_prime() made sure e divides neither p - 1 nor q - 1, so
        // this has an inverse, which Fermat's little theorem gives.
        let inverse = pow_mod_small(phi_mod_e, e - 2, e);
        // Both below 2^17.
        let k = (e - inverse) as u32;
        let numerator = phi * k + 1u32;
        debug_assert_eq!(rem_small(&numerator, FAST_EXPONENT), 0);
        let slow_exponent = numerator / FAST_EXPONENT;

        // The primes are only very probably prime, so prove the pair on a
        // random block before handing it out.
        let block = random.below_bits(bit_len(&modulus) - 1);
        let fast_exponent = BigUint::from(FAST_EXPONENT);
        let round_trip = block
            .modpow(&fast_exponent, &modulus)
            .modpow(&slow_exponent, &modulus);
        if round_trip != block {
            continue;
        }
        return Some(KeyPair {
            modulus: to_number(&modulus),
            fast_exponent: to_number(&fast_exponent),
            slow_exponent: to_number(&slow_exponent),
        });
    }
}

/// Returns the low 2048 bits of `value` as a register.
fn to_number(value: &BigUint) -> Number {
    let mut number = [0; NUMBER_BYTES];
    for (out, byte) in number.iter_mut().zip(value.to_bytes_le()) {
        *out = byte;
    }
    number
}

/// The number of bits up to and including the highest one set.
fn bit_len(value: &BigUint) -> usize {
    usize::try_from(value.bits()).expect("a number that fits in memory")
}

fn rem_small(value: &BigUint, divisor: u32) -> u32 {
    (value % divisor).iter_u32_digits().next().unwrap_or(0)
}

/// `base ^ exponent mod modulus` for a modulus below 2^32.
fn pow_mod_small(base: u64, mut exponent: u64, modulus: u64) -> u64 {
    let mut square = base % modulus;
    let mut result = 1 % modulus;
    while exponent != 0 {
        if exponent & 1 != 0 {
            result = result * square % modulus;
        }
        square = square * square % modulus;
        exponent >>= 1;
    }
    result
}

/// An endless stream of bytes derived from a seed: the SHA-1 digests of the
/// seed followed by a counter.
struct SeededRandom {
    /// The seed, then the eight bytes of the counter.
    message: Vec<u8>,
    counter: u64,
    digest: [u8; 20],
    /// How much of `digest` has been handed out.
    used: usize,
}

impl SeededRandom {
    fn new(seed: &[u8]) -> SeededRandom {
        let mut message = seed.to_vec();
        message.extend([0; 8]);
        SeededRandom {
            message,
            counter: 0,
            digest: [0; 20],
            used: 20,
        }
    }

    fn fill(&mut self, bytes: &mut [u8]) {
        for byte in bytes {
            if self.used == self.digest.len() {
                let counter_at = self.message.len() - 8;
                self.message[counter_at..].copy_from_slice(&self.counter.to_le_bytes());
                self.counter += 1;
                self.digest = sha1::digest(&self.message);
                self.used = 0;
            }
            *byte = self.digest[self.used];
            self.used += 1;
        }
    }

    /// The bytes of a random number below 2^bits, least significant first.
    fn bytes_below_bits(&mut self, bits: usize) -> Vec<u8> {
        let mut bytes = vec![0; bits.div_ceil(8)];
        self.fill(&mut bytes);
        if let Some(top) = bytes.last_mut()
            && !bits.is_multiple_of(8)
        {
            *top &= 0xff >> (8 - bits % 8);
        }
        bytes
    }

    /// A random number below 2^bits.
    fn below_bits(&mut self, bits: usize) -> BigUint {
        BigUint::from_bytes_le(&self.bytes_below_bits(bits))
    }
}

/// Returns a random number of exactly `bits` bits that is very probably
/// prime, and one more than a number FAST_EXPONENT does not divide.
fn random_prime(random: &mut SeededRandom, bits: usize) -> BigUint {
    loop {
        let mut bytes = random.bytes_below_bits(bits);
        // The top bit, so that it has `bits` bits, and the bottom bit, so
        // that it is odd.
        bytes[(bits - 1) / 8] |= 1 << ((bits - 1) % 8);
        bytes[0] |= 1;
        let candidate = BigUint::from_bytes_le(&bytes);
        if rem_small(&(&candidate - 1u32), FAST_EXPONENT) != 0
            && is_probable_prime(&candidate, random)
        {
            return candidate;
        }
    }
}

/// The odd primes below 2000, which sieve out nine candidates in ten before
/// the expensive test.
fn small_odd_primes() -> impl Iterator<Item = u32> {
    (3..2000u32).step_by(2).filter(|n| {
        (3..)
            .step_by(2)
            .take_while(|d| d * d <= *n)
            .all(|d| n % d != 0)
    })
}

/// Whether the odd number `candidate`, of more than 16 bits, passes trial
/// division and 24 rounds of the Miller-Rabin test with random bases. A
/// composite passes with a probability below 4^-24.
fn is_probable_prime(candidate: &BigUint, random: &mut SeededRandom) -> bool {
    const ROUNDS: usize = 24;
    if small_odd_primes().any(|prime| rem_small(candidate, prime) == 0) {
        return false;
    }
    // candidate - 1 = odd * 2^twos.
    let minus_one = candidate - 1u32;
    let twos = minus_one.trailing_zeros().expect("an odd number above 1");
    let odd = &minus_one >> twos;
    let one = BigUint::from(1u32);
    'rounds: for _ in 0..ROUNDS {
        // A base from 2 up; one bit shorter than the candidate is below it.
        let base = random
            .below_bits(bit_len(candidate) - 1)
            .max(BigUint::from(2u32));
        // A prime has base^odd = 1, or base^(odd * 2^i) = -1 for some i
        // below twos.
        let mut power = base.modpow(&odd, candidate);
        if power == one || power == minus_one {
            continue;
        }
        for _ in 1..twos {
            power = &power * &power % candidate;
            if power == minus_one {
                continue 'rounds;
            }
        }
        return false;
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    fn number(value: u64) -> Number {
        let mut number = [0; NUMBER_BYTES];
        number[..8].copy_from_slice(&value.to_le_bytes());
        number
    }

    fn decoded(input: &[u8]) -> Number {
        let mut number = [0; NUMBER_BYTES];
        der_decode(input, &mut number);
        number
    }

    fn encoded(number: &Number) -> Vec<u8> {
        let (bytes, len) = der_encode(number);
        bytes[..len].to_vec()
    }

    #[test]
    fn der_decodes_big_endian() {
        assert_eq!(decoded(&[2, 3, 1, 0, 1]), number(65537));
        // Bytes after the number are ignored.
        assert_eq!(decoded(&[2, 1, 7, 0xff, 0xff]), number(7));
        // The long form of the length, in one byte and in two.
        assert_eq!(decoded(&[2, 0x81, 2, 1, 2]), number(0x0102));
        assert_eq!(decoded(&[2, 0x82, 0, 2, 1, 2]), number(0x0102));
    }

    #[test]
    fn der_decode_sign_extends() {
        let mut want = [0xff; NUMBER_BYTES];
        want[0] = 0x80;
        assert_eq!(decoded(&[2, 1, 0x80]), want);
        // A leading zero keeps it positive.
        assert_eq!(decoded(&[2, 2, 0, 0x80]), number(0x80));
    }

    #[test]
    fn der_decode_leaves_the_number_on_bad_input() {
        let whole = [&[2, 0x82, 1, 1][..], &[0x55; 257]].concat();
        for input in [
            &[][..],
            &[2],
            &[3, 1, 7],             // Not an integer.
            &[2, 3, 1],             // Shorter than its length.
            &[2, 0],                // No digits.
            &[2, 0x80, 7],          // A length of no bytes.
            &[2, 0x83, 0, 0, 1, 7], // A length of three bytes.
            &[2, 0x82, 1],          // The length itself cut short.
            &[2, 0x81, 0],          // A long-form zero.
            &whole,                 // 257 bytes do not fit.
        ] {
            let mut value = number(7);
            der_decode(input, &mut value);
            assert_eq!(value, number(7), "input {input:02x?}");
        }
        // 256 bytes do fit.
        let fits = [&[2, 0x82, 1, 0][..], &[0x55; 256]].concat();
        assert_eq!(decoded(&fits), [0x55; NUMBER_BYTES]);
    }

    #[test]
    fn der_encodes_the_shortest_form() {
        assert_eq!(encoded(&number(65537)), [2, 3, 1, 0, 1]);
        assert_eq!(encoded(&number(0)), [2, 1, 0]);
        assert_eq!(encoded(&number(0x7f)), [2, 1, 0x7f]);
        // A sign byte keeps a set top bit positive.
        assert_eq!(encoded(&number(0x80)), [2, 2, 0, 0x80]);
        assert_eq!(encoded(&[0xff; NUMBER_BYTES]), [2, 1, 0xff]);
        let mut negative = [0xff; NUMBER_BYTES];
        negative[0] = 0x7f;
        assert_eq!(encoded(&negative), [2, 2, 0xff, 0x7f]);
    }

    #[test]
    fn der_encode_sees_only_the_low_512_bits() {
        let mut value = [0; NUMBER_BYTES];
        value[63] = 0x80;
        value[64] = 1;
        let want = [&[2, 65, 0, 0x80][..], &[0; 63]].concat();
        assert_eq!(encoded(&value), want);
        assert_eq!(want.len(), MAX_DER_BYTES);
    }

    #[test]
    fn der_round_trips_what_fits() {
        for value in [
            1,
            0x1234,
            0x80,
            0xffff_ffff,
            u64::MAX,
            0x8000_0000_0000_0000,
        ] {
            assert_eq!(decoded(&encoded(&number(value))), number(value));
        }
    }

    #[test]
    fn counts_bits() {
        assert_eq!(bit_count(&number(0)), 0);
        assert_eq!(bit_count(&number(1)), 1);
        assert_eq!(bit_count(&number(0x1_0000_0000)), 33);
        assert_eq!(bit_count(&[0xff; NUMBER_BYTES]), 2048);
    }

    #[test]
    fn exp_mod_refuses_with_one() {
        let one = number(1);
        // A zero exponent, even for a zero base.
        assert_eq!(exp_mod(&number(5), &number(0), &number(1000)), one);
        assert_eq!(exp_mod(&number(0), &number(0), &number(1000)), one);
        // A zero modulus.
        assert_eq!(exp_mod(&number(5), &number(3), &number(0)), one);
        // A base or an exponent that is not below the modulus.
        assert_eq!(exp_mod(&number(1000), &number(3), &number(1000)), one);
        assert_eq!(exp_mod(&number(5), &number(1000), &number(1000)), one);
        // Otherwise the power.
        assert_eq!(exp_mod(&number(5), &number(3), &number(1000)), number(125));
        assert_eq!(exp_mod(&number(0), &number(3), &number(1000)), number(0));
    }

    /// The modulus of the public key the games' MIX archives are signed for.
    fn mix_modulus() -> Number {
        decoded(&[
            0x02, 0x28, 0x51, 0xbc, 0xda, 0x08, 0x6d, 0x39, 0xfc, 0xe4, 0x56, 0x51, 0x60, 0xd6,
            0x51, 0x71, 0x3f, 0xa2, 0xe8, 0xaa, 0x54, 0xfa, 0x66, 0x82, 0xb0, 0x4a, 0xab, 0xdd,
            0x0e, 0x6a, 0xf8, 0xb0, 0xc1, 0xe6, 0xd1, 0xfb, 0x4f, 0x3d, 0xaa, 0x43, 0x7f, 0x15,
        ])
    }

    #[test]
    fn crypts_a_block_with_the_mix_key() {
        // Python's pow(block, 65537, modulus), both little-endian.
        let mut block: Vec<u8> = (1..=39).collect();
        block.push(0x10);
        let want = [
            0xc3, 0x2a, 0xaf, 0xf0, 0x5b, 0xb0, 0x16, 0x0f, 0x10, 0xe4, 0xc9, 0xd1, 0x07, 0xa4,
            0x1a, 0xa3, 0xaa, 0xbb, 0x30, 0xa9, 0xfb, 0xc8, 0xf5, 0x0a, 0x6f, 0x24, 0xb4, 0x4e,
            0xfa, 0x02, 0x47, 0x2c, 0x8d, 0xfc, 0x5d, 0x9d, 0x5c, 0x4e, 0xde, 0x0d,
        ];
        assert_eq!(bit_count(&mix_modulus()), 319);
        let mut out = [0xee; 40];
        crypt_block(&number(65537), &mix_modulus(), &block, &mut out);
        assert_eq!(out, want);
        // A shorter output takes the low bytes; a longer one is zero-filled.
        let mut short = [0xee; 39];
        crypt_block(&number(65537), &mix_modulus(), &block, &mut short);
        assert_eq!(short, want[..39]);
        let mut long = [0xee; 300];
        crypt_block(&number(65537), &mix_modulus(), &block, &mut long);
        assert_eq!(long[..40], want);
        assert_eq!(long[40..256], [0; 216]);
        assert_eq!(long[256..], [0xee; 44]);
    }

    #[test]
    fn generated_pairs_round_trip() {
        for (seed, bits) in [
            (&b"one seed"[..], 32),
            (b"another", 64),
            (b"a third seed", 128),
        ] {
            let pair = generate(seed, bits).expect("a size in range");
            assert_eq!(pair.fast_exponent, number(65537));
            let modulus_bits = bit_count(&pair.modulus);
            assert!(modulus_bits == 2 * bits || modulus_bits == 2 * bits - 1);
            // A block one bit shorter than the modulus is below it.
            let plain_len = (modulus_bits - 1) / 8;
            let plain: Vec<u8> = (0..plain_len).map(|at| (at * 37 + 11) as u8).collect();
            let mut cipher = vec![0; plain_len + 1];
            crypt_block(&pair.fast_exponent, &pair.modulus, &plain, &mut cipher);
            assert_ne!(cipher[..plain_len], plain[..]);
            let mut round_trip = vec![0; plain_len];
            crypt_block(&pair.slow_exponent, &pair.modulus, &cipher, &mut round_trip);
            assert_eq!(round_trip, plain, "{bits}-bit primes");
            // And the other way: sign with the slow key, check with the fast.
            crypt_block(&pair.slow_exponent, &pair.modulus, &plain, &mut cipher);
            crypt_block(&pair.fast_exponent, &pair.modulus, &cipher, &mut round_trip);
            assert_eq!(round_trip, plain, "{bits}-bit primes, slow first");
        }
    }

    #[test]
    fn the_seed_decides_the_pair() {
        let first = generate(b"seed", 48).expect("a size in range");
        let again = generate(b"seed", 48).expect("a size in range");
        let other = generate(b"seee", 48).expect("a size in range");
        assert_eq!(first.modulus, again.modulus);
        assert_eq!(first.slow_exponent, again.slow_exponent);
        assert_ne!(first.modulus, other.modulus);
    }

    #[test]
    fn rejects_primes_too_small_or_too_large() {
        assert!(generate(b"seed", 31).is_none());
        assert!(generate(b"seed", 1025).is_none());
    }

    #[test]
    fn tells_primes_from_composites() {
        let mut random = SeededRandom::new(b"bases");
        let natural = BigUint::from;
        // 2^61 - 1 and the largest prime below 2^64.
        assert!(is_probable_prime(&natural((1u64 << 61) - 1), &mut random));
        assert!(is_probable_prime(
            &natural(18_446_744_073_709_551_557u64),
            &mut random
        ));
        // A product of two primes above the sieve, and a Carmichael number
        // whose factors are too (2521 * 5041 * 7561 would pass a Fermat test).
        assert!(!is_probable_prime(&natural(2003u64 * 2011), &mut random));
        assert!(!is_probable_prime(
            &natural(2521u64 * 5041 * 7561),
            &mut random
        ));
        assert_eq!(
            small_odd_primes().take(5).collect::<Vec<_>>(),
            [3, 5, 7, 11, 13]
        );
        assert_eq!(small_odd_primes().last(), Some(1999));
    }

    #[test]
    fn seeded_random_is_a_stream() {
        let mut whole = [0; 50];
        SeededRandom::new(b"seed").fill(&mut whole);
        let mut parts = [0; 50];
        let mut random = SeededRandom::new(b"seed");
        random.fill(&mut parts[..7]);
        random.fill(&mut parts[7..]);
        assert_eq!(whole, parts);
        // Successive digests differ.
        assert_ne!(whole[..20], whole[20..40]);
        assert!(random.below_bits(13).bits() <= 13);
    }
}
