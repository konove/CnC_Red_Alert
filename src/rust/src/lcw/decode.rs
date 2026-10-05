//! The two LCW decoders: the lenient one that pictures, shapes and movies
//! use, and the strict one behind the block codec.
#![forbid(unsafe_code)]

/// Where the lenient decoder reads its stream and writes its output.
trait Buffers {
    fn source_len(&self) -> usize;
    fn source(&self, index: usize) -> u8;
    fn dest_len(&self) -> usize;
    fn dest(&self, index: usize) -> u8;
    fn set_dest(&mut self, index: usize, value: u8);
}

/// A stream and an output in separate storage.
struct Separate<'a> {
    source: &'a [u8],
    dest: &'a mut [u8],
}

impl Buffers for Separate<'_> {
    fn source_len(&self) -> usize {
        self.source.len()
    }
    fn source(&self, index: usize) -> u8 {
        self.source[index]
    }
    fn dest_len(&self) -> usize {
        self.dest.len()
    }
    fn dest(&self, index: usize) -> u8 {
        self.dest[index]
    }
    fn set_dest(&mut self, index: usize, value: u8) {
        self.dest[index] = value;
    }
}

/// One buffer holding the output at its front and the stream at
/// `source_start`. Reads see earlier writes, as the C++ decoder's did.
struct InPlace<'a> {
    buffer: &'a mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
}

impl Buffers for InPlace<'_> {
    fn source_len(&self) -> usize {
        self.source_len
    }
    fn source(&self, index: usize) -> u8 {
        self.buffer[self.source_start + index]
    }
    fn dest_len(&self) -> usize {
        self.dest_len
    }
    fn dest(&self, index: usize) -> u8 {
        self.buffer[index]
    }
    fn set_dest(&mut self, index: usize, value: u8) {
        self.buffer[index] = value;
    }
}

/// Decodes the LCW stream in `source` into `dest`. Stops at the end marker,
/// when `dest` is full, or at the first malformed command, and returns the
/// number of bytes written so far. Never panics.
pub fn uncompress_into(source: &[u8], dest: &mut [u8]) -> usize {
    decode_lenient(&mut Separate { source, dest })
}

/// As `uncompress_into`, returning at most `max_len` bytes of output in a
/// vector. Allocates `max_len` bytes up front, so the caller bounds a length
/// it read from a file.
pub fn uncompress(source: &[u8], max_len: usize) -> Vec<u8> {
    let mut dest = vec![0; max_len];
    let written = uncompress_into(source, &mut dest);
    dest.truncate(written);
    dest
}

/// As `uncompress_into`, for a stream stored inside its own output buffer:
/// `buffer[source_start..source_start + source_len]` is decoded into
/// `buffer[..dest_len]`. This is how WSA animations, VQA movies and CPS
/// pictures load; Rust may not see one buffer as both a shared and a mutable
/// slice, so it is passed once.
///
/// Panics if either range does not fit in `buffer`: that is a caller bug,
/// never a property of the stream.
pub fn uncompress_in_place(
    buffer: &mut [u8],
    source_start: usize,
    source_len: usize,
    dest_len: usize,
) -> usize {
    assert!(
        source_start
            .checked_add(source_len)
            .is_some_and(|end| end <= buffer.len()),
        "LCW stream outside its buffer"
    );
    assert!(dest_len <= buffer.len(), "LCW output outside its buffer");
    decode_lenient(&mut InPlace {
        buffer,
        source_start,
        source_len,
        dest_len,
    })
}

fn read_u16(buffers: &impl Buffers, index: usize) -> usize {
    usize::from(buffers.source(index)) | (usize::from(buffers.source(index + 1)) << 8)
}

// A port of the C++ LCW_Uncompress(), check for check: every bound is tested
// before the read or write it guards, so the indexing below cannot panic.
fn decode_lenient(buffers: &mut impl Buffers) -> usize {
    let source_len = buffers.source_len();
    let dest_len = buffers.dest_len();
    let mut read = 0;
    let mut written = 0;
    while written < dest_len && read < source_len {
        let opcode = buffers.source(read);
        read += 1;
        let (count, mut from) = if opcode & 0x80 == 0 {
            // Short copy, relative to the write position.
            if read == source_len {
                break;
            }
            let distance = usize::from(opcode & 0x0f) * 256 + usize::from(buffers.source(read));
            read += 1;
            if distance == 0 || distance > written {
                break;
            }
            (usize::from(opcode >> 4) + 3, written - distance)
        } else if opcode & 0x40 == 0 {
            if opcode == 0x80 {
                break; // The end marker.
            }
            // Literal bytes. Byte by byte, in order: in place, the stream
            // sits behind the output.
            let count = usize::from(opcode & 0x3f)
                .min(dest_len - written)
                .min(source_len - read);
            for i in 0..count {
                let value = buffers.source(read + i);
                buffers.set_dest(written + i, value);
            }
            read += count;
            written += count;
            continue;
        } else if opcode == 0xfe {
            // Long run of one byte value.
            if source_len - read < 3 {
                break;
            }
            let count = read_u16(buffers, read).min(dest_len - written);
            let value = buffers.source(read + 2);
            read += 3;
            for i in 0..count {
                buffers.set_dest(written + i, value);
            }
            written += count;
            continue;
        } else {
            // Long (0xff) or medium copy from an absolute output position.
            let count = if opcode == 0xff {
                if source_len - read < 2 {
                    break;
                }
                let count = read_u16(buffers, read);
                read += 2;
                count
            } else {
                usize::from(opcode & 0x3f) + 3
            };
            if source_len - read < 2 {
                break;
            }
            let from = read_u16(buffers, read);
            read += 2;
            // An empty copy never reads its position.
            if count != 0 && from >= written {
                break;
            }
            (count, from)
        };
        // Forward, byte by byte, so a copy overlapping its output repeats it.
        for _ in 0..count.min(dest_len - written) {
            let value = buffers.dest(from);
            buffers.set_dest(written, value);
            from += 1;
            written += 1;
        }
    }
    written
}

fn u16_at(bytes: &[u8], index: usize) -> usize {
    usize::from(u16::from_le_bytes([bytes[index], bytes[index + 1]]))
}

/// Copies `count` bytes forward from output position `from` to `written`.
/// Returns the new write position, or None if `from` is not behind `written`
/// or the copy would run past `dest`.
fn copy_back(dest: &mut [u8], written: usize, from: usize, count: usize) -> Option<usize> {
    if from >= written || count > dest.len() - written {
        return None;
    }
    for offset in 0..count {
        dest[written + offset] = dest[from + offset];
    }
    Some(written + count)
}

/// As `uncompress_bounded_into`, returning the output in a vector of at most
/// `max_len` bytes. Allocates `max_len` bytes up front.
pub fn uncompress_bounded(source: &[u8], max_len: usize) -> Option<Vec<u8>> {
    let mut dest = vec![0; max_len];
    let written = uncompress_bounded_into(source, &mut dest)?;
    dest.truncate(written);
    Some(dest)
}

/// Decodes the LCW stream in `source` into `dest` and returns the number of
/// bytes written, or None if the stream is malformed: a command runs past
/// either buffer, a copy reads from at or beyond the write position, or the
/// end marker is missing. Never panics.
// A port of the C++ LcwUncompBounded(), check for check.
pub fn uncompress_bounded_into(source: &[u8], dest: &mut [u8]) -> Option<usize> {
    let mut read = 0;
    let mut written: usize = 0;
    while read < source.len() {
        let opcode = source[read];
        read += 1;
        let remaining = source.len() - read;
        if opcode & 0x80 == 0 {
            // Short copy, relative to the write position.
            if remaining < 1 {
                return None;
            }
            let distance = usize::from(opcode & 0x0f) * 256 + usize::from(source[read]);
            read += 1;
            let from = written.checked_sub(distance)?;
            written = copy_back(dest, written, from, usize::from(opcode >> 4) + 3)?;
        } else if opcode & 0x40 == 0 {
            if opcode == 0x80 {
                return Some(written);
            }
            let count = usize::from(opcode & 0x3f);
            if count > remaining || count > dest.len() - written {
                return None;
            }
            dest[written..written + count].copy_from_slice(&source[read..read + count]);
            read += count;
            written += count;
        } else if opcode == 0xfe {
            if remaining < 3 {
                return None;
            }
            let count = u16_at(source, read);
            if count > dest.len() - written {
                return None;
            }
            dest[written..written + count].fill(source[read + 2]);
            read += 3;
            written += count;
        } else if opcode == 0xff {
            if remaining < 4 {
                return None;
            }
            let count = u16_at(source, read);
            let from = u16_at(source, read + 2);
            read += 4;
            written = copy_back(dest, written, from, count)?;
        } else {
            if remaining < 2 {
                return None;
            }
            let from = u16_at(source, read);
            read += 2;
            written = copy_back(dest, written, from, usize::from(opcode & 0x3f) + 3)?;
        }
    }
    None // The stream ran out before the end marker.
}

#[cfg(test)]
mod tests {
    use super::*;

    // The C++ cases of engine/codec/lcw_uncompress_test.cc, one for one.

    #[test]
    fn bounds_every_command() {
        // Each stream emits abc, then a command longer than the two bytes left.
        let streams: [&[u8]; 5] = [
            &[0x83, b'a', b'b', b'c', 0x83, b'a', b'b', b'c', 0x80],
            &[0x83, b'a', b'b', b'c', 0x00, 3, 0x80],
            &[0x83, b'a', b'b', b'c', 0xc0, 0, 0, 0x80],
            &[0x83, b'a', b'b', b'c', 0xff, 3, 0, 0, 0, 0x80],
            &[0x83, b'a', b'b', b'c', 0xfe, 3, 0, b'a', 0x80],
        ];
        for stream in streams {
            let output = uncompress(stream, 5);
            assert_eq!(output.len(), 5, "{stream:x?}");
            assert_eq!(&output[..4], b"abca");
        }
    }

    #[test]
    fn into_leaves_the_rest_of_the_destination_alone() {
        let stream = [0x83, b'a', b'b', b'c', 0xfe, 9, 0, b'x', 0x80];
        let mut output = [0xcc_u8; 10];
        assert_eq!(uncompress_into(&stream, &mut output[1..6]), 5);
        assert_eq!(&output, b"\xccabcxx\xcc\xcc\xcc\xcc");

        let mut output = [0xcc_u8; 6];
        assert_eq!(
            uncompress_bounded_into(&[0x82, b'a', b'b', 0x80], &mut output[1..5]),
            Some(2)
        );
        assert_eq!(&output, b"\xccab\xcc\xcc\xcc");
    }

    #[test]
    fn rejects_references_outside_decoded_prefix() {
        let streams: [&[u8]; 6] = [
            &[0x81, b'a', 0x00, 0, 0x80],
            &[0x81, b'a', 0x00, 2, 0x80],
            &[0x81, b'a', 0xc0, 1, 0, 0x80],
            &[0x81, b'a', 0xc0, 0xff, 0xff, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 1, 0, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 0xff, 0xff, 0x80],
        ];
        for stream in streams {
            assert_eq!(uncompress(stream, 8), b"a", "{stream:x?}");
        }
    }

    #[test]
    fn preserves_overlapping_back_references() {
        let streams: [&[u8]; 3] = [
            &[0x81, b'a', 0x20, 1, 0x80],
            &[0x81, b'a', 0xc2, 0, 0, 0x80],
            &[0x81, b'a', 0xff, 5, 0, 0, 0, 0x80],
        ];
        for stream in streams {
            assert_eq!(uncompress(stream, 7), b"aaaaaa", "{stream:x?}");
        }
    }

    #[test]
    fn accepts_empty_commands_and_end_marker() {
        let stream = [0xff, 0, 0, 0xff, 0xff, 0xfe, 0, 0, b'x', 0x80, 0];
        assert!(uncompress(&stream, 1).is_empty());
    }

    #[test]
    fn empty_buffers() {
        assert!(uncompress(&[], 0).is_empty());
        assert_eq!(uncompress_bounded(&[], 0), None);
        assert_eq!(uncompress_bounded(&[0x80], 0), Some(vec![]));
    }

    #[test]
    fn stops_at_capacity_without_reading_next_command() {
        assert_eq!(uncompress(&[0x81, b'a'], 1), b"a");
    }

    #[test]
    fn truncated_commands_keep_decoded_prefix() {
        let streams: [&[u8]; 4] = [
            &[0x81, b'a', 0x00],
            &[0x81, b'a', 0xfe, 3],
            &[0x81, b'a', 0xff, 3, 0],
            &[0x81, b'a', 0xc0, 0],
        ];
        for stream in streams {
            assert_eq!(uncompress(stream, 8), b"a", "{stream:x?}");
        }
    }

    #[test]
    fn in_place_literal_copy() {
        // The stream starts at the destination's first byte.
        let mut buffer = [0x83, b'a', b'b', b'c', 0x80];
        assert_eq!(uncompress_in_place(&mut buffer, 0, 5, 3), 3);
        assert_eq!(&buffer[..3], b"abc");
    }

    #[test]
    fn in_place_stream_at_the_tail() {
        // WSA's layout: the stream fills the tail, the output grows from the
        // front and finally overwrites the stream's own end marker, which was
        // read before the copy that overwrites it.
        let mut buffer = [0_u8; 12];
        buffer[5..].copy_from_slice(&[0x83, b'a', b'b', b'c', 0x60, 3, 0x80]);
        assert_eq!(uncompress_in_place(&mut buffer, 5, 7, 12), 12);
        assert_eq!(&buffer, b"abcabcabcabc");
    }

    #[test]
    #[should_panic]
    fn in_place_rejects_ranges_outside_the_buffer() {
        let mut buffer = [0x80_u8; 4];
        uncompress_in_place(&mut buffer, 2, 3, 4);
    }

    #[test]
    fn bounded_decodes_every_command() {
        let cases: [(&[u8], &[u8]); 5] = [
            (&[0x83, b'a', b'b', b'c', 0x60, 3, 0x80], b"abcabcabcabc"),
            (&[0xfe, 5, 0, b'x', 0x80], b"xxxxx"),
            (&[0x81, b'a', 0xff, 5, 0, 0, 0, 0x80], b"aaaaaa"),
            (&[0x81, b'a', 0xc2, 0, 0, 0x80], b"aaaaaa"),
            (&[0x81, b'a', 0x20, 1, 0x80], b"aaaaaa"),
        ];
        for (stream, plain) in cases {
            let output = uncompress_bounded(stream, plain.len() + 2);
            assert_eq!(output.as_deref(), Some(plain));
        }
    }

    #[test]
    fn bounded_rejects_malformed_streams() {
        let streams: [&[u8]; 9] = [
            &[0x81, b'a'],                         // no end marker
            &[0x81, b'a', 0x00, 0, 0x80],          // short copy from zero bytes back
            &[0x81, b'a', 0x00, 2, 0x80],          // short copy before the start
            &[0x81, b'a', 0xc0, 1, 0, 0x80],       // medium copy from the write position
            &[0x81, b'a', 0xff, 0, 0, 1, 0, 0x80], // empty long copy, but from ahead
            &[0x84, b'a', b'b'],                   // literal longer than the stream
            &[0x81, b'a', 0xfe, 3],                // truncated fill
            &[0x81, b'a', 0xff, 3, 0, 0],          // truncated long copy
            &[0x81, b'a', 0xc0],                   // truncated medium copy
        ];
        for stream in streams {
            assert_eq!(uncompress_bounded(stream, 16), None, "{stream:x?}");
        }
    }

    #[test]
    fn bounded_rejects_output_overruns() {
        let streams: [&[u8]; 4] = [
            &[0x83, b'a', b'b', b'c', 0x80],
            &[0xfe, 3, 0, b'x', 0x80],
            &[0x81, b'a', 0x10, 1, 0x80],
            &[0x81, b'a', 0xff, 3, 0, 0, 0, 0x80],
        ];
        for stream in streams {
            assert_eq!(uncompress_bounded(stream, 2), None, "{stream:x?}");
        }
    }
}
