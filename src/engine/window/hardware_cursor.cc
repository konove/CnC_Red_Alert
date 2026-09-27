// Implements HardwareCursor: decoding the game's
// cursor shapes into SDL color cursors, scaling them for the display, and
// keeping their colors in step with the palette.

#include "engine/window/hardware_cursor.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_surface.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/gfx/shape.h"
#include "engine/window/display.h"

namespace engine::window {

bool mouse_grab_disabled = false;

void SdlCursorDeleter::operator()(SDL_Cursor* cursor) const noexcept {
  SDL_FreeCursor(cursor);
}

void SdlSurfaceDeleter::operator()(SDL_Surface* surface) const noexcept {
  SDL_FreeSurface(surface);
}

// The Shape_Type fields up to and including DataLength. The color table after
// them belongs to compact shapes, which SetShape() rejects.
constexpr base::ssize kShapeHeaderSize = offsetof(Shape_Type, Colortable);
static_assert(kShapeHeaderSize == 10);

// Decodes the pixels of a 256-color shape: `data` is what follows the header,
// LCW-compressed, and under the LCW the pixels are zero-run-length encoded. A
// nonzero byte is a pixel, and a zero byte is followed by a count of
// transparent (color 0) pixels to skip. Returns the Width * OriginalHeight
// pixels, or nullopt for data that runs out early or holds a zero or overlong
// count.
static std::optional<std::vector<uint8_t>> DecodeShapePixels(
    const Shape_Type& header, std::span<const std::byte> data) {
  std::vector<uint8_t> rle_data(header.DataLength);
  const int32_t rle_bytes =
      LCW_Uncompress(data, std::as_writable_bytes(std::span(rle_data)));
  auto input = std::span(rle_data).first(base::ToSize(rle_bytes));
  // Starts out transparent: skipped pixels stay color 0.
  std::vector<uint8_t> pixels(
      base::ToSize(int{header.Width} * int{header.OriginalHeight}));
  auto output = std::span(pixels);
  while (!output.empty()) {
    if (input.empty()) {
      return std::nullopt;
    }
    const uint8_t pixel = input.front();
    input = input.subspan(1);
    if (pixel != 0) {
      output.front() = pixel;
      output = output.subspan(1);
    } else {
      if (input.empty()) {
        return std::nullopt;
      }
      const auto transparent_run = input.front();
      input = input.subspan(1);
      if (transparent_run == 0 || transparent_run > output.size()) {
        return std::nullopt;
      }
      output = output.subspan(transparent_run);
    }
  }
  return pixels;
}

// Draws the width x height image `src` into the 8-bit `surface`, scaled up by
// the factor that makes it fill the surface. Nearest-neighbor keeps the pixel
// art crisp.
static void ScaleUpNearest(std::span<const uint8_t> src, int width,
                           const SDL_Surface& surface) {
  const int scale = surface.w / width;
  // SDL-created surfaces own pitch * h writable bytes; the pitch may include
  // padding past the w pixels of a row.
  const auto surface_pixels =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint8_t*>(surface.pixels),
                base::ToSize(surface.pitch) * base::ToSize(surface.h));
  for (int y = 0; y < surface.h; ++y) {
    const auto src_row =
        src.subspan(base::ToSize((y / scale) * width), base::ToSize(width));
    const auto dst_row = surface_pixels.subspan(base::ToSize(y * surface.pitch),
                                                base::ToSize(surface.w));
    for (int x = 0; x < surface.w; ++x) {
      base::At(dst_row, x) = base::At(src_row, x / scale);
    }
  }
}

// Returns the whole factor to scale cursors by: the scale the game's picture
// is drawn at, so that the cursor is as big next to the game's art as its
// authors drew it. SDL scales the picture up to the window, but the OS draws a
// hardware cursor itself, where an unscaled cursor would shrink. The picture
// scale is fractional only on a high-DPI display, and rounds to the nearest
// whole factor there. The cap bounds the surface on very large windows.
static int CursorScale() {
  constexpr int kMaxScale = 8;
  const float picture_scale = HasDisplay() ? TheDisplay().PictureScale() : 1;
  return std::clamp(static_cast<int>(std::lround(picture_scale)), 1, kMaxScale);
}

// Returns the display's palette, or nullptr without a display or before it has
// a video mode.
static const SDL_Palette* GamePalette() {
  if (!HasDisplay()) {
    return nullptr;
  }
  return static_cast<const SDL_Palette*>(TheDisplay().palette());
}

// Gives colors 1-255 of the cursor's palette the game's colors. Color 0 is
// left alone: it is the transparent one.
static void CopyGameColors(const SDL_Palette& game_palette,
                           SDL_Palette& cursor_palette) {
  SDL_SetPaletteColors(
      &cursor_palette,
      // SDL owns ncolors (256) entries in the window palette.
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(game_palette.colors, base::ToSize(game_palette.ncolors))
          .subspan(1, 255)
          .data(),
      1, 255);
}

HardwareCursor::ShownScope::ShownScope(HardwareCursor& cursor)
    : cursor_(cursor), saved_hide_count_(cursor.hide_count()) {
  cursor_.ForceShow();
}

// Goes back through Hide() and Show() rather than setting the count, so that
// SDL hears about the change; the second loop undoes Hide() calls the scope's
// code left unmatched.
HardwareCursor::ShownScope::~ShownScope() {
  while (cursor_.hide_count() < saved_hide_count_) {
    cursor_.Hide();
  }
  while (cursor_.hide_count() > saved_hide_count_) {
    cursor_.Show();
  }
}

HardwareCursor::HardwareCursor(int max_width, int max_height)
    : max_width_(max_width), max_height_(max_height) {}

HardwareCursor::~HardwareCursor() = default;

void HardwareCursor::SetShape(int hot_x, int hot_y,
                              std::span<const std::byte> shape) {
  if (std::ssize(shape) < kShapeHeaderSize || current_shape_ == shape.data()) {
    return;
  }
  Shape_Type header{};
  base::CopyBytes(base::ObjectBytes(header), shape, kShapeHeaderSize);

  if (header.Width == 0 || header.OriginalHeight == 0 ||
      std::cmp_greater(header.Width, max_width_) ||
      std::cmp_greater(header.OriginalHeight, max_height_)) {
    return;
  }

  // Only ShapeType 0 (LCW compressed, 256-color) is supported here.
  if (header.ShapeType != 0) {
    DLOG(INFO) << "SetShape: shape type " << header.ShapeType;
    return;
  }
  auto pixels = DecodeShapePixels(header, shape.subspan(kShapeHeaderSize));
  if (!pixels) {
    return;
  }

  current_shape_ = shape.data();
  shape_pixels_ = *std::move(pixels);
  shape_width_ = header.Width;
  shape_height_ = header.OriginalHeight;
  hot_x_ = hot_x;
  hot_y_ = hot_y;
  DrawSurface(CursorScale());
}

void HardwareCursor::UpdateScale() {
  if (shape_pixels_.empty()) {
    return;
  }
  const int scale = CursorScale();
  if (scale != scale_) {
    DrawSurface(scale);
  }
}

void HardwareCursor::DrawSurface(int scale) {
  // Scaled pixels are new ones, so the surface owns them rather than
  // borrowing through SDL_CreateRGBSurfaceFrom.
  SdlSurfacePtr surface(SDL_CreateRGBSurface(
      0, shape_width_ * scale, shape_height_ * scale, 8, 0, 0, 0, 0));
  if (!surface) {
    return;
  }
  ScaleUpNearest(shape_pixels_, shape_width_, *surface);

  if (const SDL_Palette* game_palette = GamePalette()) {
    CopyGameColors(*game_palette, *surface->format->palette);
    // An 8-bit surface's palette has 256 entries. SDL_CreateColorCursor()
    // converts the surface to ARGB through it, so an alpha of 0 on entry 0 is
    // what makes color 0 see-through.
    surface->format->palette->colors->a = 0;
    // The new cursor has the current colors already.
    palette_stale_ = false;
  }

  sdl_surface_ = std::move(surface);
  scale_ = scale;
  RebuildCursor();
}

// Only the transitions between a count of 0 and 1 reach SDL.
void HardwareCursor::Hide() {
  if (!hide_count_++) {
    SDL_ShowCursor(SDL_DISABLE);
  }
}

void HardwareCursor::Show() {
  if (!hide_count_) {
    return;
  }
  if (--hide_count_ == 0) {
    if (palette_stale_) {
      UpdatePalette();
    }
    SDL_ShowCursor(SDL_ENABLE);
  }
}

// Leaves one Hide() outstanding so that Show() does the work of making the
// cursor visible, including a palette change that waited for it.
void HardwareCursor::ForceShow() {
  hide_count_ = std::min(hide_count_, 1);
  Show();
}

void HardwareCursor::GrabMouse() {
  if (!mouse_grab_disabled) {
    TheDisplay().SetMouseGrab(true);
  }
}

void HardwareCursor::ReleaseMouse() { TheDisplay().SetMouseGrab(false); }

void HardwareCursor::UpdatePalette() {
  const SDL_Palette* game_palette = GamePalette();
  if (game_palette == nullptr || !sdl_surface_) {
    return;
  }

  if (hide_count_) {
    // Show() comes back here once the count reaches 0.
    palette_stale_ = true;
    return;
  }

  palette_stale_ = false;
  // Color 0 keeps the alpha of 0 SetShape() gave it.
  CopyGameColors(*game_palette, *sdl_surface_->format->palette);
  RebuildCursor();
}

void HardwareCursor::RebuildCursor() {
  SdlCursorPtr cursor(SDL_CreateColorCursor(sdl_surface_.get(), hot_x_ * scale_,
                                            hot_y_ * scale_));
  SDL_SetCursor(cursor.get());
  sdl_cursor_ = std::move(cursor);
}

bool IsLeftButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
}

bool IsRightButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
}

}  // namespace engine::window
