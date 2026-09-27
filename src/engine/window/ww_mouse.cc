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
#include <span>
#include <utility>
#include <vector>

#include "absl/log/log.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/gfx/pixel_buffer.h"
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

// Returns the src_width x src_height image `src` scaled up `scale` times, and
// its size in *dst_width and *dst_height; empty, with a size of 0 x 0, for an
// empty image or a scale below 1. Nearest-neighbor keeps the pixel art crisp.
[[nodiscard]] static std::vector<uint8_t> ScaleUpNearest(
    std::span<const uint8_t> src, int src_width, int src_height, int scale,
    int* dst_width, int* dst_height) {
  if (src_width <= 0 || src_height <= 0 || scale <= 0) {
    *dst_width = 0;
    *dst_height = 0;
    return {};
  }

  const int width = src_width * scale;
  const int height = src_height * scale;
  std::vector<uint8_t> dst(
      base::ToSize(static_cast<base::ssize>(width) * height));

  for (int y = 0; y < height; ++y) {
    const int src_y = y / scale;
    for (int x = 0; x < width; ++x) {
      const int src_x = x / scale;
      dst.at(base::ToSize((y * width) + x)) =
          base::At(src, base::ToSize((src_y * src_width) + src_x));
    }
  }

  *dst_width = width;
  *dst_height = height;
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
  const int scale = mode.h / kLinesPerScaleStep;
  if (scale < 1) {
    return 1;
  }
  if (scale > kMaxScale) {
    return kMaxScale;
  }
  return scale;
}

HardwareCursor::HardwareCursor([[maybe_unused]] PixelView* scr, int max_width,
                               int max_height)
    : MouseCursor(
          base::ToSize(static_cast<base::ssize>(max_width) * max_height)),
      max_width_(max_width),
      max_height_(max_height) {
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
  // 10 is the size of the Shape_Type header up to and including DataLength;
  // the color table after it belongs to compact shapes, which are rejected
  // below.
  if (shape.size() < 10 || current_shape_ == shape.data()) {
    return;
  }
  Shape_Type header{};
  base::CopyBytes(base::ObjectBytes(header), shape, 10);
  const auto* cursor_shape = &header;

  if (cursor_shape->Width == 0 || cursor_shape->OriginalHeight == 0 ||
      std::cmp_greater(cursor_shape->Width, max_width_) ||
      std::cmp_greater(cursor_shape->OriginalHeight, max_height_)) {
    return;
  }

  // Only ShapeType 0 (LCW compressed, 256-color) is supported here.
  if (cursor_shape->ShapeType != 0) {
    DLOG(INFO) << "SetShape: shape type " << cursor_shape->ShapeType;
    return;
  }

  // Shape data is LCW compressed starting at byte 10 (after the header).
  std::vector<uint8_t> rle_data(cursor_shape->DataLength);
  const int32_t rle_bytes = LCW_Uncompress(
      shape.subspan(10), std::as_writable_bytes(std::span(rle_data)));
  auto input = std::span(rle_data).first(base::ToSize(rle_bytes));
  const auto pixel_count =
      base::ToSize(cursor_shape->Width * cursor_shape->OriginalHeight);
  auto output = std::span(MouseCursor).first(pixel_count);
  std::ranges::fill(output, 0);
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

  // Scale a copy of the unscaled shape for the display. Palette changes do not
  // come back to it: UpdatePalette() recolors the scaled surface instead.
  OriginalWidth = cursor_shape->Width;
  OriginalHeight = cursor_shape->OriginalHeight;
  const int original_size = OriginalWidth * OriginalHeight;

  OriginalCursor.assign(MouseCursor.begin(),
                        MouseCursor.begin() + original_size);

  scale_ = CursorScaleForDisplay();
  int scaled_width = 0;
  int scaled_height = 0;
  const auto scaled_cursor =
      ScaleUpNearest(OriginalCursor, OriginalWidth, OriginalHeight, scale_,
                     &scaled_width, &scaled_height);
  if (scaled_cursor.empty()) {
    return;
  }

  const int scaled_hot_x = hot_x * scale_;
  const int scaled_hot_y = hot_y * scale_;

  // The surface outlives scaled_cursor, because UpdatePalette() recolors it
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
        std::span(scaled_cursor)
            .subspan(base::ToSize(y * scaled_width),
                     base::ToSize(scaled_width)),
        surface_pixels.subspan(base::ToSize(y * surface->pitch)).begin());
  }

  const auto* window_palette = engine::window::HasDisplay()
                                   ? static_cast<const SDL_Palette*>(
                                         engine::window::TheDisplay().palette())
                                   : nullptr;
  if (window_palette != nullptr) {
    // Give colors 1-255 the game's colors; color 0 is transparent. SDL owns
    // ncolors (256) entries in the window palette.
    SDL_SetPaletteColors(
        surface->format->palette,
        // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
        std::span(window_palette->colors, base::ToSize(window_palette->ncolors))
            .subspan(1)
            .data(),
        1, 255);
    // An 8-bit surface's palette has 256 entries. SDL_CreateColorCursor()
    // converts the surface to ARGB through it, so an alpha of 0 on entry 0 is
    // what makes color 0 see-through.
    surface->format->palette->colors->a = 0;
  }

  SdlCursorPtr sdl_cursor(
      SDL_CreateColorCursor(surface.get(), scaled_hot_x, scaled_hot_y));

  SDL_SetCursor(sdl_cursor.get());
  current_shape_ = shape.data();
  sdl_cursor_ = std::move(sdl_cursor);
  sdl_surface_ = std::move(surface);
  hot_x_ = hot_x;
  hot_y_ = hot_y;
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

void HardwareCursor::Conditional_Hide_Mouse(int /*x1*/, int /*y1*/, int /*x2*/,
                                            int /*y2*/) {}

void HardwareCursor::Conditional_Show_Mouse() {}

int HardwareCursor::hide_count() const { return hide_count_; }

int HardwareCursor::x() const { return x_; }

int HardwareCursor::y() const { return y_; }

void HardwareCursor::Draw_Mouse(PixelView* /*scr*/) {
  // No-op: SDL hardware cursor is drawn by the OS, not by us.
}

void HardwareCursor::Erase_Mouse(PixelView* /*scr*/, bool /*forced*/) {}

void HardwareCursor::GrabMouse() {
  if (!mouse_grab_disabled) {
    engine::window::TheDisplay().SetMouseGrab(true);
  }
}

void HardwareCursor::ReleaseMouse() {
  engine::window::TheDisplay().SetMouseGrab(false);
}

void HardwareCursor::UpdatePalette() {
  if (!engine::window::HasDisplay() ||
      engine::window::TheDisplay().palette() == nullptr || !sdl_surface_) {
    return;
  }

  if (hide_count_) {
    // Show() comes back here once the count reaches 0.
    palette_stale_ = true;
    return;
  }

  palette_stale_ = false;

  const auto* window_palette =
      static_cast<const SDL_Palette*>(engine::window::TheDisplay().palette());
  // Color 0 keeps the alpha of 0 SetShape() gave it. SDL owns ncolors (256)
  // entries in the window palette.
  SDL_SetPaletteColors(
      sdl_surface_->format->palette,
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(window_palette->colors, base::ToSize(window_palette->ncolors))
          .subspan(1)
          .data(),
      1, 255);

  const int scaled_hot_x = hot_x_ * scale_;
  const int scaled_hot_y = hot_y_ * scale_;
  SdlCursorPtr sdl_cursor(
      SDL_CreateColorCursor(sdl_surface_.get(), scaled_hot_x, scaled_hot_y));
  SDL_SetCursor(sdl_cursor.get());

  sdl_cursor_ = std::move(sdl_cursor);
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

void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2) {
  if (active_cursor) {
    active_cursor->Conditional_Hide_Mouse(x1, y1, x2, y2);
  }
}

void Conditional_Show_Mouse() {
  if (active_cursor) {
    active_cursor->Conditional_Show_Mouse();
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
