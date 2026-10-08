// Tests for the PCX writer: the bytes of the file it produces.

#include "engine/gfx/pcx_file.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/file/game_files_test_util.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "gtest/gtest.h"

namespace engine::gfx {

namespace {

constexpr std::ptrdiff_t kHeaderSize = sizeof(PcxHeader);

// Writes `view` as a PCX file in the test's own directory and returns the
// file's bytes, or nothing if no file was written.
std::vector<uint8_t> WrittenFile(PixelView& view, const Palette& palette) {
  engine::file::TestGameFiles files;
  const std::filesystem::path path = files.Path("picture.pcx");

  WritePcxFile(path.string(), view, palette);

  std::ifstream in(path, std::ios::binary);
  return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

// The header of a written file, or nothing if the file is shorter than one.
std::vector<uint8_t> Header(const std::vector<uint8_t>& file) {
  if (std::ssize(file) < kHeaderSize) {
    return {};
  }
  return {file.begin(), file.begin() + kHeaderSize};
}

// The encoded scanlines of a written file: what lies between the header and
// the palette with its marker byte.
std::vector<uint8_t> Scanlines(const std::vector<uint8_t>& file) {
  constexpr std::ptrdiff_t kTrailerSize = 1 + Palette::kFileSize;
  if (std::ssize(file) < kHeaderSize + kTrailerSize) {
    return {};
  }
  return {file.begin() + kHeaderSize, file.end() - kTrailerSize};
}

// A memory picture `width` pixels wide holding `pixels`, row after row.
class Picture {
 public:
  Picture(const int width, std::vector<uint8_t> pixels)
      : pixels_(std::move(pixels)),
        buffer_(width, static_cast<int>(std::ssize(pixels_)) / width, pixels_) {
  }

  PixelView& view() ABSL_ATTRIBUTE_LIFETIME_BOUND { return buffer_.view(); }

 private:
  std::vector<uint8_t> pixels_;
  PixelBuffer buffer_;
};

// The scanlines a picture of one row of `pixels` is written as.
std::vector<uint8_t> EncodedRow(std::vector<uint8_t> pixels) {
  const int width = static_cast<int>(std::ssize(pixels));
  Picture picture(width, std::move(pixels));
  return Scanlines(WrittenFile(picture.view(), {}));
}

TEST(PcxFileTest, WritesTheHeaderOfAnEightBitPicture) {
  Picture picture(5, std::vector<uint8_t>(15));

  const std::vector<uint8_t> file = WrittenFile(picture.view(), {});

  std::vector<uint8_t> expected(kHeaderSize);
  expected.at(0) = 10;     // PCX identifier.
  expected.at(1) = 5;      // Version.
  expected.at(2) = 1;      // Run-length encoded.
  expected.at(3) = 8;      // Bits a pixel.
  expected.at(8) = 4;      // Right edge, inclusive.
  expected.at(10) = 2;     // Bottom edge, inclusive.
  expected.at(12) = 0x40;  // Device resolution: 320 ...
  expected.at(13) = 0x01;
  expected.at(14) = 200;  // ... by 200.
  expected.at(65) = 1;    // Color planes.
  expected.at(66) = 5;    // Bytes a scanline.
  expected.at(68) = 1;    // Color palette.
  EXPECT_EQ(Header(file), expected);
}

TEST(PcxFileTest, WritesRunsAndLonePixels) {
  EXPECT_EQ(EncodedRow({9, 9, 9, 7, 1, 2, 2}),
            (std::vector<uint8_t>{0xC3, 9, 7, 1, 0xC2, 2}));
}

// A byte with both top bits set would be read as a run count.
TEST(PcxFileTest, WritesALonePixelOfAHighColorAsARunOfOne) {
  EXPECT_EQ(EncodedRow({0xC5, 0xBF, 0xFF, 1}),
            (std::vector<uint8_t>{0xC1, 0xC5, 0xBF, 0xC1, 0xFF, 1}));
}

// A count holds six bits, so a run stops at 63 pixels.
TEST(PcxFileTest, SplitsRunsLongerThanACountHolds) {
  EXPECT_EQ(EncodedRow(std::vector<uint8_t>(63, 4)),
            (std::vector<uint8_t>{0xFF, 4}));
  EXPECT_EQ(EncodedRow(std::vector<uint8_t>(64, 4)),
            (std::vector<uint8_t>{0xFF, 4, 4}));
  EXPECT_EQ(EncodedRow(std::vector<uint8_t>(130, 0xC4)),
            (std::vector<uint8_t>{0xFF, 0xC4, 0xFF, 0xC4, 0xC4, 0xC4}));
}

// Every row is encoded on its own: a run does not carry on into the next.
TEST(PcxFileTest, EndsARunAtTheEndOfARow) {
  Picture picture(2, {7, 7,  //
                      7, 7});

  EXPECT_EQ(Scanlines(WrittenFile(picture.view(), {})),
            (std::vector<uint8_t>{0xC2, 7, 0xC2, 7}));
}

TEST(PcxFileTest, WritesOnlyThePartOfThePageAViewCovers) {
  Picture picture(4, {1, 2, 3, 4,  //
                      5, 6, 7, 8,  //
                      9, 10, 11, 12});
  PixelView view(picture.view().buffer(), 1, 1, 2, 2);

  const std::vector<uint8_t> file = WrittenFile(view, {});

  EXPECT_EQ(Scanlines(file), (std::vector<uint8_t>{6, 7, 10, 11}));
  const std::vector<uint8_t> header = Header(file);
  ASSERT_FALSE(header.empty());
  EXPECT_EQ(header.at(8), 1);   // Right edge.
  EXPECT_EQ(header.at(10), 1);  // Bottom edge.
  EXPECT_EQ(header.at(66), 2);  // Bytes a scanline.
}

// The palette closes the file, 8 bits a component, behind a byte of 12.
TEST(PcxFileTest, EndsWithThePalette) {
  Picture picture(1, {0});
  Palette palette;
  palette.at(1) = PaletteColor(255, 80, 3);
  palette.at(255) = PaletteColor(1, 2, 3);

  const std::vector<uint8_t> file = WrittenFile(picture.view(), palette);

  const auto rgb = palette.ToRgb();
  std::vector<uint8_t> expected{0, 12};
  expected.insert(expected.end(), rgb.begin(), rgb.end());
  ASSERT_GE(std::ssize(file), kHeaderSize);
  EXPECT_EQ(std::vector<uint8_t>(file.begin() + kHeaderSize, file.end()),
            expected);
}

// A view can be squeezed to no width at all; its rows then encode to nothing.
TEST(PcxFileTest, WritesNoScanlinesForAViewWithoutWidth) {
  Picture picture(4, std::vector<uint8_t>(8, 7));
  PixelView view(picture.view().buffer(), 0, 0, 0, 2);
  ASSERT_EQ(view.width(), 0);
  ASSERT_EQ(view.height(), 2);

  const std::vector<uint8_t> file = WrittenFile(view, {});

  EXPECT_EQ(std::ssize(file), kHeaderSize + 1 + Palette::kFileSize);
}

TEST(PcxFileTest, ReportsWhetherTheFileWasWritten) {
  Picture picture(1, {0});
  engine::file::TestGameFiles files;

  EXPECT_TRUE(
      WritePcxFile(files.Path("picture.pcx").string(), picture.view(), {}));
  // A file cannot be created in a directory that does not exist.
  EXPECT_FALSE(WritePcxFile(files.Path("missing/picture.pcx").string(),
                            picture.view(), {}));
  PixelView unattached;
  EXPECT_FALSE(
      WritePcxFile(files.Path("unattached.pcx").string(), unattached, {}));
}

// A surface of plain memory whose rows are `pitch` bytes apart.
class PaddedSurface final : public PixelSurface {
 public:
  PaddedSurface(const int pitch, std::vector<uint8_t> pixels)
      : pixels_(std::move(pixels)), pitch_(pitch) {}

  std::optional<Pixels> Lock() override {
    return Pixels{.bytes = pixels_, .pitch = pitch_};
  }
  void Unlock() override {}

 private:
  std::vector<uint8_t> pixels_;
  int pitch_;
};

// The window's page has pixels only while it is locked, and its rows can be
// wider than the page: the writer locks it and steps over the padding.
TEST(PcxFileTest, WritesASurfaceSkippingItsRowPadding) {
  PaddedSurface surface(4, {1, 2, 3, 99,  //
                            4, 5, 6, 99});
  PixelBuffer page;
  page.Init(3, 2, surface);

  const std::vector<uint8_t> file = WrittenFile(page.view(), {});

  EXPECT_EQ(Scanlines(file), (std::vector<uint8_t>{1, 2, 3, 4, 5, 6}));
}

// Screen's views exist before they are attached to a page; there is nothing
// to write for one.
TEST(PcxFileTest, WritesNoFileForAnUnattachedView) {
  PixelView view;

  EXPECT_TRUE(WrittenFile(view, {}).empty());
}

}  // namespace

}  // namespace engine::gfx
