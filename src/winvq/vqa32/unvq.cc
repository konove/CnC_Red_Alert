#include "winvq/vqa32/unvq.h"

#include <algorithm>
#include <cstddef>
#include <span>

#include "base/array.h"

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
  const auto columns = static_cast<size_t>(blocks_per_row);
  const auto rows = static_cast<size_t>(num_rows);
  const auto width = static_cast<size_t>(stride);
  const auto height = static_cast<size_t>(block_height);
  // Two pointer bytes per block, and pixel rows for every block row; the
  // last pixel row needs only its blocks' width, not a whole stride.
  if (rows > pointers.size() / 2 / columns || buffer.size() < columns * 4 ||
      rows > (((buffer.size() - (columns * 4)) / width) + 1) / height) {
    return;
  }
  const auto entries = rows * columns;
  for (size_t row = 0; row < rows; ++row) {
    for (size_t col = 0; col < columns; ++col) {
      const auto entry = (row * columns) + col;
      const auto value = base::At(pointers, entry);
      const auto high = base::At(pointers, entries + entry);
      // A reserved high byte marks a solid block of the color in the low
      // byte.
      const bool solid = high == (block_height == 2 ? 0x0f : 0xff);
      const auto code_offset =
          ((static_cast<size_t>(high) * 256) + value) * 4 * height;
      if (!solid && (code_offset > codebook.size() ||
                     4 * height > codebook.size() - code_offset)) {
        return;
      }
      for (size_t line = 0; line < height; ++line) {
        const auto destination =
            buffer.subspan((((row * height) + line) * width) + (col * 4), 4);
        if (solid) {
          std::ranges::fill(destination, value);
        } else {
          std::ranges::copy(codebook.subspan(code_offset + (line * 4), 4),
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
