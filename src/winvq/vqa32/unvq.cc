#include "winvq/vqa32/unvq.h"

#include <algorithm>
#include <iterator>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "base/types.h"

namespace {
// Decodes blocks 4 pixels wide and block_height lines high. The sizes are
// checked against the spans before anything is drawn. A pointer past the end
// of the codebook shows only when decoding reaches it, so it stops the decode
// there and leaves the rest of the buffer as it was.
void DecodeBlocks(std::span<const unsigned char> codebook,
                  std::span<const unsigned char> pointers,
                  std::span<unsigned char> buffer, int blocks_per_row,
                  int num_rows, int stride, int block_height) {
  if (blocks_per_row <= 0 || num_rows <= 0 || stride <= 0 ||
      blocks_per_row > stride / 4) {
    return;
  }
  const base::ssize columns = blocks_per_row;
  const base::ssize rows = num_rows;
  const base::ssize width = stride;
  const base::ssize height = block_height;
  const base::ssize buffer_bytes = std::ssize(buffer);
  const base::ssize codebook_bytes = std::ssize(codebook);
  // Two pointer bytes per block, and pixel rows for every block row; the
  // last pixel row needs only its blocks' width, not a whole stride.
  if (rows > std::ssize(pointers) / 2 / columns || buffer_bytes < columns * 4 ||
      rows > (((buffer_bytes - (columns * 4)) / width) + 1) / height) {
    return;
  }
  const base::ssize entries = rows * columns;
  for (base::ssize row = 0; row < rows; ++row) {
    for (base::ssize col = 0; col < columns; ++col) {
      const base::ssize entry = (row * columns) + col;
      const auto value = base::At(pointers, entry);
      const auto high = base::At(pointers, entries + entry);
      // A reserved high byte marks a solid block of the color in the low
      // byte.
      const bool solid = high == (block_height == 2 ? 0x0f : 0xff);
      const base::ssize code_offset =
          ((base::ssize{high} * 256) + value) * 4 * height;
      if (!solid && (code_offset > codebook_bytes ||
                     4 * height > codebook_bytes - code_offset)) {
        return;
      }
      for (base::ssize line = 0; line < height; ++line) {
        const auto destination = buffer.subspan(
            base::ToSize((((row * height) + line) * width) + (col * 4)), 4);
        if (solid) {
          std::ranges::fill(destination, value);
        } else {
          std::ranges::copy(
              codebook.subspan(base::ToSize(code_offset + (line * 4)), 4),
              destination.begin());
        }
      }
    }
  }
}
}  // namespace

void UnVQ_4x2(std::span<const unsigned char> codebook,
              std::span<const unsigned char> pointers,
              std::span<unsigned char> buffer, int blocksperrow, int numrows,
              int bufwidth) {
  DecodeBlocks(codebook, pointers, buffer, blocksperrow, numrows, bufwidth, 2);
}

void UnVQ_4x4(std::span<const unsigned char> codebook,
              std::span<const unsigned char> pointers,
              std::span<unsigned char> buffer, int blocksperrow, int numrows,
              int bufwidth) {
  DecodeBlocks(codebook, pointers, buffer, blocksperrow, numrows, bufwidth, 4);
}
