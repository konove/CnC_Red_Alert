#include "engine/video/vqa/vq_decoder.h"

#include <optional>
#include <span>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

std::optional<BlockShape> BlockShapeFor(const int width, const int height) {
  if (width == 4 && height == 2) {
    return BlockShape::k4x2;
  }
  if (width == 4 && height == 4) {
    return BlockShape::k4x4;
  }
  return std::nullopt;
}

// The decoder is engine_rs's vq::decode_frame (src/rust/src/vq.rs), which
// checks the sizes against the spans.
void DecodeVqFrame(const BlockShape shape,
                   const std::span<const unsigned char> codebook,
                   const std::span<const unsigned char> pointers,
                   const std::span<unsigned char> buffer,
                   const int blocks_per_row, const int block_rows,
                   const int stride) {
  if (blocks_per_row <= 0 || block_rows <= 0 || stride <= 0) {
    return;
  }
  const auto codebook_bytes = std::as_bytes(codebook);
  const auto pointer_bytes = std::as_bytes(pointers);
  const auto pixels = std::as_writable_bytes(buffer);
  // Rust may not see one byte through a shared and a mutable slice at once.
  // A movie's codebook, pointers and image are separate buffers.
  CHECK(!Overlaps(codebook_bytes, pixels) && !Overlaps(pointer_bytes, pixels))
      << "VQ frame decoded onto its own codebook or pointers";
  // A reserved high byte marks a solid block of the color in the low byte.
  const bool is_4x2 = shape == BlockShape::k4x2;
  engine_rs::vq_decode_frame(AsRustSlice(codebook_bytes),
                             AsRustSlice(pointer_bytes), AsRustSlice(pixels),
                             base::ToSize(blocks_per_row),
                             base::ToSize(block_rows), base::ToSize(stride),
                             is_4x2 ? 2 : 4, is_4x2 ? 0x0f : 0xff);
}
