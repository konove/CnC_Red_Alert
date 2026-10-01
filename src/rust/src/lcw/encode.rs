//! The LCW encoder: greedy, finding earlier occurrences through hash chains
//! over 3-byte prefixes.
#![forbid(unsafe_code)]

const MAX_LITERAL_RUN: u8 = 63; // 10xxxxxx holds 1..63 bytes.
const MAX_SHORT_COUNT: usize = 10; // 0xxx: 3..10 bytes.
const MAX_SHORT_DISTANCE: usize = 0xfff; // 12-bit relative distance.
const MAX_MEDIUM_COUNT: usize = 64; // 11xxxxxx below 0xfe: 3..64.
const MAX_LONG_COUNT: usize = 0xffff; // 16-bit count for 0xfe and 0xff.
const MAX_ABSOLUTE_POSITION: usize = 0xffff;
const HASH_SIZE: usize = 1 << 12;
const MAX_CHAIN_STEPS: usize = 256;

/// Returns the largest stream `compress` produces for `length` input bytes:
/// the input as literal runs of at most 63 bytes, each behind one opcode,
/// plus the end marker. Matches the C++ LcwWorstCaseSize().
pub const fn worst_case_size(length: usize) -> usize {
    length + length.div_ceil(MAX_LITERAL_RUN as usize) + 1
}

/// One way to encode the bytes at the current position.
#[derive(Clone, Copy)]
enum Kind {
    Literal,
    Short,
    Medium,
    Long,
    Fill,
}

#[derive(Clone, Copy)]
struct Choice {
    kind: Kind,
    count: usize,   // Input bytes covered.
    from: usize,    // Distance (short) or absolute position (medium, long).
    savings: isize, // Input bytes covered minus encoded bytes.
}

const LITERAL: Choice = Choice {
    kind: Kind::Literal,
    count: 1,
    from: 0,
    savings: 0,
};

/// Bytes covered minus bytes spent; counts never exceed 0xffff.
fn savings(count: usize, cost: usize) -> isize {
    count as isize - cost as isize
}

/// Returns the cheapest back-reference for a match of `length` bytes,
/// `distance` bytes back, at absolute `position`. Ties keep the earlier,
/// shorter encoding, as the C++ BestReference() does.
fn best_reference(length: usize, distance: usize, position: usize) -> Choice {
    let mut best = LITERAL;
    if distance <= MAX_SHORT_DISTANCE {
        let count = length.min(MAX_SHORT_COUNT);
        best = Choice {
            kind: Kind::Short,
            count,
            from: distance,
            savings: savings(count, 2),
        };
    }
    if position <= MAX_ABSOLUTE_POSITION {
        let medium = length.min(MAX_MEDIUM_COUNT);
        if savings(medium, 3) > best.savings {
            best = Choice {
                kind: Kind::Medium,
                count: medium,
                from: position,
                savings: savings(medium, 3),
            };
        }
        let long = length.min(MAX_LONG_COUNT);
        if savings(long, 5) > best.savings {
            best = Choice {
                kind: Kind::Long,
                count: long,
                from: position,
                savings: savings(long, 5),
            };
        }
    }
    best
}

fn hash_at(input: &[u8], pos: usize) -> usize {
    let hash =
        (u32::from(input[pos]) << 8) ^ (u32::from(input[pos + 1]) << 4) ^ u32::from(input[pos + 2]);
    hash as usize % HASH_SIZE
}

/// Earlier positions by their 3-byte prefix, most recent first.
struct Chains {
    head: Vec<Option<usize>>,
    previous: Vec<Option<usize>>,
}

impl Chains {
    fn new(length: usize) -> Self {
        Self {
            head: vec![None; HASH_SIZE],
            previous: vec![None; length],
        }
    }

    fn remember(&mut self, input: &[u8], pos: usize) {
        if pos + 2 < input.len() {
            let bucket = hash_at(input, pos);
            self.previous[pos] = self.head[bucket];
            self.head[bucket] = Some(pos);
        }
    }
}

/// The output stream being written.
struct Writer<'a> {
    output: &'a mut [u8],
    written: usize,
}

impl Writer<'_> {
    fn put(&mut self, value: u8) {
        self.output[self.written] = value;
        self.written += 1;
    }

    fn put_u16(&mut self, value: usize) {
        let [low, high] = u16::try_from(value)
            .expect("LCW operands are 16 bits")
            .to_le_bytes();
        self.put(low);
        self.put(high);
    }
}

/// Compresses `input` into `output` as one LCW stream ending in the 0x80
/// marker. Returns the stream size, or None if `output` is shorter than
/// `worst_case_size(input.len())`.
// A port of the C++ LCW_Comp(): a back-reference or fill is used only when it
// saves at least one byte over literals, which also pays for any literal
// opcode it forces afterwards, so the output never exceeds worst_case_size().
pub fn compress(input: &[u8], output: &mut [u8]) -> Option<usize> {
    let length = input.len();
    if output.len() < worst_case_size(length) {
        return None;
    }
    let mut chains = Chains::new(length);
    let mut out = Writer { output, written: 0 };
    let mut literal_opcode: Option<usize> = None; // The open literal run's opcode.

    let mut pos = 0;
    while pos < length {
        let limit = (length - pos).min(MAX_LONG_COUNT);
        let mut best = LITERAL;

        let run = 1 + input[pos + 1..pos + limit]
            .iter()
            .take_while(|&&byte| byte == input[pos])
            .count();
        if savings(run, 4) > best.savings {
            best = Choice {
                kind: Kind::Fill,
                count: run,
                from: 0,
                savings: savings(run, 4),
            };
        }

        if pos + 2 < length {
            let mut candidate = chains.head[hash_at(input, pos)];
            let mut steps = 0;
            while let Some(start) = candidate {
                if steps == MAX_CHAIN_STEPS {
                    break;
                }
                // Overlapping matches are fine: the decoder copies forward.
                let matched = input[start..]
                    .iter()
                    .zip(&input[pos..pos + limit])
                    .take_while(|(earlier, current)| earlier == current)
                    .count();
                if matched >= 3 {
                    let reference = best_reference(matched, pos - start, start);
                    if reference.savings > best.savings {
                        best = reference;
                    }
                }
                candidate = chains.previous[start];
                steps += 1;
            }
        }

        if best.savings < 1 {
            let opcode = match literal_opcode {
                Some(at) if out.output[at] != 0x80 + MAX_LITERAL_RUN => at,
                _ => {
                    let at = out.written;
                    out.put(0x80);
                    literal_opcode = Some(at);
                    at
                }
            };
            out.output[opcode] += 1;
            out.put(input[pos]);
            chains.remember(input, pos);
            pos += 1;
            continue;
        }

        match best.kind {
            Kind::Short => {
                // Both fit: count - 3 <= 7 and from <= 0xfff.
                out.put(((best.count - 3) * 16 + best.from / 256) as u8);
                out.put((best.from % 256) as u8);
            }
            Kind::Medium => {
                out.put(0xc0 + (best.count - 3) as u8);
                out.put_u16(best.from);
            }
            Kind::Long => {
                out.put(0xff);
                out.put_u16(best.count);
                out.put_u16(best.from);
            }
            Kind::Fill => {
                out.put(0xfe);
                out.put_u16(best.count);
                out.put(input[pos]);
            }
            Kind::Literal => unreachable!("a literal saves nothing"),
        }
        for i in 0..best.count {
            chains.remember(input, pos + i);
        }
        pos += best.count;
        literal_opcode = None;
    }

    out.put(0x80);
    Some(out.written)
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::lcw::uncompress_bounded;

    // The data generators of engine/codec/lcw_comp_test.cc.

    /// Deterministic pseudo-random bytes; incompressible for LCW.
    fn random(size: usize, mut seed: u32) -> Vec<u8> {
        (0..size)
            .map(|_| {
                seed = seed.wrapping_mul(1_664_525).wrapping_add(1_013_904_223);
                (seed >> 24) as u8
            })
            .collect()
    }

    /// Runs of one value, lengths cycling from 1 to 300.
    fn runs(size: usize) -> Vec<u8> {
        let mut bytes = Vec::with_capacity(size);
        let mut run = 1;
        while bytes.len() < size {
            bytes.extend(std::iter::repeat_n((run % 251) as u8, run));
            run = run % 300 + 1;
        }
        bytes.truncate(size);
        bytes
    }

    /// A 37-byte pattern repeated, with every 97th byte changed.
    fn repeats(size: usize) -> Vec<u8> {
        (0..size)
            .map(|i| if i % 97 == 0 { 0xee } else { (i % 37) as u8 })
            .collect()
    }

    fn compress_all(input: &[u8]) -> Vec<u8> {
        let mut output = vec![0_u8; worst_case_size(input.len())];
        let size = compress(input, &mut output).expect("output sized by worst_case_size");
        output.truncate(size);
        output
    }

    fn assert_round_trip(plain: &[u8]) {
        let stream = compress_all(plain);
        assert!(stream.len() <= worst_case_size(plain.len()));
        let mut decoded = vec![0_u8; plain.len()];
        assert_eq!(uncompress_bounded(&stream, &mut decoded), Some(plain.len()));
        assert!(
            decoded == plain,
            "round trip changed {} bytes of input",
            plain.len()
        );
    }

    #[test]
    #[allow(
        clippy::manual_div_ceil,
        reason = "spells out the C++ LcwWorstCaseSize()"
    )]
    fn worst_case_size_matches_the_cpp_formula() {
        for length in [0, 1, 62, 63, 64, 126, 127, 4096, 65_535] {
            assert_eq!(worst_case_size(length), length + (length + 62) / 63 + 1);
        }
    }

    #[test]
    fn empty_input_is_just_the_end_marker() {
        assert_eq!(compress_all(&[]), [0x80]);
    }

    #[test]
    fn single_byte_is_one_literal() {
        assert_eq!(compress_all(&[42]), [0x81, 42, 0x80]);
    }

    #[test]
    fn repeated_pattern_becomes_a_short_copy() {
        // Three literals, then nine bytes from three back: 0x60 = (9 - 3) << 4.
        assert_eq!(
            compress_all(b"abcabcabcabc"),
            [0x83, b'a', b'b', b'c', 0x60, 3, 0x80]
        );
    }

    #[test]
    fn long_run_becomes_a_fill() {
        // 0xfe, count 1000 = 0x03e8 little-endian, value 7.
        assert_eq!(compress_all(&[7; 1000]), [0xfe, 0xe8, 0x03, 7, 0x80]);
    }

    #[test]
    fn rejects_a_small_output() {
        let input = random(100, 1);
        let mut output = vec![0_u8; worst_case_size(input.len()) - 1];
        assert_eq!(compress(&input, &mut output), None);
    }

    #[test]
    fn round_trips() {
        assert_round_trip(&random(8192, 12_345));
        assert_round_trip(&runs(8192));
        assert_round_trip(&repeats(8192));
        assert_round_trip(&runs(65_535));
    }

    #[test]
    fn round_trips_past_sixteen_bit_positions() {
        let mut plain = repeats(70_000);
        plain.extend(runs(70_000));
        plain.extend(random(10_000, 99));
        assert_round_trip(&plain);
    }

    #[test]
    fn compresses_redundant_data() {
        let plain = repeats(8192);
        assert!(compress_all(&plain).len() < plain.len() / 4);
    }
}
