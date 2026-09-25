// Tests for PixelView and PixelBuffer: locking, both of memory and of a
// PixelSurface, and the clipping the drawing primitives do.

#include "engine/gfx/pixel_buffer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/base/unaligned.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_surface.h"
#include "gtest/gtest.h"

namespace {

// A viewport that Attach() has not been called on yet. Screen builds its
// views this way and attaches them once the video mode is known, so anything
// that draws in between meets one.
TEST(GraphicViewPortLockTest, AnUnattachedViewportDoesNotLock) {
  PixelView view;

  EXPECT_FALSE(view.Lock());
  EXPECT_EQ(view.lock_count(), 0);
}

// The drawing members all call the primitive only when Lock() succeeded, so
// an unattached viewport has to be a silent no-op rather than a crash.
TEST(GraphicViewPortLockTest, DrawingToAnUnattachedViewportDoesNothing) {
  PixelView view;

  view.Clear();
  view.PutPixel(0, 0, 1);
  view.DrawLine(0, 0, 1, 1, 1);
  view.FillRect(0, 0, 1, 1, 1);

  EXPECT_EQ(view.GetPixel(0, 0), 0);
}

// Screen constructs its views against pages that Init() has not sized yet,
// so Attach() has to cope with a buffer that has no pixels. It used to clamp
// the viewport's corner to width() - 1, that is to -1, and pixels() then
// subspanned an empty span by a negative offset.
TEST(GraphicViewPortLockTest, AttachingToAnEmptyBufferGivesAnEmptyViewport) {
  PixelBuffer page;
  PixelView view(&page, 0, 0, 640, 480);

  EXPECT_EQ(view.width(), 0);
  EXPECT_EQ(view.height(), 0);
  EXPECT_EQ(view.x_pos(), 0);
  EXPECT_EQ(view.y_pos(), 0);
  EXPECT_TRUE(view.pixels().empty());

  view.Clear();
  view.PutPixel(0, 0, 1);
}

// A plain memory buffer has no surface to lock, so Lock() succeeds without
// SDL and nothing is counted.
TEST(GraphicViewPortLockTest, MemoryBufferLocksWithoutCounting) {
  std::vector<uint8_t> pixels(size_t{4} * 4, 0);
  PixelBuffer page(4, 4, pixels);
  PixelView view(&page, 1, 1, 2, 2);

  ASSERT_TRUE(view.Lock());
  ASSERT_TRUE(view.Lock());
  EXPECT_EQ(view.lock_count(), 0);
  view.Unlock();
  view.Unlock();
  EXPECT_EQ(view.lock_count(), 0);
}

// A surface of plain memory that counts the locks it sees and can be told to
// refuse them, standing in for the window.
class FakeSurface final : public PixelSurface {
 public:
  // `pitch` bytes to a row, so a pitch wider than the page leaves padding
  // that drawing must skip.
  FakeSurface(int pitch, int height)
      : pixels_(static_cast<size_t>(pitch) * static_cast<size_t>(height)),
        pitch_(pitch) {}

  std::optional<Pixels> Lock() override {
    ++lock_calls_;
    if (refuse_lock_) {
      return std::nullopt;
    }
    return Pixels{.bytes = pixels_, .pitch = pitch_};
  }
  void Unlock() override { ++unlock_calls_; }

  void set_refuse_lock(bool refuse) { refuse_lock_ = refuse; }
  [[nodiscard]] const std::vector<uint8_t>& pixels() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return pixels_;
  }
  [[nodiscard]] int lock_calls() const { return lock_calls_; }
  [[nodiscard]] int unlock_calls() const { return unlock_calls_; }

 private:
  std::vector<uint8_t> pixels_;
  int pitch_;
  bool refuse_lock_ = false;
  int lock_calls_ = 0;
  int unlock_calls_ = 0;
};

// The window page has no pixels of its own: they exist only while the
// surface is locked.
TEST(SurfaceBufferTest, HasNoPixelsUntilLocked) {
  FakeSurface surface(4, 2);
  PixelBuffer page;
  page.Init(4, 2, surface);

  EXPECT_TRUE(page.HasSurface());
  EXPECT_TRUE(page.view().NeedsLock());
  EXPECT_TRUE(page.bytes().empty());
  EXPECT_EQ(surface.lock_calls(), 0);
}

// Locks nest, and only the outermost pair reaches the surface, which
// PixelSurface promises its implementations.
TEST(SurfaceBufferTest, OnlyTheOutermostLockReachesTheSurface) {
  FakeSurface surface(4, 2);
  PixelBuffer page;
  page.Init(4, 2, surface);

  ASSERT_TRUE(page.LockSurface());
  ASSERT_TRUE(page.LockSurface());
  EXPECT_EQ(page.lock_count(), 2);
  EXPECT_EQ(surface.lock_calls(), 1);
  EXPECT_EQ(page.bytes().size(), 8U);

  page.UnlockSurface();
  EXPECT_EQ(surface.unlock_calls(), 0);
  page.UnlockSurface();
  EXPECT_EQ(surface.unlock_calls(), 1);
  EXPECT_EQ(page.lock_count(), 0);
  EXPECT_TRUE(page.bytes().empty());
}

// The surface's rows can be wider than the page; drawing has to step over
// the padding rather than run on into the next row.
TEST(SurfaceBufferTest, DrawsIntoTheSurfaceSkippingItsRowPadding) {
  FakeSurface surface(4, 2);
  PixelBuffer page;
  page.Init(3, 2, surface);

  page.view().Clear(7);

  EXPECT_EQ(surface.pixels(), (std::vector<uint8_t>{7, 7, 7, 0,  //
                                                    7, 7, 7, 0}));
  EXPECT_EQ(surface.lock_calls(), 1);
  EXPECT_EQ(surface.unlock_calls(), 1);
}

// A view onto part of the page reaches the surface's pixels at its own
// corner once locked.
TEST(SurfaceBufferTest, AViewLocksThePageAndFindsItsCorner) {
  FakeSurface surface(4, 3);
  PixelBuffer page;
  page.Init(3, 3, surface);
  PixelView view(&page, 1, 1, 2, 2);

  view.PutPixel(0, 0, 5);

  EXPECT_EQ(surface.pixels(), (std::vector<uint8_t>{0, 0, 0, 0,  //
                                                    0, 5, 0, 0,  //
                                                    0, 0, 0, 0}));
}

// A surface that refuses the lock leaves the page unlocked, and drawing
// becomes a no-op that never unlocks what it could not lock.
TEST(SurfaceBufferTest, ARefusedLockDrawsNothing) {
  FakeSurface surface(2, 2);
  surface.set_refuse_lock(true);
  PixelBuffer page;
  page.Init(2, 2, surface);

  EXPECT_FALSE(page.LockSurface());
  EXPECT_EQ(page.lock_count(), 0);
  page.view().Clear(7);

  EXPECT_EQ(surface.unlock_calls(), 0);
  EXPECT_EQ(surface.pixels(), (std::vector<uint8_t>{0, 0, 0, 0}));
}

// Giving the page memory again lets go of the surface.
TEST(SurfaceBufferTest, InitWithMemoryDetachesTheSurface) {
  FakeSurface surface(2, 2);
  PixelBuffer page;
  page.Init(2, 2, surface);
  std::vector<uint8_t> pixels(size_t{2} * 2, 0);

  page.Init(2, 2, pixels, 0);
  page.view().Clear(3);

  EXPECT_FALSE(page.HasSurface());
  EXPECT_EQ(surface.lock_calls(), 0);
  EXPECT_EQ(pixels, (std::vector<uint8_t>{3, 3, 3, 3}));
}

// Returns `count` pixels numbered from 1, so that every pixel of a test image
// is distinct and none is the 0 of a cleared destination.
std::vector<uint8_t> NumberedPixels(int count) {
  std::vector<uint8_t> pixels(static_cast<size_t>(count));
  std::ranges::iota(pixels, uint8_t{1});
  return pixels;
}

TEST(CopyFromBufferTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2
  // 3 4
  // 5 6
  const std::vector<uint8_t> image = NumberedPixels(2 * 3);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  dest.view().CopyFromBuffer(-1, -1, 2, 3, image);

  EXPECT_EQ(page, (std::vector<uint8_t>{4, 0, 0, 0,  //
                                        6, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(CopyToBufferTest, ClipsRowsAndColumnsOffTheTopLeft) {
  //  1  2  3  4
  //  5  6  7  8
  //  ...
  std::vector<uint8_t> page = NumberedPixels(4 * 4);
  PixelBuffer dest(4, 4, page);
  std::array<uint8_t, 9> out{};
  out.fill(0xff);

  // The part of the 3x3 rectangle that lies off the view is left alone.
  dest.view().CopyToBuffer(-1, -1, 3, 3, out);

  EXPECT_EQ(out, (std::array<uint8_t, 9>{0xff, 0xff, 0xff,  //
                                         0xff, 1, 2,        //
                                         0xff, 5, 6}));
}

TEST(BlitTest, ClipsRowsAndColumnsOffTheTopLeft) {
  // 1 2 3
  // 4 5 6
  // 7 8 9
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, -1, -1, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{5, 6, 0, 0,  //
                                        8, 9, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 0, 0}));
}

TEST(BlitTest, SourceClippedAtTopLeftKeepsItsPlaceInTheDestination) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  // The rectangle's first row and column lie off the source, so the pixels
  // that do exist belong one row down and one column right of (1, 1).
  source.view().BlitTo(dest.view(), -1, -1, 1, 1, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

TEST(BlitTest, ClipsRowsAndColumnsOffTheBottomRight) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, 2, 2, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        0, 0, 1, 2,  //
                                        0, 0, 4, 5}));
}

TEST(BlitTest, RectangleWhollyOffTheDestinationDrawsNothing) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  const std::vector<uint8_t> blank(size_t{4} * 4, 0);
  std::vector<uint8_t> page = blank;
  PixelBuffer dest(4, 4, page);

  source.view().BlitTo(dest.view(), 0, 0, -3, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 4, 0, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 0, -3, 3, 3);
  EXPECT_EQ(page, blank);
  source.view().BlitTo(dest.view(), 0, 0, 0, 4, 3, 3);
  EXPECT_EQ(page, blank);
}

// The shape the chronal vortex blits: a source buffer exactly the size of the
// rectangle, landing partly off two edges of the destination at once.
TEST(BlitTest, FullSourceOverhangingTwoEdgesCarriesTheSourceAlong) {
  std::vector<uint8_t> image = NumberedPixels(3 * 3);
  PixelBuffer source(3, 3, image);
  std::vector<uint8_t> page(size_t{4} * 4, 0);
  PixelBuffer dest(4, 4, page);

  // Off the left by one and off the bottom by one.
  source.view().BlitTo(dest.view(), 0, 0, -1, 2, 3, 3);

  EXPECT_EQ(page, (std::vector<uint8_t>{0, 0, 0, 0,  //
                                        0, 0, 0, 0,  //
                                        2, 3, 0, 0,  //
                                        5, 6, 0, 0}));
}

// A three-glyph font, 2 rows tall: glyph 1 is 3 pixels wide and 2 rows,
// glyph 2 is 2 pixels wide and 1 row with 1 blank row above.
std::vector<std::byte> MakePrintFont() {
  std::vector<std::byte> blob(42, std::byte{0});
  const auto word = [&blob](int offset, int value) {
    base::WriteUnaligned(std::span(blob).subspan(static_cast<size_t>(offset)),
                         static_cast<uint16_t>(value));
  };
  const auto byte = [&blob](int offset, int value) {
    blob.at(static_cast<size_t>(offset)) = static_cast<std::byte>(value);
  };
  word(4, 14);   // info block
  word(6, 20);   // offset table
  word(8, 26);   // width table
  word(10, 35);  // glyph data
  word(12, 29);  // height table
  byte(14 + kFontInfoMaxHeight, 2);
  byte(14 + kFontInfoMaxWidth, 3);
  word(22, 35);             // glyph 1 data
  word(24, 39);             // glyph 2 data
  byte(27, 3);              // glyph 1 width
  byte(28, 2);              // glyph 2 width
  word(31, 2 * 256);        // glyph 1: 2 rows
  word(33, (1 * 256) + 1);  // glyph 2: 1 row, 1 blank above
  // Glyph 1 rows: pixel values 1 2 3 and 0 1 2, two per byte, low nibble
  // first. Glyph 2: 1 3.
  byte(35, 0x21);
  byte(36, 0x03);
  byte(37, 0x10);
  byte(38, 0x02);
  byte(39, 0x31);
  return blob;
}

TEST(PrintTest, DrawsThroughTheStylesPaletteAndSpacing) {
  const std::vector<std::byte> blob = MakePrintFont();
  FontStyle style{.font = FontView(blob), .x_spacing = 1};
  style.palette.at(2) = 50;
  style.palette.at(3) = 60;
  std::vector<uint8_t> pixels(size_t{8} * 2, 0);
  PixelBuffer page(8, 2, pixels);

  page.view().Print(style, "\x01\x02", 0, 0, 7, 0);

  // Glyph 1 at x 0-2, one pixel of spacing, glyph 2 at x 4-5 on row 1.
  EXPECT_EQ(pixels, (std::vector<uint8_t>{7, 50, 60, 0, 0, 0, 0, 0,  //
                                          0, 7, 50, 0, 7, 60, 0, 0}));
}

}  // namespace
