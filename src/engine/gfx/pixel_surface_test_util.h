// File: FakeSurface, a PixelSurface of plain memory for tests that draw on or
// read from the window's page without a window.

#ifndef CNC_RED_ALERT_ENGINE_GFX_PIXEL_SURFACE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_GFX_PIXEL_SURFACE_TEST_UTIL_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_surface.h"

// A surface of plain memory that counts the locks it sees and can be told to
// refuse them, standing in for the window. Its rows are `pitch` bytes apart,
// so a pitch wider than the page attached to it leaves padding that drawing
// and reading must skip.
//
// Example:
//   FakeSurface surface(4, 2);
//   PixelBuffer page;
//   page.Init(3, 2, surface);
//   page.view().Clear(7);
//   EXPECT_EQ(surface.pixels(), (std::vector<uint8_t>{7, 7, 7, 0,  //
//                                                     7, 7, 7, 0}));
class FakeSurface final : public PixelSurface {
 public:
  // A surface of `height` rows, every pixel 0.
  FakeSurface(int pitch, int height);
  // A surface that starts with `pixels`, which are whole rows of `pitch`
  // bytes.
  FakeSurface(int pitch, std::vector<uint8_t> pixels);

  std::optional<Pixels> Lock() override;
  void Unlock() override;

  // Makes every Lock() from now on fail, or succeed again.
  void set_refuse_lock(bool refuse) { refuse_lock_ = refuse; }
  // Every byte of the surface, padding included.
  [[nodiscard]] const std::vector<uint8_t>& pixels() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return pixels_;
  }
  // How often Lock() was called, refused calls included.
  [[nodiscard]] int lock_calls() const { return lock_calls_; }
  [[nodiscard]] int unlock_calls() const { return unlock_calls_; }

 private:
  std::vector<uint8_t> pixels_;
  int pitch_;
  bool refuse_lock_ = false;
  int lock_calls_ = 0;
  int unlock_calls_ = 0;
};

#endif  // CNC_RED_ALERT_ENGINE_GFX_PIXEL_SURFACE_TEST_UTIL_H_
