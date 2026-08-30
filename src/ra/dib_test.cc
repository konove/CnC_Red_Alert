#include "ra/dib.h"

#include <cstdint>
#include <vector>

#include "gtest/gtest.h"

namespace {

void PutU16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
  out.push_back(static_cast<std::uint8_t>(value >> 8U));
}

void PutU32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<std::uint8_t>((value >> (8U * i)) & 0xFFU));
  }
}

// A minimal 8-bit BMP. `width` of 3 exercises the row padding, since a row of
// three bytes is stored as four.
std::vector<std::uint8_t> MakeBmp(int width, int height, int colors = 2,
                                  std::uint16_t signature = 0x4D42,
                                  std::uint16_t bit_count = 8,
                                  std::uint32_t compression = 0) {
  const int stride = (width + 3) & ~3;
  const std::uint32_t table_bytes = static_cast<std::uint32_t>(colors) * 4U;
  const std::uint32_t bits_offset = 14U + 40U + table_bytes;

  std::vector<std::uint8_t> bmp;
  PutU16(bmp, signature);
  PutU32(bmp, bits_offset + static_cast<std::uint32_t>(
                                stride * (height > 0 ? height : 0)));
  PutU16(bmp, 0);
  PutU16(bmp, 0);
  PutU32(bmp, bits_offset);

  PutU32(bmp, 40);  // biSize
  PutU32(bmp, static_cast<std::uint32_t>(width));
  PutU32(bmp, static_cast<std::uint32_t>(height));
  PutU16(bmp, 1);  // biPlanes
  PutU16(bmp, bit_count);
  PutU32(bmp, compression);
  PutU32(bmp, 0);  // biSizeImage
  PutU32(bmp, 0);  // biXPelsPerMeter
  PutU32(bmp, 0);  // biYPelsPerMeter
  PutU32(bmp, static_cast<std::uint32_t>(colors));
  PutU32(bmp, 0);  // biClrImportant

  for (int i = 0; i < colors; ++i) {
    bmp.push_back(static_cast<std::uint8_t>(i));      // blue
    bmp.push_back(static_cast<std::uint8_t>(i + 1));  // green
    bmp.push_back(static_cast<std::uint8_t>(i + 2));  // red
    bmp.push_back(0);
  }

  for (int row = 0; row < height; ++row) {
    for (int column = 0; column < stride; ++column) {
      bmp.push_back(static_cast<std::uint8_t>(column < width ? row + 1 : 0));
    }
  }
  return bmp;
}

TEST(DibTest, ReadsDimensionsColoursAndPixels) {
  const std::vector<std::uint8_t> bmp = MakeBmp(3, 2);

  const auto image = dib::Image::FromBmp(bmp);

  ASSERT_TRUE(image.has_value());
  EXPECT_EQ(image->Width(), 3);
  EXPECT_EQ(image->Height(), 2);
  EXPECT_EQ(image->Stride(), 4) << "rows pad out to four bytes";

  ASSERT_EQ(image->Colors().size(), 2U);
  EXPECT_EQ(image->Colors()[1], (dib::Color{1, 2, 3, 0}));

  // Bottom row first, as stored.
  ASSERT_EQ(image->Bits().size(), 8U);
  EXPECT_EQ(image->Bits()[0], 1);
  EXPECT_EQ(image->Bits()[4], 2);
}

TEST(DibTest, ColoursCanBeRemappedInPlace) {
  const std::vector<std::uint8_t> bmp = MakeBmp(1, 1);
  auto image = dib::Image::FromBmp(bmp);
  ASSERT_TRUE(image.has_value());

  image->MutableColors()[0] = dib::Color{9, 9, 9, 0};

  EXPECT_EQ(image->Colors()[0], (dib::Color{9, 9, 9, 0}));
}

TEST(DibTest, RejectsWhatItCannotDraw) {
  EXPECT_FALSE(dib::Image::FromBmp({}).has_value());

  // Not a BMP.
  EXPECT_FALSE(dib::Image::FromBmp(MakeBmp(1, 1, 2, 0x4142)).has_value());
  // Not 8 bits per pixel.
  EXPECT_FALSE(dib::Image::FromBmp(MakeBmp(1, 1, 2, 0x4D42, 24)).has_value());
  // Compressed.
  EXPECT_FALSE(dib::Image::FromBmp(MakeBmp(1, 1, 2, 0x4D42, 8, 1)).has_value());
  // Top-down (negative height).
  EXPECT_FALSE(dib::Image::FromBmp(MakeBmp(1, -1)).has_value());
}

TEST(DibTest, RejectsTruncatedPixelData) {
  std::vector<std::uint8_t> bmp = MakeBmp(4, 4);
  bmp.resize(bmp.size() - 1);

  EXPECT_FALSE(dib::Image::FromBmp(bmp).has_value());
}

TEST(DibTest, RejectsTruncatedColourTable) {
  std::vector<std::uint8_t> bmp = MakeBmp(1, 1, 256);
  bmp.resize(14 + 40 + 4);

  EXPECT_FALSE(dib::Image::FromBmp(bmp).has_value());
}

}  // namespace
