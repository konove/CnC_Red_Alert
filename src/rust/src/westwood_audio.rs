//! Westwood's sliding-delta compression of unsigned 8-bit samples, the
//! SCOMP_WESTWOOD blocks of .AUD sounds and the SND1 sound of VQA movies.
//!
//! A block is a list of commands. The top two bits of a command byte select
//! what follows; the low six are a count, stored less one:
//!
//! | Bits | Meaning                                                        |
//! | ---- | -------------------------------------------------------------- |
//! | `00` | `count` bytes of four 2-bit deltas each, lowest bits first      |
//! | `01` | `count` bytes of two 4-bit deltas each, low nibble first        |
//! | `10` | bit 5 set: the low five bits are one signed delta; otherwise    |
//! |      | `count` raw samples follow                                      |
//! | `11` | the current sample repeated `count` times                       |
//!
//! Each block starts from the midpoint 0x80.
#![forbid(unsafe_code)]

const DELTAS_2_BIT: [i8; 4] = [-2, -1, 0, 1];
const DELTAS_4_BIT: [i8; 16] = [-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8];

/// One command and the bytes it owns.
enum Command<'a> {
    Deltas2Bit(&'a [u8]),
    Deltas4Bit(&'a [u8]),
    Delta(i8),
    Raw(&'a [u8]),
    Repeat(usize),
}

impl Command<'_> {
    /// The number of samples the command decodes to.
    fn samples(&self) -> usize {
        match self {
            Command::Deltas2Bit(packed) => packed.len() * 4,
            Command::Deltas4Bit(packed) => packed.len() * 2,
            Command::Delta(_) => 1,
            Command::Raw(raw) => raw.len(),
            Command::Repeat(count) => *count,
        }
    }
}

/// Splits the first command off `block`. Returns None at the end of the
/// block, and `Some(None)` for a command that runs past it.
fn next_command<'a>(block: &mut &'a [u8]) -> Option<Option<Command<'a>>> {
    let (&command, rest) = block.split_first()?;
    *block = rest;
    let low_bits = command & 0x3f;
    let count = usize::from(low_bits) + 1;
    let mut take = || {
        let (taken, rest) = block.split_at_checked(count)?;
        *block = rest;
        Some(taken)
    };
    Some(match command >> 6 {
        0 => take().map(Command::Deltas2Bit),
        1 => take().map(Command::Deltas4Bit),
        // Five bits, two's complement: shift the sign bit to the top and
        // back down.
        2 if low_bits & 0x20 != 0 => Some(Command::Delta(((low_bits << 3) as i8) >> 3)),
        2 => take().map(Command::Raw),
        _ => Some(Command::Repeat(count)),
    })
}

/// For each command byte, the number of bytes that follow it and the number
/// of samples it decodes to. `decoded_len` reads them from a table so that
/// sizing a block costs no branch on the kind of each command, which
/// `decode_into` then pays again.
const SIZES: [(u8, u16); 256] = {
    let mut sizes = [(0, 0); 256];
    let mut command = 0;
    while command < 256 {
        let count = (command as u8 & 0x3f) + 1;
        sizes[command] = match command >> 6 {
            0 => (count, count as u16 * 4),
            1 => (count, count as u16 * 2),
            2 if command & 0x20 != 0 => (0, 1),
            2 => (count, count as u16),
            _ => (0, count as u16),
        };
        command += 1;
    }
    sizes
};

/// Returns the number of samples `block` decodes to, or None if a command
/// runs past its end.
pub fn decoded_len(mut block: &[u8]) -> Option<usize> {
    let mut len = 0;
    while let Some((&command, rest)) = block.split_first() {
        let (payload, samples) = SIZES[usize::from(command)];
        block = rest.get(usize::from(payload)..)?;
        len += usize::from(samples);
    }
    Some(len)
}

/// Decodes `block` into the front of `dest` and returns the number of
/// samples. Returns None if a command runs past the end of `block` or its
/// samples past the end of `dest`; `dest` then holds what was decoded before
/// it. Never panics.
pub fn decode_into(mut block: &[u8], dest: &mut [u8]) -> Option<usize> {
    let mut sample = 0x80u8;
    let mut written = 0;
    while let Some(command) = next_command(&mut block) {
        let command = command?;
        let out = dest.get_mut(written..written + command.samples())?;
        written += out.len();
        match command {
            Command::Deltas2Bit(packed) => {
                for (&codes, out) in packed.iter().zip(out.as_chunks_mut::<4>().0) {
                    for (shift, out) in [0, 2, 4, 6].into_iter().zip(out) {
                        let delta = DELTAS_2_BIT[usize::from((codes >> shift) & 3)];
                        sample = sample.saturating_add_signed(delta);
                        *out = sample;
                    }
                }
            }
            Command::Deltas4Bit(packed) => {
                for (&codes, out) in packed.iter().zip(out.as_chunks_mut::<2>().0) {
                    for (shift, out) in [0, 4].into_iter().zip(out) {
                        let delta = DELTAS_4_BIT[usize::from((codes >> shift) & 0xf)];
                        sample = sample.saturating_add_signed(delta);
                        *out = sample;
                    }
                }
            }
            // Unlike the packed deltas it wraps rather than saturates.
            Command::Delta(delta) => {
                sample = sample.wrapping_add_signed(delta);
                out[0] = sample;
            }
            Command::Raw(raw) => {
                out.copy_from_slice(raw);
                // `raw` holds at least one sample.
                sample = raw[raw.len() - 1];
            }
            Command::Repeat(_) => out.fill(sample),
        }
    }
    Some(written)
}

/// Decodes `block` into a vector; None as for `decode_into`.
pub fn decode(block: &[u8]) -> Option<Vec<u8>> {
    let mut samples = vec![0; decoded_len(block)?];
    decode_into(block, &mut samples)?;
    Some(samples)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_size_table_agrees_with_the_decoder() {
        for command in 0..=u8::MAX {
            let (payload, samples) = SIZES[usize::from(command)];
            let mut block = vec![command];
            block.resize(1 + usize::from(payload), 0x55);
            let decoded = decode(&block).expect("a whole command");
            assert_eq!(decoded.len(), usize::from(samples));
            // One byte less cuts a command with a payload short, and leaves
            // nothing of one without.
            block.pop();
            assert_eq!(decoded_len(&block).is_none(), payload > 0);
        }
    }

    #[test]
    fn empty_block_is_no_samples() {
        assert_eq!(decode(&[]), Some(vec![]));
    }

    #[test]
    fn two_bit_deltas_lowest_bits_first() {
        // 0b11_10_01_00: -2, -1, 0, +1.
        assert_eq!(decode(&[0x00, 0xe4]), Some(vec![0x7e, 0x7d, 0x7d, 0x7e]));
    }

    #[test]
    fn four_bit_deltas_low_nibble_first() {
        // Low nibble 0 is -9, high nibble 15 is +8.
        assert_eq!(decode(&[0x40, 0xf0]), Some(vec![0x77, 0x7f]));
    }

    #[test]
    fn packed_deltas_saturate() {
        // Raw 0x01, then four -2s; raw 0xfe, then two +8s.
        assert_eq!(decode(&[0x80, 0x01, 0x00, 0x00]), Some(vec![1, 0, 0, 0, 0]));
        assert_eq!(
            decode(&[0x80, 0xfe, 0x40, 0xff]),
            Some(vec![0xfe, 0xff, 0xff])
        );
    }

    #[test]
    fn a_single_delta_wraps() {
        // 0xbf is delta -1, 0xaf is delta +15.
        assert_eq!(decode(&[0x80, 0x00, 0xbf]), Some(vec![0x00, 0xff]));
        assert_eq!(decode(&[0x80, 0xf8, 0xaf]), Some(vec![0xf8, 0x07]));
        assert_eq!(decode(&[0xb0]), Some(vec![0x70]));
    }

    #[test]
    fn raw_samples_set_the_current_sample() {
        assert_eq!(
            decode(&[0x81, 0x10, 0x20, 0xc1]),
            Some(vec![0x10, 0x20, 0x20, 0x20])
        );
    }

    #[test]
    fn repeat_runs_up_to_sixty_four() {
        assert_eq!(decode(&[0xff]), Some(vec![0x80; 64]));
    }

    #[test]
    fn a_command_past_the_end_fails() {
        assert_eq!(
            decode(&[0xc0, 0x41, 0x00, 0x00]).map(|samples| samples.len()),
            Some(5)
        );
        assert_eq!(decode(&[0xc0, 0x41, 0x00]), None);
        assert_eq!(decode(&[0x82, 0x01]), None);
        assert_eq!(decode(&[0x03, 0x00]), None);
        assert_eq!(decoded_len(&[0x03, 0x00]), None);
    }

    #[test]
    fn a_short_destination_fails() {
        let mut dest = [0; 3];
        assert_eq!(decode_into(&[0xc1, 0xc1], &mut dest), None);
        assert_eq!(dest, [0x80, 0x80, 0]);
        assert_eq!(decode_into(&[0xc1], &mut dest), Some(2));
    }
}
