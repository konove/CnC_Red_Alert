// Exercise PCX byte-read failures through the real file and graphics classes.
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <span>
#include <string>
#include <system_error>

#include "gtest/gtest.h"
#include "sdllib/gbuffer.h"

GraphicBufferClass* Read_PCX_File(const char* name, char* palette, void* buffer,
                                long size);

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
    header[0] = 10;  // PCX identifier.
    header[1] = 5;   // Version.
    header[2] = 1;   // RLE encoding.
    header[3] = 8;   // Bits per pixel.
    header[8] = 1;   // Inclusive right edge: width is two pixels.
    header[65] = 1;  // One color plane.
    header[66] = padded ? 4 : 2;  // Bytes per scanline.
    std::ofstream out(path_, std::ios::binary);
    out.write(header.data(), static_cast<std::streamsize>(header.size()));
    for (uint8_t pixel : pixels) {
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

  // Load without a palette so EOF is exactly the end of the encoded pixels.
  std::unique_ptr<GraphicBufferClass> Load() const {
    return std::unique_ptr<GraphicBufferClass>(
        Read_PCX_File(path_.string().c_str(), nullptr, nullptr, 0));
  }

 private:
  std::filesystem::path path_;
};

TEST(PcxTest, RejectsMissingFirstPixel) {
  for (bool padded : {false, true}) {
    EXPECT_EQ(PcxFile({}, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsTruncatedLiteralPixels) {
  constexpr std::array<uint8_t, 1> pixels{7};
  for (bool padded : {false, true}) {
    EXPECT_EQ(PcxFile(pixels, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsMissingRunColor) {
  constexpr std::array<uint8_t, 1> pixels{194};
  for (bool padded : {false, true}) {
    EXPECT_EQ(PcxFile(pixels, padded).Load(), nullptr);
  }
}

TEST(PcxTest, RejectsMissingPixelAfterRun) {
  constexpr std::array<uint8_t, 2> pixels{193, 7};
  for (bool padded : {false, true}) {
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
  constexpr std::array<uint8_t, 3> pixels{7, 8, 0};
  for (bool padded : {false, true}) {
    const auto image = PcxFile(std::span(pixels).first(padded ? 3 : 2), padded).Load();
    ASSERT_NE(image, nullptr);
    const auto* decoded = static_cast<const uint8_t*>(image->Get_Buffer());
    EXPECT_EQ(decoded[0], 7);
    EXPECT_EQ(decoded[1], 8);
  }
}

TEST(PcxTest, DecodesRepeatedPixels) {
  constexpr std::array<uint8_t, 4> pixels{194, 7, 194, 0};
  for (bool padded : {false, true}) {
    const auto image = PcxFile(std::span(pixels).first(padded ? 4 : 2), padded).Load();
    ASSERT_NE(image, nullptr);
    const auto* decoded = static_cast<const uint8_t*>(image->Get_Buffer());
    EXPECT_EQ(decoded[0], 7);
    EXPECT_EQ(decoded[1], 7);
  }
}

}  // namespace
