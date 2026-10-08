// Tests for Red Alert's PCX writer: the scanlines it produces.

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/base/attributes.h"
#include "base/types.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/seek_origin.h"
#include "gtest/gtest.h"
#include "ra/assets/filepcx.h"

namespace ra {

namespace {

// A stream that keeps what is written to it.
class RecordingStream final : public ByteStream {
 public:
  [[nodiscard]] const std::vector<uint8_t>& bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bytes_;
  }

  using ByteStream::Read;
  using ByteStream::Write;
  base::ssize Read(std::span<std::byte> /*buffer*/) override { return 0; }
  base::ssize Write(const std::span<const std::byte> buffer) override {
    for (const std::byte value : buffer) {
      bytes_.push_back(std::to_integer<uint8_t>(value));
    }
    return std::ssize(buffer);
  }
  base::ssize Seek(base::ssize /*offset*/, SeekOrigin /*origin*/) override {
    return std::ssize(bytes_);
  }
  base::ssize Size() override { return std::ssize(bytes_); }

 private:
  std::vector<uint8_t> bytes_;
};

// The encoded scanlines `pic` is written as: what lies between the header and
// the palette with its marker byte. Empty if nothing was written.
std::vector<uint8_t> WrittenScanlines(PixelBuffer& pic) {
  RecordingStream out;
  Write_PCX_File(out, pic, {});
  constexpr std::ptrdiff_t kHeaderSize = sizeof(PCX_HEADER);
  constexpr std::ptrdiff_t kTrailerSize = 1 + engine::gfx::Palette::kFileSize;
  const std::vector<uint8_t>& file = out.bytes();
  if (std::ssize(file) < kHeaderSize + kTrailerSize) {
    return {};
  }
  return {file.begin() + kHeaderSize, file.end() - kTrailerSize};
}

// A surface of plain memory whose rows are `pitch` bytes apart, and which can
// refuse to lock.
class PaddedSurface final : public PixelSurface {
 public:
  PaddedSurface(const int pitch, std::vector<uint8_t> pixels,
                const bool locks = true)
      : pixels_(std::move(pixels)), pitch_(pitch), locks_(locks) {}

  std::optional<Pixels> Lock() override {
    if (!locks_) {
      return std::nullopt;
    }
    return Pixels{.bytes = pixels_, .pitch = pitch_};
  }
  void Unlock() override {}

 private:
  std::vector<uint8_t> pixels_;
  int pitch_;
  bool locks_;
};

TEST(WritePcxTest, WritesAMemoryBufferRowByRow) {
  std::vector<uint8_t> pixels{1, 2, 3,  //
                              4, 5, 6};
  PixelBuffer page(3, 2, pixels);

  EXPECT_EQ(WrittenScanlines(page), (std::vector<uint8_t>{1, 2, 3, 4, 5, 6}));
}

// The window's page has pixels only while it is locked, and its rows can be
// wider than the page: the writer locks it and steps over the padding.
TEST(WritePcxTest, WritesASurfaceSkippingItsRowPadding) {
  PaddedSurface surface(4, {1, 2, 3, 99,  //
                            4, 5, 6, 99});
  PixelBuffer page;
  page.Init(3, 2, surface);

  EXPECT_EQ(WrittenScanlines(page), (std::vector<uint8_t>{1, 2, 3, 4, 5, 6}));
}

TEST(WritePcxTest, WritesNothingForASurfaceThatWillNotLock) {
  PaddedSurface surface(4, std::vector<uint8_t>(8), false);
  PixelBuffer page;
  page.Init(3, 2, surface);
  RecordingStream out;

  Write_PCX_File(out, page, {});

  EXPECT_TRUE(out.bytes().empty());
}

}  // namespace

}  // namespace ra
