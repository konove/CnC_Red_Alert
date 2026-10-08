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
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "gtest/gtest.h"

namespace {

// The size of the header a PCX file starts with.
constexpr std::ptrdiff_t kHeaderSize = 128;

// Writes `view` as a PCX file in the temporary directory and returns the
// file's bytes, or nothing if no file was written.
std::vector<uint8_t> WrittenFile(PixelView& view,
                                 const engine::gfx::Palette& palette) {
  const std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      (std::string("cnc_pcx_file_") +
       testing::UnitTest::GetInstance()->current_test_info()->name() + ".pcx");
  std::error_code ignored;
  std::filesystem::remove(path, ignored);

  Write_PCX_File(path.string().c_str(), view, palette);

  std::ifstream in(path, std::ios::binary);
  std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(in),
                             std::istreambuf_iterator<char>()};
  in.close();
  std::filesystem::remove(path, ignored);
  return bytes;
}

// The encoded scanlines of a written file: what lies between the header and
// the palette with its marker byte.
std::vector<uint8_t> Scanlines(const std::vector<uint8_t>& file) {
  const std::ptrdiff_t trailer = 1 + engine::gfx::Palette::kFileSize;
  if (std::ssize(file) < kHeaderSize + trailer) {
    return {};
  }
  return {file.begin() + kHeaderSize, file.end() - trailer};
}

// A surface of plain memory whose rows are `pitch` bytes apart.
class PaddedSurface final : public PixelSurface {
 public:
  PaddedSurface(int pitch, std::vector<uint8_t> pixels)
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
