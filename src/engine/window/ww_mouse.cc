// Implements HardwareCursor and the free mouse functions: decoding the game's
// cursor shapes into SDL color cursors, scaling them for the display, and
// keeping their colors in step with the palette.

#include "engine/window/ww_mouse.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_surface.h>
#include <SDL_video.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
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

bool mouse_grab_disabled = false;

void SdlCursorDeleter::operator()(SDL_Cursor* cursor) const noexcept {
  SDL_FreeCursor(cursor);
}

void SdlSurfaceDeleter::operator()(SDL_Surface* surface) const noexcept {
  SDL_FreeSurface(surface);
}

// The instance the free functions forward to: the one constructed last, or
// nullptr while there is none.
static HardwareCursor* active_cursor = nullptr;

// The Shape_Type fields up to and including DataLength. The color table after
// them belongs to compact shapes, which SetShape() rejects.
constexpr base::ssize kShapeHeaderSize = offsetof(Shape_Type, Colortable);
static_assert(kShapeHeaderSize == 10);

// Returns the width x height image `src` scaled up `scale` (at least 1) times.
// Nearest-neighbor keeps the pixel art crisp.
[[nodiscard]] static std::vector<uint8_t> ScaleUpNearest(
    std::span<const uint8_t> src, int width, int height, int scale) {
  const int dst_width = width * scale;
  const int dst_height = height * scale;
  std::vector<uint8_t> dst(
      base::ToSize(static_cast<base::ssize>(dst_width) * dst_height));

  for (int y = 0; y < dst_height; ++y) {
    const int src_y = y / scale;
    for (int x = 0; x < dst_width; ++x) {
      const int src_x = x / scale;
      dst.at(base::ToSize((y * dst_width) + x)) =
          base::At(src, base::ToSize((src_y * width) + src_x));
    }
  }
  return dst;
}

// Returns the whole factor to scale cursors by on the display the window is on:
// one step per 500 display lines, from 1 to 4 (1080 lines give 2, 2160 give
// 4). SDL's logical size scales the game's 400-line picture up to the window,
// but the OS draws a hardware cursor at the display's own resolution, where an
// unscaled cursor would shrink.
static int CursorScaleForDisplay() {
  const int display_index = engine::window::HasDisplay()
                                ? engine::window::TheDisplay().DisplayIndex()
                                : 0;

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(display_index, &mode) != 0) {
    // No video subsystem, or no such display: draw the cursor unscaled.
    return 1;
  }

  constexpr int kLinesPerScaleStep = 500;
  constexpr int kMaxScale = 4;
  return std::clamp(mode.h / kLinesPerScaleStep, 1, kMaxScale);
}

// Returns the display's palette, or nullptr without a display or before it has
// a video mode.
static const SDL_Palette* GamePalette() {
  if (!engine::window::HasDisplay()) {
    return nullptr;
  }
  return static_cast<const SDL_Palette*>(
      engine::window::TheDisplay().palette());
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

HardwareCursor::HardwareCursor(int max_width, int max_height)
    : max_width_(max_width), max_height_(max_height) {
  GrabMouse();
  active_cursor = this;
}

HardwareCursor::~HardwareCursor() {
  ReleaseMouse();
  // Input::InstallMouse() builds a replacement before the old mouse goes, and
  // then active_cursor is already the new one.
  if (active_cursor == this) {
    active_cursor = nullptr;
  }
}

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
  const int width = header.Width;
  const int height = header.OriginalHeight;

  std::vector<uint8_t> rle_data(header.DataLength);
  const int32_t rle_bytes =
      LCW_Uncompress(shape.subspan(kShapeHeaderSize),
                     std::as_writable_bytes(std::span(rle_data)));
  auto input = std::span(rle_data).first(base::ToSize(rle_bytes));
  // Starts out transparent: skipped pixels stay color 0.
  std::vector<uint8_t> pixels(base::ToSize(width * height));
  auto output = std::span(pixels);
  // Undo the zero-run-length encoding under the LCW: a nonzero byte is a
  // pixel, and a zero byte is followed by a count of transparent (color 0)
  // pixels to skip. Data that runs out early or holds a zero or overlong count
  // is corrupt, and the old cursor stays.
  while (!output.empty()) {
    if (input.empty()) {
      return;
    }
    const uint8_t pixel = input.front();
    input = input.subspan(1);
    if (pixel != 0) {
      output.front() = pixel;
      output = output.subspan(1);
    } else {
      if (input.empty()) {
        return;
      }
      const auto transparent_run = input.front();
      input = input.subspan(1);
      if (transparent_run == 0 || transparent_run > output.size()) {
        return;
      }
      output = output.subspan(transparent_run);
    }
  }

  const int scale = CursorScaleForDisplay();
  const int scaled_width = width * scale;
  const int scaled_height = height * scale;
  const auto scaled_pixels = ScaleUpNearest(pixels, width, height, scale);

  // The surface outlives scaled_pixels, because UpdatePalette() recolors it
  // later, so it has to own its pixels: SDL_CreateRGBSurfaceFrom would only
  // borrow them.
  SdlSurfacePtr surface(
      SDL_CreateRGBSurface(0, scaled_width, scaled_height, 8, 0, 0, 0, 0));
  if (!surface) {
    return;
  }
  // SDL surface pitch may include padding, so copy row by row.
  // SDL-created surface owns pitch*h writable bytes.
  const auto surface_pixels =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint8_t*>(surface->pixels),
                base::ToSize(surface->pitch) * base::ToSize(surface->h));
  for (int y = 0; y < scaled_height; ++y) {
    std::ranges::copy(
        std::span(scaled_pixels)
            .subspan(base::ToSize(y * scaled_width),
                     base::ToSize(scaled_width)),
        surface_pixels.subspan(base::ToSize(y * surface->pitch)).begin());
  }

  if (const SDL_Palette* game_palette = GamePalette()) {
    CopyGameColors(*game_palette, *surface->format->palette);
    // An 8-bit surface's palette has 256 entries. SDL_CreateColorCursor()
    // converts the surface to ARGB through it, so an alpha of 0 on entry 0 is
    // what makes color 0 see-through.
    surface->format->palette->colors->a = 0;
  }

  current_shape_ = shape.data();
  sdl_surface_ = std::move(surface);
  hot_x_ = hot_x;
  hot_y_ = hot_y;
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

int HardwareCursor::hide_count() const { return hide_count_; }

int HardwareCursor::x() const { return x_; }

int HardwareCursor::y() const { return y_; }

void HardwareCursor::GrabMouse() {
  if (!mouse_grab_disabled) {
    engine::window::TheDisplay().SetMouseGrab(true);
  }
}

void HardwareCursor::ReleaseMouse() {
  engine::window::TheDisplay().SetMouseGrab(false);
}

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

void HardwareCursor::RecordPosition(int x, int y) {
  x_ = x;
  y_ = y;
}

// The free functions the games call, forwarding to active_cursor.

void HideMouse() {
  if (active_cursor) {
    active_cursor->Hide();
  }
}

void ShowMouse() {
  if (active_cursor) {
    active_cursor->Show();
  }
}

int MouseHideCount() {
  if (active_cursor) {
    return active_cursor->hide_count();
  }
  return 0;
}
void SetMouseShape(int hot_x, int hot_y, std::span<const std::byte> shape) {
  if (active_cursor) {
    active_cursor->SetShape(hot_x, hot_y, shape);
  }
}

bool IsLeftButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
}

bool IsRightButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
}

int MouseX() {
  if (active_cursor) {
    return active_cursor->x();
  }
  return 0;
}

int MouseY() {
  if (active_cursor) {
    return active_cursor->y();
  }
  return 0;
}

void UpdateMousePalette() {
  if (active_cursor) {
    active_cursor->UpdatePalette();
  }
}

void RecordMousePosition(int x, int y) {
  if (active_cursor) {
    active_cursor->RecordPosition(x, y);
  }
}
