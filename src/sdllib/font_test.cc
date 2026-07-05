#include "sdllib/font.h"

#include <cstdint>
#include <cstring>
#include <vector>

#include "gtest/gtest.h"

namespace {

// Writes a little-endian uint16 into the blob at the given byte offset.
void WriteWord(std::vector<uint8_t>& blob, int offset, uint16_t value) {
  std::memcpy(blob.data() + offset, &value, sizeof(value));
}

// Builds a minimal two-glyph font blob:
//   header      @ 0   (14 bytes)
//   info block  @ 14  (6 bytes; max height 8, max width 5)
//   offset table@ 20  (2 x uint16 glyph data offsets)
//   width table @ 24  (2 x uint8 glyph widths: 3 and 4)
//   height table@ 27  (2 x uint16 packed heights; odd offset on purpose to
//                      exercise the unaligned-read path)
//   glyph data  @ 31
std::vector<uint8_t> MakeTestFont() {
  std::vector<uint8_t> blob(40, 0);

  // Header.
  WriteWord(blob, 0, 40);   // size
  blob[2] = 0;              // compression
  blob[3] = 5;              // num_blocks
  WriteWord(blob, 4, 14);   // info_block
  WriteWord(blob, 6, 20);   // offset_block
  WriteWord(blob, 8, 24);   // width_block
  WriteWord(blob, 10, 31);  // data_block
  WriteWord(blob, 12, 27);  // height_block

  // Info block: max glyph height 8, max glyph width 5.
  blob[14 + kFontInfoMaxHeight] = 8;
  blob[14 + kFontInfoMaxWidth] = 5;

  // Offset table: glyph 0 data at 31, glyph 1 data at 35.
  WriteWord(blob, 20, 31);
  WriteWord(blob, 22, 35);

  // Width table.
  blob[24] = 3;
  blob[25] = 4;

  // Height table (unaligned): packed as (drawn rows << 8) | blank rows above.
  WriteWord(blob, 27, (6 << 8) | 2);  // glyph 0: 6 rows drawn, 2 blank above
  WriteWord(blob, 29, (8 << 8) | 0);  // glyph 1: 8 rows drawn, 0 blank above

  // Glyph data: arbitrary marker bytes.
  blob[31] = 0xAB;
  blob[35] = 0xCD;

  return blob;
}

TEST(FontHeaderTest, MatchesOnDiskLayout) {
  const std::vector<uint8_t> blob = MakeTestFont();

  FontHeader header;
  std::memcpy(&header, blob.data(), sizeof(header));

  EXPECT_EQ(header.size, 40);
  EXPECT_EQ(header.compression, 0);
  EXPECT_EQ(header.num_blocks, 5);
  EXPECT_EQ(header.info_block, 14);
  EXPECT_EQ(header.offset_block, 20);
  EXPECT_EQ(header.width_block, 24);
  EXPECT_EQ(header.data_block, 31);
  EXPECT_EQ(header.height_block, 27);
}

TEST(FontViewTest, FontWideMetrics) {
  const std::vector<uint8_t> blob = MakeTestFont();
  const FontView font(blob.data());

  EXPECT_EQ(font.MaxHeight(), 8);
  EXPECT_EQ(font.MaxWidth(), 5);
}

TEST(FontViewTest, GlyphWidths) {
  const std::vector<uint8_t> blob = MakeTestFont();
  const FontView font(blob.data());

  EXPECT_EQ(font.GlyphWidth(0), 3);
  EXPECT_EQ(font.GlyphWidth(1), 4);
}

TEST(FontViewTest, GlyphHeightsFromUnalignedTable) {
  const std::vector<uint8_t> blob = MakeTestFont();
  const FontView font(blob.data());

  EXPECT_EQ(font.GlyphHeight(0), 6);
  EXPECT_EQ(font.GlyphBlankRowsAbove(0), 2);
  EXPECT_EQ(font.GlyphHeight(1), 8);
  EXPECT_EQ(font.GlyphBlankRowsAbove(1), 0);
}

TEST(FontViewTest, GlyphDataPointsIntoBlob) {
  const std::vector<uint8_t> blob = MakeTestFont();
  const FontView font(blob.data());

  EXPECT_EQ(font.GlyphData(0), blob.data() + 31);
  EXPECT_EQ(*font.GlyphData(0), 0xAB);
  EXPECT_EQ(font.GlyphData(1), blob.data() + 35);
  EXPECT_EQ(*font.GlyphData(1), 0xCD);
}

}  // namespace
