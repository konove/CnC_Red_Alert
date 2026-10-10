//! The VQ frame decoder of VQA movies, which rebuilds a frame's pixels from
//! its vector pointers and the codebook of pixel blocks they index.
#![forbid(unsafe_code)]

/// Every block is this many pixels wide.
const BLOCK_WIDTH: usize = 4;

/// Decodes a frame of blocks 4 pixels wide and `block_height` lines high
/// into `buffer`, a pixel buffer `stride` bytes wide that starts at the
/// image's top-left pixel.
///
/// `pointers` holds one 16-bit entry per block, `blocks_per_row` x
/// `block_rows` of them, as two planes: every entry's low byte, then every
/// entry's high byte. An entry numbers a block in `codebook`, or, when its
/// high byte is `solid_marker`, is a solid block of the color in its low
/// byte.
///
/// Sizes that do not fit the slices draw nothing. An entry past the end of
/// `codebook` shows only when decoding reaches it, so it stops the decode
/// there and leaves the rest of `buffer` as it was. Never panics.
#[allow(clippy::too_many_arguments)]
pub fn decode_frame(
    codebook: &[u8],
    pointers: &[u8],
    buffer: &mut [u8],
    blocks_per_row: usize,
    block_rows: usize,
    stride: usize,
    block_height: usize,
    solid_marker: u8,
) {
    let (columns, rows, height) = (blocks_per_row, block_rows, block_height);
    if columns == 0 || rows == 0 || height == 0 || columns > stride / BLOCK_WIDTH {
        return;
    }
    let row_bytes = columns * BLOCK_WIDTH;
    // Two pointer bytes per block, and pixel rows for every block row; the
    // last pixel row needs only its blocks' width, not a whole stride.
    if rows > pointers.len() / 2 / columns
        || buffer.len() < row_bytes
        || rows > ((buffer.len() - row_bytes) / stride + 1) / height
    {
        return;
    }
    let block_bytes = BLOCK_WIDTH * height;
    let (low_bytes, high_bytes) = pointers.split_at(rows * columns);
    for row in 0..rows {
        let low_bytes = &low_bytes[row * columns..][..columns];
        let high_bytes = &high_bytes[row * columns..][..columns];
        // Where entry `col` of this row has its block in the codebook, if it
        // is not solid.
        let code_offset = |col: usize| {
            (usize::from(high_bytes[col]) * 256 + usize::from(low_bytes[col])) * block_bytes
        };
        // The blocks before the first entry past the codebook. The frame is
        // drawn a pixel row at a time, so they are counted first.
        let drawn = (0..columns)
            .find(|&col| {
                high_bytes[col] != solid_marker && code_offset(col) + block_bytes > codebook.len()
            })
            .unwrap_or(columns);
        for line in 0..height {
            let pixels = &mut buffer[(row * height + line) * stride..][..drawn * BLOCK_WIDTH];
            for (col, block) in pixels
                .as_chunks_mut::<BLOCK_WIDTH>()
                .0
                .iter_mut()
                .enumerate()
            {
                if high_bytes[col] == solid_marker {
                    *block = [low_bytes[col]; BLOCK_WIDTH];
                } else {
                    let code = &codebook[code_offset(col) + line * BLOCK_WIDTH..][..BLOCK_WIDTH];
                    block.copy_from_slice(code);
                }
            }
        }
        if drawn < columns {
            return;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::decode_frame;

    /// Two 4x2 blocks: 1..=8 and 11..=18.
    const CODEBOOK: [u8; 16] = [1, 2, 3, 4, 5, 6, 7, 8, 11, 12, 13, 14, 15, 16, 17, 18];

    fn decode_4x2(pointers: &[u8], buffer: &mut [u8], columns: usize, rows: usize, stride: usize) {
        decode_frame(&CODEBOOK, pointers, buffer, columns, rows, stride, 2, 0x0f);
    }

    #[test]
    fn copies_codebook_blocks_and_fills_solid_ones() {
        // Row 0: block 1, solid 0x77. Row 1: solid 0x66, block 0.
        let pointers = [1, 0x77, 0x66, 0, 0, 0x0f, 0x0f, 0];
        let mut buffer = [0xee; 10 * 3 + 8];
        decode_4x2(&pointers, &mut buffer, 2, 2, 10);
        #[rustfmt::skip]
        assert_eq!(buffer, [
            11, 12, 13, 14, 0x77, 0x77, 0x77, 0x77, 0xee, 0xee,
            15, 16, 17, 18, 0x77, 0x77, 0x77, 0x77, 0xee, 0xee,
            0x66, 0x66, 0x66, 0x66, 1, 2, 3, 4, 0xee, 0xee,
            0x66, 0x66, 0x66, 0x66, 5, 6, 7, 8,
        ]);
    }

    #[test]
    fn four_line_blocks_use_their_own_marker() {
        let codebook: Vec<u8> = (0..32).collect();
        // Block 1, then 0x0f05 which is past the codebook, not solid.
        let mut buffer = [0xee; 8 * 4];
        decode_frame(&codebook, &[1, 5, 0, 0x0f], &mut buffer, 2, 1, 8, 4, 0xff);
        #[rustfmt::skip]
        assert_eq!(buffer, [
            16, 17, 18, 19, 0xee, 0xee, 0xee, 0xee,
            20, 21, 22, 23, 0xee, 0xee, 0xee, 0xee,
            24, 25, 26, 27, 0xee, 0xee, 0xee, 0xee,
            28, 29, 30, 31, 0xee, 0xee, 0xee, 0xee,
        ]);
        let mut buffer = [0xee; 4 * 4];
        decode_frame(&codebook, &[9, 0xff], &mut buffer, 1, 1, 4, 4, 0xff);
        assert_eq!(buffer, [9; 16]);
    }

    #[test]
    fn an_entry_past_the_codebook_stops_the_decode() {
        // Block 0, block 2 (past the end), block 1; then a second row.
        let pointers = [0, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0];
        let mut buffer = [0xee; 12 * 4];
        decode_4x2(&pointers, &mut buffer, 3, 2, 12);
        let mut want = [0xee; 12 * 4];
        want[..4].copy_from_slice(&[1, 2, 3, 4]);
        want[12..16].copy_from_slice(&[5, 6, 7, 8]);
        assert_eq!(buffer, want);
    }

    #[test]
    fn sizes_that_do_not_fit_draw_nothing() {
        let pointers = [0; 8];
        let untouched = [0xee; 32];
        for (columns, rows, stride, buffer_len, pointer_len) in [
            (0, 1, 8, 32, 8), // No columns.
            (1, 0, 8, 32, 8), // No rows.
            (3, 1, 8, 32, 8), // Wider than the stride.
            (2, 2, 8, 32, 6), // Too few pointers.
            (2, 2, 8, 31, 8), // The last pixel row is a byte short.
            (2, 1, 8, 7, 8),  // Not one row of blocks.
            (1, 1, 0, 32, 8), // No stride.
        ] {
            let mut buffer = untouched;
            decode_4x2(
                &pointers[..pointer_len],
                &mut buffer[..buffer_len],
                columns,
                rows,
                stride,
            );
            assert_eq!(buffer, untouched, "{columns}x{rows}, stride {stride}");
        }
        let mut buffer = untouched;
        decode_frame(&CODEBOOK, &pointers, &mut buffer, 1, 1, 8, 0, 0x0f);
        assert_eq!(buffer, untouched);
    }

    #[test]
    fn the_last_pixel_row_needs_only_its_blocks() {
        let mut buffer = [0xee; 8 * 3 + 4];
        decode_4x2(&[0, 1, 0, 0], &mut buffer, 1, 2, 8);
        assert_eq!(buffer[24..], [15, 16, 17, 18]);
    }
}
