#include "engine/gfx/cps_image.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <utility>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "engine/codec/compressed_block.h"
#include "engine/gfx/palette.h"
#include "engine/stream/byte_stream.h"

namespace engine::gfx {

CpsImage ReadCpsImage(ByteStream& file) {
  CpsImage image;
  // The file holds the block's size, then the block: its header, the skipped
  // area, and the data. UncompressBlock() skips the area itself.
  uint16_t block_size = 0;
  CompressedBlockHeader header{};
  if (!file.ReadObject(block_size) || !file.ReadObject(header) ||
      block_size < sizeof(header) || header.skip_bytes < 0 ||
      std::cmp_greater(header.skip_bytes, block_size - sizeof(header)) ||
      std::cmp_greater(header.uncompressed_bytes, kCpsWidth * kCpsHeight)) {
    return image;
  }
  std::vector<std::byte> block(block_size);
  std::ranges::copy(base::ObjectBytes(header), block.begin());
  const auto rest = std::span(block).subspan(sizeof(header));
  if (file.Read(rest) != std::ssize(rest)) {
    return image;
  }

  image.pixels.resize(header.uncompressed_bytes);
  if (UncompressBlock(block, std::as_writable_bytes(std::span(image.pixels))) !=
      std::ssize(image.pixels)) {
    image.pixels.clear();
    return image;
  }

  // A palette shorter than 256 colors holds only the colors it has whole; the
  // bytes past 256 colors are not palette.
  const auto palette_bytes = std::min(base::ToSize(header.skip_bytes),
                                      base::ToSize(Palette::kFileSize));
  std::array<uint8_t, Palette::kFileSize> vga{};
  std::ranges::transform(
      rest.first(palette_bytes), vga.begin(),
      [](std::byte byte) { return std::to_integer<uint8_t>(byte); });
  const Palette palette = Palette::FromVga(vga);
  image.colors.assign(
      palette.colors().begin(),
      std::next(palette.colors().begin(),
                static_cast<std::ptrdiff_t>(palette_bytes / 3)));
  return image;
}

}  // namespace engine::gfx
