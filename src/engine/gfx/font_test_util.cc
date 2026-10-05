#include "engine/gfx/font_test_util.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"

namespace {

constexpr int kGlyphCount = 256;

// Where each table sits in the file. The tables carry no length, so only
// their order and sizes matter: the header, the info block, then an offset
// (2 bytes), a width (1 byte) and a packed height (2 bytes) per glyph.
constexpr int kInfoBlock = sizeof(FontHeader);
constexpr int kOffsetBlock = kInfoBlock + 6;
constexpr int kWidthBlock = kOffsetBlock + (2 * kGlyphCount);
constexpr int kHeightBlock = kWidthBlock + kGlyphCount;
constexpr int kDataBlock = kHeightBlock + (2 * kGlyphCount);

void WriteWord(const std::span<std::byte> blob, const int offset,
               const int value) {
  const auto word = static_cast<uint16_t>(value);
  base::CopyBytes(blob.subspan(base::ToSize(offset)), base::ObjectBytes(word),
                  sizeof(word));
}

}  // namespace

std::vector<std::byte> MakeBlockFont(const int width, const int height) {
  CHECK(width >= 1 && width <= 16);
  CHECK(height >= 1 && height <= 16);

  // Two pixels to a byte, each row starting on a byte boundary.
  const int glyph_bytes = ((width + 1) / 2) * height;
  const int size = kDataBlock + glyph_bytes;
  std::vector<std::byte> blob(base::ToSize(size), std::byte{0});

  WriteWord(blob, 0, size);
  blob.at(3) = std::byte{5};  // num_blocks
  WriteWord(blob, 4, kInfoBlock);
  WriteWord(blob, 6, kOffsetBlock);
  WriteWord(blob, 8, kWidthBlock);
  WriteWord(blob, 10, kDataBlock);
  WriteWord(blob, 12, kHeightBlock);

  blob.at(base::ToSize(kInfoBlock + kFontInfoMaxHeight)) =
      static_cast<std::byte>(height);
  blob.at(base::ToSize(kInfoBlock + kFontInfoMaxWidth)) =
      static_cast<std::byte>(width);

  // Every glyph is the one block of pixels at the end of the file.
  for (int glyph = 0; glyph < kGlyphCount; ++glyph) {
    WriteWord(blob, kOffsetBlock + (2 * glyph), kDataBlock);
    blob.at(base::ToSize(kWidthBlock + glyph)) = static_cast<std::byte>(width);
    // `height` rows drawn, none blank above.
    WriteWord(blob, kHeightBlock + (2 * glyph), height * 256);
  }

  // Glyph palette index 1 in both pixels of every byte.
  for (int i = 0; i < glyph_bytes; ++i) {
    blob.at(base::ToSize(kDataBlock + i)) = std::byte{0x11};
  }
  return blob;
}
