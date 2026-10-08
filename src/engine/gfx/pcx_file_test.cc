// Tests for the PCX writer: the bytes it produces.

#include "engine/gfx/pcx_file.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/file/game_files_test_util.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/palette_color.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/memory_stream.h"
#include "engine/stream/seek_origin.h"
#include "gtest/gtest.h"

namespace engine::gfx {

namespace {

constexpr std::ptrdiff_t kHeaderSize = sizeof(PcxHeader);

// A stream that keeps what is written to it, up to `capacity` bytes.
class RecordingStream final : public ByteStream {
 public:
  explicit RecordingStream(
      const base::ssize capacity = std::numeric_limits<base::ssize>::max())
      : capacity_(capacity) {}

  [[nodiscard]] const std::vector<uint8_t>& bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> /*buffer*/) override { return 0; }
  base::ssize Write(const std::span<const std::byte> buffer) override {
    const base::ssize count =
        std::min(std::ssize(buffer), capacity_ - std::ssize(bytes_));
    for (const std::byte value : buffer.first(base::ToSize(count))) {
      bytes_.push_back(std::to_integer<uint8_t>(value));
    }
    return count;
  }
  base::ssize Seek(base::ssize /*offset*/, SeekOrigin /*origin*/) override {
    return std::ssize(bytes_);
  }
  base::ssize Size() override { return std::ssize(bytes_); }

 private:
  std::vector<uint8_t> bytes_;
  base::ssize capacity_;
};

// The bytes `view` is written as, or nothing if it was not written.
std::vector<uint8_t> WrittenFile(PixelView& view, const Palette& palette) {
  RecordingStream out;
  WritePcx(out, view, palette);
  return out.bytes();
}

// The bytes of the file at `path`, or nothing if there is none.
std::vector<uint8_t> FileBytes(const std::filesystem::path& path) {
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

TEST(PcxFileTest, ReportsWhetherThePictureWasWritten) {
  Picture picture(1, {0});
  RecordingStream out;
  EXPECT_TRUE(WritePcx(out, picture.view(), {}));

  // A stream that fills up inside the header, the scanlines, the palette
  // marker and the palette.
  const base::ssize size = std::ssize(out.bytes());
  for (const base::ssize capacity :
       {kHeaderSize - 1, kHeaderSize, kHeaderSize + 1, size - 1}) {
    RecordingStream full(capacity);
    EXPECT_FALSE(WritePcx(full, picture.view(), {})) << capacity;
  }
}

// A picture is written where the stream stands, after what it already holds.
TEST(PcxFileTest, AppendsToTheStream) {
  Picture picture(1, {0});
  RecordingStream out;
  out.WriteObject(uint8_t{42});

  EXPECT_TRUE(WritePcx(out, picture.view(), {}));

  ASSERT_GT(std::ssize(out.bytes()), 1);
  EXPECT_EQ(out.bytes().at(0), 42);
  EXPECT_EQ(std::vector<uint8_t>(out.bytes().begin() + 1, out.bytes().end()),
            WrittenFile(picture.view(), {}));
}

TEST(PcxFileTest, WritesTheSamePictureToAGameFile) {
  Picture picture(3, {1, 1, 2});
  engine::file::TestGameFiles files;
  const std::filesystem::path path = files.Path("picture.pcx");

  EXPECT_TRUE(WritePcxFile(path.string(), picture.view(), {}));

  EXPECT_EQ(FileBytes(path), WrittenFile(picture.view(), {}));
}

TEST(PcxFileTest, ReportsAFileThatCannotBeCreated) {
  Picture picture(1, {0});
  engine::file::TestGameFiles files;

  // A file cannot be created in a directory that does not exist.
  EXPECT_FALSE(WritePcxFile(files.Path("missing/picture.pcx").string(),
                            picture.view(), {}));
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
TEST(PcxFileTest, WritesNothingForAnUnattachedView) {
  PixelView view;
  RecordingStream out;

  EXPECT_FALSE(WritePcx(out, view, {}));

  EXPECT_TRUE(out.bytes().empty());
}

TEST(PcxFileTest, CreatesNoFileForAnUnattachedView) {
  PixelView view;
  engine::file::TestGameFiles files;
  const std::filesystem::path path = files.Path("unattached.pcx");

  EXPECT_FALSE(WritePcxFile(path.string(), view, {}));

  EXPECT_FALSE(std::filesystem::exists(path));
}

// The header of a `width` by `height` picture at 0,0 whose scanlines decode
// to `bytes_per_line` pixels.
PcxHeader HeaderOf(const int width, const int height,
                   const int bytes_per_line) {
  return {
      .id = 10,
      .version = 5,
      .encoding = 1,
      .bits_per_pixel = 8,
      .left = 0,
      .top = 0,
      .right = static_cast<int16_t>(width - 1),
      .bottom = static_cast<int16_t>(height - 1),
      .horizontal_dpi = 0,
      .vertical_dpi = 0,
      .ega_palette = {},
      .reserved = 0,
      .color_planes = 1,
      .bytes_per_line = static_cast<int16_t>(bytes_per_line),
      .palette_type = 1,
      .padding = {},
  };
}

// The bytes of a PCX file: `header`, `scanlines` as they are, and `palette`
// behind its marker.
std::vector<uint8_t> FileOf(const PcxHeader& header,
                            const std::vector<uint8_t>& scanlines,
                            const Palette& palette = {}) {
  std::vector<uint8_t> file(kHeaderSize);
  std::ranges::copy(std::as_bytes(std::span(&header, 1)),
                    std::as_writable_bytes(std::span(file)).begin());
  file.insert(file.end(), scanlines.begin(), scanlines.end());
  file.push_back(12);
  const auto rgb = palette.ToRgb();
  file.insert(file.end(), rgb.begin(), rgb.end());
  return file;
}

std::optional<PcxPicture> ReadBytes(const std::vector<uint8_t>& file) {
  MemoryStream in(std::as_bytes(std::span(file)));
  return ReadPcx(in);
}

// What a picture of one row of two pixels reads as when its scanline is
// `scanline`: with the row as wide as the picture, and with two bytes of
// padding behind it.
std::vector<std::optional<PcxPicture>> ReadRowOfTwo(
    const std::vector<uint8_t>& scanline) {
  std::vector<std::optional<PcxPicture>> pictures;
  for (const int bytes_per_line : {2, 4}) {
    pictures.push_back(
        ReadBytes(FileOf(HeaderOf(2, 1, bytes_per_line), scanline)));
  }
  return pictures;
}

// The pixels each of `pictures` holds, or nothing for one that did not read.
std::vector<std::vector<uint8_t>> PixelsOf(
    const std::vector<std::optional<PcxPicture>>& pictures) {
  std::vector<std::vector<uint8_t>> pixels;
  pixels.reserve(pictures.size());
  for (const std::optional<PcxPicture>& picture : pictures) {
    pixels.push_back(picture ? picture->pixels : std::vector<uint8_t>{});
  }
  return pixels;
}

TEST(PcxFileTest, ReadsLonePixels) {
  EXPECT_EQ(PixelsOf(ReadRowOfTwo({7, 8, 0, 0})),
            (std::vector<std::vector<uint8_t>>{{7, 8}, {7, 8}}));
}

TEST(PcxFileTest, ReadsRuns) {
  EXPECT_EQ(PixelsOf(ReadRowOfTwo({0xC2, 7, 0xC2, 0})),
            (std::vector<std::vector<uint8_t>>{{7, 7}, {7, 7}}));
}

TEST(PcxFileTest, RejectsARunThatCrossesIntoTheNextRow) {
  EXPECT_FALSE(ReadBytes(FileOf(HeaderOf(2, 1, 2), {0xC3, 7})).has_value());
}

TEST(PcxFileTest, RejectsARunOfNoPixels) {
  EXPECT_FALSE(
      ReadBytes(FileOf(HeaderOf(2, 1, 2), {0xC0, 7, 7, 8})).has_value());
}

// Runs longer than a count holds, lone pixels and lone pixels of a color that
// reads as a count come back as they went out, with the palette.
TEST(PcxFileTest, ReadsWhatWasWritten) {
  constexpr int kWidth = 70;
  std::vector<uint8_t> pixels(base::ToSize(2 * kWidth), 9);
  for (int x = 0; x < kWidth; x++) {
    pixels.at(base::ToSize(kWidth + x)) =
        static_cast<uint8_t>(x % 2 == 0 ? 0xC5 : x);
  }
  Picture picture(kWidth, pixels);
  Palette palette;
  palette.at(1) = PaletteColor(255, 80, 3);

  const std::optional<PcxPicture> read =
      ReadBytes(WrittenFile(picture.view(), palette));

  ASSERT_TRUE(read.has_value());
  EXPECT_EQ(read->width, kWidth);
  EXPECT_EQ(read->height, 2);
  EXPECT_EQ(read->pixels, pixels);
  EXPECT_EQ(read->palette, palette);
}

// The palette's bytes behind a picture that is cut short are not pixels.
TEST(PcxFileTest, RejectsScanlinesThatAreCutShort) {
  const std::vector<std::vector<uint8_t>> cut_short = {
      {},        // No pixel at all.
      {7},       // One pixel of two.
      {0xC2},    // A run without its color.
      {0xC1, 7}  // A run of one and nothing behind it.
  };
  for (const std::vector<uint8_t>& scanline : cut_short) {
    for (const std::optional<PcxPicture>& picture : ReadRowOfTwo(scanline)) {
      EXPECT_FALSE(picture.has_value()) << testing::PrintToString(scanline);
    }
  }
}

TEST(PcxFileTest, RejectsPaddingThatIsCutShort) {
  // The picture's two pixels are there, the scanline's other two are not.
  EXPECT_FALSE(ReadBytes(FileOf(HeaderOf(2, 1, 4), {0xC2, 7})).has_value());
  EXPECT_FALSE(
      ReadBytes(FileOf(HeaderOf(2, 1, 4), {0xC2, 7, 0xC1})).has_value());
}

TEST(PcxFileTest, RejectsAPictureWithoutAPaletteMarker) {
  std::vector<uint8_t> file = FileOf(HeaderOf(2, 1, 2), {7, 8});
  file.at(file.size() - Palette::kFileSize - 1) = 13;

  EXPECT_FALSE(ReadBytes(file).has_value());
}

}  // namespace

}  // namespace engine::gfx
