//! Westwood's uncompressed XOR delta ("format 40"), which WSA animation
//! frames and key-framed shapes are built from.
//!
//! A delta is a list of commands that walk the pixels of an image in order:
//!
//! | Command           | Meaning                                              |
//! | ----------------- | ---------------------------------------------------- |
//! | `00 nn vv`        | XOR the next `nn` pixels with `vv`                   |
//! | `01..7F ...`      | XOR the next 1..127 pixels with the bytes that follow |
//! | `81..FF`          | skip 1..127 pixels                                   |
//! | `80 00 00`        | end of the delta                                     |
//! | `80 lo hi`        | by the top bits of `hi`: `0x` skips `hi:lo` pixels,  |
//! |                   | `10` XORs the next `hi:lo & 0x3FFF` pixels with the  |
//! |                   | bytes that follow, `11` with the one byte that follows |
#![forbid(unsafe_code)]

/// Applies `delta` to an image `width` pixels wide whose rows start `stride`
/// bytes apart in `target`; the bytes between rows are left alone. The delta
/// is XORed onto the pixels, or replaces them if `copy` is set.
///
/// Stops at the end of `delta`, at a command that is cut short or has a zero
/// count, or at one that would run past the end of `target`. Does nothing if
/// `width` is zero or `stride` is less than `width`. Never panics.
pub fn apply(target: &mut [u8], mut delta: &[u8], width: usize, stride: usize, copy: bool) {
    if width == 0 || stride < width {
        return;
    }
    // The most pixels `target` can hold: whole rows, plus a final partial row
    // cut off at `width`. Checking pixel counts against it avoids multiplying
    // offsets the delta controls.
    let capacity = (target.len() / stride) * width + (target.len() % stride).min(width);
    // Index of the next pixel in the width-wide image, not an offset into
    // `target`; the two differ once stride > width. Never above `capacity`.
    let mut pixel = 0;
    while let Some((&command, rest)) = delta.split_first() {
        delta = rest;
        // Until shown otherwise, a short literal: `command` bytes to XOR.
        let mut count = usize::from(command);
        let mut run = false; // One byte is repeated `count` times.
        let mut skip = false; // `count` pixels are left unchanged.
        if command == 0 {
            let Some((&length, rest)) = delta.split_first() else {
                return;
            };
            delta = rest;
            count = usize::from(length);
            run = true;
        } else if command & 0x80 != 0 {
            count = usize::from(command & 0x7f);
            skip = true;
            // 0x80 escapes to a 16-bit little-endian count whose top bits
            // select the operation.
            if count == 0 {
                let Some((code, rest)) = delta.split_first_chunk::<2>() else {
                    return;
                };
                delta = rest;
                let code = u16::from_le_bytes(*code);
                if code == 0 {
                    return;
                }
                skip = code & 0x8000 == 0;
                run = code & 0xc000 == 0xc000;
                count = usize::from(code & if run { 0x3fff } else { 0x7fff });
            }
        }
        // No command has a use for a zero count, so the delta is corrupt.
        if count == 0 || count > capacity - pixel {
            return;
        }
        if skip {
            pixel += count;
            continue;
        }
        let operand_len = if run { 1 } else { count };
        let Some((operand, rest)) = delta.split_at_checked(operand_len) else {
            return;
        };
        delta = rest;
        // A command may cross the row end, and so has to step over the
        // stride - width bytes that are not part of the image: take it a row
        // segment at a time. The capacity check has shown that every segment
        // lies in `target`.
        let mut done = 0;
        while done < count {
            let column = pixel % width;
            let length = (count - done).min(width - column);
            let start = (pixel / width) * stride + column;
            let segment = &mut target[start..start + length];
            if run {
                let value = operand[0];
                if copy {
                    segment.fill(value);
                } else {
                    segment.iter_mut().for_each(|old| *old ^= value);
                }
            } else {
                let literal = &operand[done..done + length];
                if copy {
                    segment.copy_from_slice(literal);
                } else {
                    for (old, value) in segment.iter_mut().zip(literal) {
                        *old ^= value;
                    }
                }
            }
            pixel += length;
            done += length;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::apply;

    /// Applies `delta` to `target` as one contiguous run of pixels.
    fn apply_flat(target: &mut [u8], delta: &[u8]) {
        let len = target.len();
        apply(target, delta, len, len, false);
    }

    #[test]
    fn runs_literals_and_skips() {
        let mut pixels = [0x10; 10];
        // Skip 1, literal of 2, run of 3, end; the trailing byte is not read.
        apply_flat(&mut pixels, &[0x81, 2, 1, 2, 0, 3, 0xf0, 0x80, 0, 0, 0xff]);
        assert_eq!(
            pixels,
            [0x10, 0x11, 0x12, 0xe0, 0xe0, 0xe0, 0x10, 0x10, 0x10, 0x10]
        );
    }

    #[test]
    fn long_commands() {
        let mut pixels = [0; 8];
        // Long skip of 2, long literal of 2, long run of 3.
        apply_flat(
            &mut pixels,
            &[0x80, 2, 0x00, 0x80, 2, 0x80, 7, 8, 0x80, 3, 0xc0, 9],
        );
        assert_eq!(pixels, [0, 0, 7, 8, 9, 9, 9, 0]);
    }

    #[test]
    fn stops_at_commands_that_do_not_fit() {
        let before = [1, 2, 3, 4, 5, 6];
        for delta in [
            &[0, 7, 0xff][..],      // A run longer than the target.
            &[0x80, 0],             // A long command cut short.
            &[0],                   // A run with no count.
            &[0, 3],                // A run with no value.
            &[3, 1, 2],             // A literal cut short.
            &[0x86, 1, 0xff, 0],    // A skip to the end, then a literal.
            &[0, 0, 0xff],          // A zero count.
            &[0x80, 0, 0x80, 0xff], // A long literal of zero bytes.
            &[0x80, 0, 0, 1, 0xff], // The end marker.
            &[],
        ] {
            let mut pixels = before;
            apply_flat(&mut pixels, delta);
            assert_eq!(pixels, before, "delta {delta:02x?}");
        }
    }

    #[test]
    fn keeps_what_it_decoded_before_a_bad_command() {
        let mut pixels = [0; 4];
        apply_flat(&mut pixels, &[2, 5, 6, 0, 9, 1]);
        assert_eq!(pixels, [5, 6, 0, 0]);
    }

    #[test]
    fn rows_skip_the_padding() {
        let literal = [4, 1, 2, 3, 4, 0x80, 0, 0];
        let mut copied = [0xa5; 8];
        apply(&mut copied, &literal, 2, 4, true);
        assert_eq!(copied, [1, 2, 0xa5, 0xa5, 3, 4, 0xa5, 0xa5]);
        let mut xored = [0xa5; 8];
        apply(&mut xored, &literal, 2, 4, false);
        assert_eq!(xored, [0xa4, 0xa7, 0xa5, 0xa5, 0xa6, 0xa1, 0xa5, 0xa5]);
        let mut run = [0xa5; 8];
        apply(&mut run, &[0, 3, 7], 2, 4, true);
        assert_eq!(run, [7, 7, 0xa5, 0xa5, 7, 0xa5, 0xa5, 0xa5]);
    }

    #[test]
    fn a_partial_last_row_holds_at_most_its_width() {
        // Two rows of stride 4, then 3 bytes: 2 + 2 + 2 pixels.
        let mut pixels = [0; 11];
        apply(&mut pixels, &[0, 6, 1], 2, 4, false);
        assert_eq!(pixels, [1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0]);
        let mut pixels = [0; 11];
        apply(&mut pixels, &[0, 7, 1], 2, 4, false);
        assert_eq!(pixels, [0; 11]);
    }

    #[test]
    fn bad_geometry_does_nothing() {
        let mut pixels = [3; 4];
        apply(&mut pixels, &[0, 2, 1], 0, 4, false);
        apply(&mut pixels, &[0, 2, 1], 4, 2, false);
        apply(&mut [], &[0, 2, 1], 0, 0, false);
        assert_eq!(pixels, [3; 4]);
    }
}
