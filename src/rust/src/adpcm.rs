//! IMA ADPCM as Westwood used it: 4 bits a 16-bit sample, the low nibble of
//! each byte first. It is the SCOMP_SOS compression of .AUD sounds and the
//! SND2 sound of VQA movies.
#![forbid(unsafe_code)]

/// Step index change for each 4-bit code: small magnitudes step the
/// quantizer down, large ones up. The sign bit (8) does not matter.
const INDEX_TABLE: [i8; 16] = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8];

/// Quantizer step sizes, roughly 10% apart.
const STEP_TABLE: [i16; 89] = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272,
    2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
];

const MAX_STEP_INDEX: u8 = 88;

/// The running state of a stream. It carries over from one block to the
/// next; the default is the start of a stream.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct State {
    /// The last sample decoded.
    pub predictor: i16,
    /// Index into the step table. Anything above 88 is read as 88.
    pub step_index: u8,
}

/// The step index after each code at each index: `index + INDEX_TABLE[code]`
/// held to the step table. A table because whether the index saturates
/// follows the sound, so a branch on it is mispredicted often.
const NEXT_INDEX: [[u8; 16]; STEP_TABLE.len()] = {
    let mut table = [[0; 16]; STEP_TABLE.len()];
    let mut index = 0;
    while index < table.len() {
        let mut code = 0;
        while code < 16 {
            let next = index as i32 + INDEX_TABLE[code] as i32;
            table[index][code] = if next < 0 {
                0
            } else if next > MAX_STEP_INDEX as i32 {
                MAX_STEP_INDEX
            } else {
                next as u8
            };
            code += 1;
        }
        index += 1;
    }
    table
};

/// Returns `value` held to the 16-bit range, for a value within twice that
/// range. Written as mask arithmetic for the reason NEXT_INDEX is a table:
/// with `clamp`, which compiles to two branches, the decoder ran at a third
/// of the speed on sound that saturates now and then.
fn saturate(value: i32) -> i16 {
    // All ones if the value is below, or above, the range.
    let below = (value - i32::from(i16::MIN)) >> 31;
    let above = (i32::from(i16::MAX) - value) >> 31;
    let held =
        (value & !(below | above)) | (i32::from(i16::MIN) & below) | (i32::from(i16::MAX) & above);
    held as i16
}

impl State {
    /// Returns the state with its step index held to the table.
    fn checked(self) -> State {
        State {
            step_index: self.step_index.min(MAX_STEP_INDEX),
            ..self
        }
    }

    /// Decodes one 4-bit code: bit 3 is the sign, bits 0-2 the magnitude.
    fn decode(&mut self, code: u8) -> i16 {
        // In range, as every decoder starts from `checked()`; the index is
        // not held to the table here because that would sit on the path from
        // one code's index to the next.
        let index = usize::from(self.step_index);
        // This sample uses the step size before the update.
        let step = i32::from(STEP_TABLE[index]);
        self.step_index = NEXT_INDEX[index][usize::from(code & 0xf)];
        // The difference is (magnitude + 1/2) * step / 4. The IMA reference
        // adds the shifted steps bit by bit and truncates each; this
        // truncates once, so it can come out up to 3 higher.
        let magnitude = (i32::from(code & 7) * 2 + 1) * step / 8;
        // All ones for a negative code; negates without a branch.
        let sign = -i32::from((code >> 3) & 1);
        let difference = (magnitude ^ sign) - sign;
        // At most 15/8 of the largest step either way.
        self.predictor = saturate(i32::from(self.predictor) + difference);
        self.predictor
    }

    /// Decodes the two codes of `byte`, low nibble first.
    fn decode_byte(&mut self, byte: u8) -> [i16; 2] {
        [self.decode(byte & 0xf), self.decode(byte >> 4)]
    }
}

/// Decodes `source` into two samples per byte, until either slice runs out.
pub fn decode_samples(state: &mut State, source: &[u8], dest: &mut [i16]) {
    *state = state.checked();
    for (&byte, pair) in source.iter().zip(dest.as_chunks_mut::<2>().0) {
        *pair = state.decode_byte(byte);
    }
}

/// Fills `dest` with little-endian samples, 4 bytes per byte of `source`; a
/// trailing part of a sample pair in `dest` is left as it was. Returns
/// false, decoding nothing, when `source` is too short to fill `dest`.
pub fn decode_bytes(state: &mut State, source: &[u8], dest: &mut [u8]) -> bool {
    let (pairs, _) = dest.as_chunks_mut::<4>();
    if pairs.len() > source.len() {
        return false;
    }
    *state = state.checked();
    for (&byte, pair) in source.iter().zip(pairs) {
        let [first, second] = state.decode_byte(byte);
        pair[..2].copy_from_slice(&first.to_le_bytes());
        pair[2..].copy_from_slice(&second.to_le_bytes());
    }
    true
}

/// As `decode_bytes`, for a stream stored in its own output buffer:
/// `buffer[source_start..source_start + source_len]` is decoded into
/// `buffer[..dest_len]`. Each byte is read before its samples are written,
/// so a stream at the end of the buffer is consumed ahead of the output; one
/// the output overtakes is read back as the samples written over it. VQA
/// movies load their sound this way.
///
/// Panics if either range does not fit in `buffer`: that is a caller bug,
/// never a property of the stream.
pub fn decode_bytes_in_place(
    state: &mut State,
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> bool {
    assert!(dest_len <= buffer.len(), "destination outside the buffer");
    assert!(
        source_len <= buffer.len() && source_start <= buffer.len() - source_len,
        "stream outside the buffer"
    );
    let pairs = dest_len / 4;
    if pairs > source_len {
        return false;
    }
    *state = state.checked();
    for pair in 0..pairs {
        let [first, second] = state.decode_byte(buffer[source_start + pair]);
        let at = pair * 4;
        buffer[at..at + 2].copy_from_slice(&first.to_le_bytes());
        buffer[at + 2..at + 4].copy_from_slice(&second.to_le_bytes());
    }
    true
}

#[cfg(test)]
mod tests {
    use super::*;

    fn samples(state: &mut State, source: &[u8]) -> Vec<i16> {
        let mut dest = vec![0; source.len() * 2];
        decode_samples(state, source, &mut dest);
        dest
    }

    #[test]
    fn low_nibble_first() {
        // Code 7 at step 7: (7 * 2 + 1) * 7 / 8 = 13, and the index rises by
        // 8 to step 16. Then code 0: 16 / 8 = 2.
        let mut state = State::default();
        assert_eq!(samples(&mut state, &[0x07]), [13, 15]);
        assert_eq!(
            state,
            State {
                predictor: 15,
                step_index: 7
            }
        );
    }

    #[test]
    fn the_sign_bit_subtracts() {
        let mut state = State::default();
        assert_eq!(samples(&mut state, &[0x0f]), [-13, -11]);
    }

    #[test]
    fn state_carries_across_calls() {
        let mut split = State::default();
        let mut decoded = samples(&mut split, &[0x37, 0x9a]);
        decoded.extend(samples(&mut split, &[0x5c]));
        let mut whole = State::default();
        assert_eq!(decoded, samples(&mut whole, &[0x37, 0x9a, 0x5c]));
        assert_eq!(split, whole);
    }

    #[test]
    fn predictor_and_index_saturate() {
        let mut state = State {
            predictor: 32767,
            step_index: 88,
        };
        assert_eq!(samples(&mut state, &[0x77]), [32767, 32767]);
        assert_eq!(state.step_index, 88);
        let mut state = State {
            predictor: -32768,
            step_index: 0,
        };
        assert_eq!(samples(&mut state, &[0x88]), [-32768, -32768]);
        assert_eq!(state.step_index, 0);
    }

    #[test]
    fn an_index_out_of_range_reads_as_the_last() {
        let mut high = State {
            predictor: 0,
            step_index: 200,
        };
        let mut last = State {
            predictor: 0,
            step_index: 88,
        };
        assert_eq!(samples(&mut high, &[0x12]), samples(&mut last, &[0x12]));
    }

    #[test]
    fn samples_stop_at_the_shorter_slice() {
        let mut dest = [9; 3];
        decode_samples(&mut State::default(), &[0x07, 0x07], &mut dest);
        assert_eq!(dest, [13, 15, 9]);
        let mut dest = [9; 4];
        decode_samples(&mut State::default(), &[0x07], &mut dest);
        assert_eq!(dest, [13, 15, 9, 9]);
    }

    #[test]
    fn bytes_are_little_endian_and_leave_a_partial_pair() {
        let mut dest = [0xee; 6];
        assert!(decode_bytes(
            &mut State::default(),
            &[0x0f, 0x00],
            &mut dest
        ));
        assert_eq!(dest, [0xf3, 0xff, 0xf5, 0xff, 0xee, 0xee]);
    }

    #[test]
    fn bytes_refuse_a_short_source() {
        let mut state = State::default();
        let mut dest = [0xee; 8];
        assert!(!decode_bytes(&mut state, &[0x07], &mut dest));
        assert_eq!(dest, [0xee; 8]);
        assert_eq!(state, State::default());
        assert!(!decode_bytes_in_place(&mut state, &mut dest, 7, 1, 8));
        assert_eq!(dest, [0xee; 8]);
    }

    #[test]
    fn in_place_matches_separate_buffers() {
        let source = [0x37, 0x9a, 0x5c, 0xf1];
        let mut want = [0; 16];
        assert!(decode_bytes(&mut State::default(), &source, &mut want));
        // The stream at the very end, as the movie loader puts it.
        let mut buffer = [0; 16];
        buffer[12..].copy_from_slice(&source);
        let mut state = State::default();
        assert!(decode_bytes_in_place(&mut state, &mut buffer, 12, 4, 16));
        assert_eq!(buffer, want);
    }

    #[test]
    fn in_place_reads_back_what_it_overwrote() {
        // Source and output start together: byte 1 is read after the first
        // pair's samples replaced it.
        let mut buffer = [0x07, 0x55, 0, 0, 0, 0, 0, 0];
        let mut state = State::default();
        assert!(decode_bytes_in_place(&mut state, &mut buffer, 0, 2, 8));
        let mut want_state = State::default();
        let first = want_state.decode_byte(0x07);
        let second = want_state.decode_byte(first[0].to_le_bytes()[1]);
        assert_eq!(buffer[..2], first[0].to_le_bytes());
        assert_eq!(buffer[4..6], second[0].to_le_bytes());
        assert_eq!(state, want_state);
    }

    #[test]
    #[should_panic(expected = "stream outside the buffer")]
    fn in_place_rejects_a_stream_past_the_buffer() {
        decode_bytes_in_place(&mut State::default(), &mut [0; 8], 6, 4, 8);
    }
}
