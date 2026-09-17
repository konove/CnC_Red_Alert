// Exercise PCX byte-read failures through the real file and graphics classes.
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <memory>
#include <span>
#include <string>
#include <system_error>

#include "base/array.h"
#include "gtest/gtest.h"
#include "sdllib/gbuffer.h"
#include "tech/pcx_file.h"

namespace {

// A two-pixel, one-row PCX file, removed when the test finishes.
class PcxFile {
 public:
  PcxFile(std::span<const uint8_t> pixels, bool padded) {
    path_ = std::filesystem::temp_directory_path() /
            (std::string("cnc_pcx_") +
             testing::UnitTest::GetInstance()->current_test_info()->name() +
             ".pcx");
    std::array<char, 128> header{};
    header.at(0) = 10;  // PCX identifier.
    header.at(1) = 5;   // Version.
    header.at(2) = 1;   // RLE encoding.
    header.at(3) = 8;   // Bits per pixel.
    header.at(8) = 1;   // Inclusive right edge: width is two pixels.
    header.at(65) = 1;  // One color plane.
    header.at(66) = padded ? 4 : 2;  // Bytes per scanline.
    std::ofstream out(path_, std::ios::binary);
    out.write(header.data(), static_cast<std::streamsize>(header.size()));
    for (const uint8_t pixel : pixels) {
      out.put(static_cast<char>(pixel));
    }
    EXPECT_TRUE(out.good());
  }

  PcxFile(const PcxFile&) = delete;
  PcxFile& operator=(const PcxFile&) = delete;

  ~PcxFile() {
    std::error_code ignored;
    std::filesystem::remove(path_, ignored);
  }
  PcxFile(PcxFile&&) = delete;
  PcxFile& operator=(PcxFile&&) = delete;

  // Load without a palette so EOF is exactly the end of the encoded pixels.
  [[nodiscard]] std::unique_ptr<GraphicBufferClass> Load() const {
    return std::unique_ptr<GraphicBufferClass>(
        Read_PCX_File(path_.string().c_str(), {}, {}, 0));
  }

 private:
  std::filesystem::path path_;
};

TEST(PcxTest, RejectsMissingFirstPixel) {
  for (const bool padded : {false, true}) {
    EXPECT_EQ(PcxFile({}, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsTruncatedLiteralPixels) {
  constexpr std::array<uint8_t, 1> pixels{7};
  for (const bool padded : {false, true}) {
    EXPECT_EQ(PcxFile(pixels, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsMissingRunColor) {
  constexpr std::array<uint8_t, 1> pixels{194};
  for (const bool padded : {false, true}) {
    EXPECT_EQ(PcxFile(pixels, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsMissingPixelAfterRun) {
  constexpr std::array<uint8_t, 2> pixels{193, 7};
  for (const bool padded : {false, true}) {
    EXPECT_EQ(PcxFile(pixels, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsMissingTrailingScanlineData) {
  constexpr std::array<uint8_t, 2> pixels{194, 7};
  EXPECT_EQ(PcxFile(pixels, true).Load(), nullptr);
}

TEST(PcxTest, RejectsMissingTrailingRunColor) {
  constexpr std::array<uint8_t, 3> pixels{194, 7, 193};
  EXPECT_EQ(PcxFile(pixels, true).Load(), nullptr);
}

TEST(PcxTest, DecodesLiteralPixels) {
  constexpr std::array<uint8_t, 4> pixels{7, 8, 0, 0};
  for (const bool padded : {false, true}) {
    const auto image =
        PcxFile(std::span(pixels).first(padded ? 4 : 2), padded).Load();
    ASSERT_NE(image, nullptr);
    const auto decoded = image->Get_Bytes();
    EXPECT_EQ(base::At(decoded, 0), 7);
    EXPECT_EQ(base::At(decoded, 1), 8);
  }
}

TEST(PcxTest, DecodesRepeatedPixels) {
  constexpr std::array<uint8_t, 4> pixels{194, 7, 194, 0};
  for (const bool padded : {false, true}) {
    const auto image = PcxFile(std::span(pixels).first(padded ? 4 : 2), padded).Load();
    ASSERT_NE(image, nullptr);
    const auto decoded = image->Get_Bytes();
    EXPECT_EQ(base::At(decoded, 0), 7);
    EXPECT_EQ(base::At(decoded, 1), 7);
  }
}

TEST(PcxTest, RejectsRunsCrossingRowBoundary) {
  constexpr std::array<uint8_t, 2> pixels{195, 7};
  EXPECT_EQ(PcxFile(pixels, false).Load(), nullptr);
}

TEST(PcxTest, RejectsZeroLengthRuns) {
  constexpr std::array<uint8_t, 4> pixels{192, 7, 7, 8};
  EXPECT_EQ(PcxFile(pixels, false).Load(), nullptr);
}

TEST(PcxTest, RejectsTruncatedPadding) {
  constexpr std::array<uint8_t, 3> pixels{7, 8, 0};
  EXPECT_EQ(PcxFile(pixels, true).Load(), nullptr);
}

}  // namespace
