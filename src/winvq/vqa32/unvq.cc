#include "winvq/vqa32/unvq.h"

#include <cstdint>

#include "base/types.h"
#include "port/unaligned.h"

void UnVQ_4x2(const unsigned char* codebook, const unsigned char* pointers,
              unsigned char* buffer, int blocksperrow, int numrows,
              int bufwidth) {
  // Pointer offsets are computed in ptrdiff_t so row multiples cannot overflow.
  const base::ssize stride = bufwidth;

  // Compute the offset to the next row of blocks
  const base::ssize rowoffset = stride * 2;

  // Compute the end address of the pointer data
  const auto entries = numrows * blocksperrow;
  const auto* data_end = pointers + entries;

  const auto* src_ptr = pointers;
  auto* dst_ptr = buffer;
  auto* dst_ptr_start = dst_ptr;

  // Drawing loop
  do {
    int count = blocksperrow;  // Number of blocks in a line
    do {
      const uint8_t v = *src_ptr;
      const int cb = src_ptr[entries];  // Get the codebook pointer value
      src_ptr++;

      if (cb == 0xF)  // Is it a one color block?
      {
        // Draw 1-color block
        const uint32_t col32 = v * 0x01010101U;  // Duplicate colour
        port::WriteUnaligned(dst_ptr, col32);  // Write 1st row to dest
        port::WriteUnaligned(dst_ptr + stride,
                             col32);  // Write 2st row to dest
      } else {
        // Draw multi-color block
        const int index = ((cb * 256) + v) * 8;
        const auto row1 = port::ReadUnaligned<uint32_t>(
            codebook + index);  // Read 1st row of codeword
        const auto row2 = port::ReadUnaligned<uint32_t>(
            codebook + index + 4);            // Read 2nd row of codeword
        port::WriteUnaligned(dst_ptr, row1);  // Write 1st row to dest
        port::WriteUnaligned(dst_ptr + stride,
                             row2);  // Write 2st row to dest
      }

      dst_ptr += 4;
    } while (--count);

    dst_ptr = dst_ptr_start + rowoffset;
    dst_ptr_start = dst_ptr;
  } while (src_ptr < data_end);
}

void UnVQ_4x4(const unsigned char* codebook, const unsigned char* pointers,
              unsigned char* buffer, int blocksperrow, int numrows,
              int bufwidth) {
  // Pointer offsets are computed in ptrdiff_t so row multiples cannot overflow.
  const base::ssize stride = bufwidth;

  // Compute the offset to the next row of blocks
  const base::ssize rowoffset = stride * 4;

  // Compute the end address of the pointer data
  const auto entries = numrows * blocksperrow;
  const auto* data_end = pointers + entries;

  const auto* src_ptr = pointers;
  auto* dst_ptr = buffer;
  auto* dst_ptr_start = dst_ptr;

  // Drawing loop
  do {
    int count = blocksperrow;  // Number of blocks in a line
    do {
      const uint8_t v = *src_ptr;
      const int cb = src_ptr[entries];  // Get the codebook pointer value
      src_ptr++;

      if (cb == 0xFF)  // Is it a one color block?
      {
        // Draw 1-color block
        const uint32_t col32 = v * 0x01010101U;  // Duplicate colour
        port::WriteUnaligned(dst_ptr, col32);  // Write 1st row to dest
        port::WriteUnaligned(dst_ptr + stride,
                             col32);  // Write 2nd row to dest
        port::WriteUnaligned(dst_ptr + (stride * 2),
                             col32);  // Write 3rd row to dest
        port::WriteUnaligned(dst_ptr + (stride * 3),
                             col32);  // Write 4th row to dest
      } else {
        // Draw multi-color block
        const int index = ((cb * 256) + v) * 16;
        const auto row1 = port::ReadUnaligned<uint32_t>(
            codebook + index);  // Read 1st row of codeword
        const auto row2 = port::ReadUnaligned<uint32_t>(
            codebook + index + 4);  // Read 2nd row of codeword
        const auto row3 = port::ReadUnaligned<uint32_t>(
            codebook + index + 8);  // Read 3rd row of codeword
        const auto row4 = port::ReadUnaligned<uint32_t>(
            codebook + index + 12);  // Read 4th row of codeword

        port::WriteUnaligned(dst_ptr, row1);  // Write 1st row to dest
        port::WriteUnaligned(dst_ptr + stride,
                             row2);  // Write 2nd row to dest
        port::WriteUnaligned(dst_ptr + (stride * 2),
                             row3);  // Write 3rt row to dest
        port::WriteUnaligned(dst_ptr + (stride * 3),
                             row4);  // Write 4th row to dest
      }

      dst_ptr += 4;
    } while (--count != 0);

    dst_ptr = dst_ptr_start + rowoffset;
    dst_ptr_start = dst_ptr;
  } while (src_ptr < data_end);
}
