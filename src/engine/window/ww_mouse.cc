// Implements WWMouseClass and the free mouse functions: decoding the game's
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

bool NoMouseGrab = false;

void SDLCursorDeleter::operator()(SDL_Cursor* p) const noexcept {
  SDL_FreeCursor(p);
}

void SDLSurfaceDeleter::operator()(SDL_Surface* p) const noexcept {
  SDL_FreeSurface(p);
}

// The instance the free functions forward to: the one constructed last, or
// nullptr before the first.
// TODO: ~WWMouseClass() does not reset this, so after Input::RemoveMouse() the
// free functions (and Display::SetPalette() through Update_Mouse_Palette())
// would call into the freed object.
static WWMouseClass* Mouse = nullptr;

// Returns the src_w x src_h image `src` scaled up `scale` times, and its size
// in *out_w and *out_h; empty, with a size of 0 x 0, for an empty image or a
// scale below 1. Nearest-neighbor keeps the pixel art crisp.
[[nodiscard]] static std::vector<uint8_t> Scale_Cursor_Nearest(
    std::span<const uint8_t> src, int src_w, int src_h, int scale, int* out_w,
    int* out_h) {
  if (src_w <= 0 || src_h <= 0 || scale <= 0) {
    *out_w = 0;
    *out_h = 0;
    return {};
  }

  const int dst_w = src_w * scale;
  const int dst_h = src_h * scale;
  std::vector<uint8_t> dst(
      base::ToSize(static_cast<base::ssize>(dst_w) * dst_h));

  for (int y = 0; y < dst_h; ++y) {
    const int src_y = y / scale;
    for (int x = 0; x < dst_w; ++x) {
      const int src_x = x / scale;
      dst.at(base::ToSize((y * dst_w) + x)) =
          base::At(src, base::ToSize((src_y * src_w) + src_x));
    }
  }

  *out_w = dst_w;
  *out_h = dst_h;
  return dst;
}

// Returns the whole factor to scale cursors by on the display the window is on:
// one step per 500 display lines, from 1 to 4 (1080 lines give 2, 2160 give
// 4). SDL's logical size scales the game's 400-line picture up to the window,
// but the OS draws a hardware cursor at the display's own resolution, where an
// unscaled cursor would shrink.
static int Get_Display_Scale() {
  const int display_index = engine::window::HasDisplay()
                                ? engine::window::TheDisplay().DisplayIndex()
                                : 0;

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(display_index, &mode) != 0) {
    // No video subsystem, or no such display: draw the cursor unscaled.
    return 1;
  }

  constexpr int kLogicalHeight = 500;
  constexpr int kMaxScale = 4;
  const int scale = mode.h / kLogicalHeight;
  if (scale < 1) {
    return 1;
  }
  if (scale > kMaxScale) {
    return kMaxScale;
  }
  return scale;
}

WWMouseClass::WWMouseClass([[maybe_unused]] PixelView* scr, int max_width,
                           int max_height)
    : MouseCursor(
          base::ToSize(static_cast<base::ssize>(max_width) * max_height)),
      MaxWidth(max_width),
      MaxHeight(max_height) {
  Set_Cursor_Clip();
  Mouse = this;
}

WWMouseClass::~WWMouseClass() { Clear_Cursor_Clip(); }

void WWMouseClass::Set_Cursor(int xhotspot, int yhotspot,
                              std::span<const std::byte> cursor) {
  // 10 is the size of the Shape_Type header up to and including DataLength;
  // the color table after it belongs to compact shapes, which are rejected
  // below.
  if (cursor.size() < 10 || PrevCursor == cursor.data()) {
    return;
  }
  Shape_Type shape{};
  base::CopyBytes(base::ObjectBytes(shape), cursor, 10);
  const auto* cursor_shape = &shape;

  if (cursor_shape->Width == 0 || cursor_shape->OriginalHeight == 0 ||
      std::cmp_greater(cursor_shape->Width, MaxWidth) ||
      std::cmp_greater(cursor_shape->OriginalHeight, MaxHeight)) {
    return;
  }

  // Only ShapeType 0 (LCW compressed, 256-color) is supported here.
  if (cursor_shape->ShapeType != 0) {
    DLOG(INFO) << "Set_Cursor type " << cursor_shape->ShapeType;
    return;
  }

  // Shape data is LCW compressed starting at byte 10 (after the header).
  std::vector<uint8_t> decompressed_data(cursor_shape->DataLength);
  const int32_t decoded = LCW_Uncompress(
      cursor.subspan(10), std::as_writable_bytes(std::span(decompressed_data)));
  auto input = std::span(decompressed_data).first(base::ToSize(decoded));
  const auto pixels =
      base::ToSize(cursor_shape->Width * cursor_shape->OriginalHeight);
  auto output = std::span(MouseCursor).first(pixels);
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
      const auto count = input.front();
      input = input.subspan(1);
      if (count == 0 || count > output.size()) {
        return;
      }
      output = output.subspan(count);
    }
  }

  // Scale a copy of the unscaled shape for the display. Palette changes do not
  // come back to it: Update_Palette() recolors the scaled surface instead.
  OriginalWidth = cursor_shape->Width;
  OriginalHeight = cursor_shape->OriginalHeight;
  const int original_size = OriginalWidth * OriginalHeight;

  OriginalCursor.assign(MouseCursor.begin(),
                        MouseCursor.begin() + original_size);

  CurrentScale = Get_Display_Scale();
  int scaled_width = 0;
  int scaled_height = 0;
  const auto scaled_cursor =
      Scale_Cursor_Nearest(OriginalCursor, OriginalWidth, OriginalHeight,
                           CurrentScale, &scaled_width, &scaled_height);
  if (scaled_cursor.empty()) {
    return;
  }

  const int scaled_hotx = xhotspot * CurrentScale;
  const int scaled_hoty = yhotspot * CurrentScale;

  // The surface outlives scaled_cursor, because Update_Palette() recolors it
  // later, so it has to own its pixels: SDL_CreateRGBSurfaceFrom would only
  // borrow them.
  SDLSurfacePtr sdl_surf(
      SDL_CreateRGBSurface(0, scaled_width, scaled_height, 8, 0, 0, 0, 0));
  if (!sdl_surf) {
    return;
  }
  // SDL surface pitch may include padding, so copy row by row.
  // SDL-created surface owns pitch*h writable bytes.
  const auto surface =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint8_t*>(sdl_surf->pixels),
                base::ToSize(sdl_surf->pitch) * base::ToSize(sdl_surf->h));
  for (int y = 0; y < scaled_height; ++y) {
    std::ranges::copy(
        std::span(scaled_cursor)
            .subspan(base::ToSize(y * scaled_width),
                     base::ToSize(scaled_width)),
        surface.subspan(base::ToSize(y * sdl_surf->pitch)).begin());
  }

  const auto* window_pal = engine::window::HasDisplay()
                               ? static_cast<const SDL_Palette*>(
                                     engine::window::TheDisplay().palette())
                               : nullptr;
  if (window_pal != nullptr) {
    // Give colors 1-255 the game's colors; color 0 is transparent. SDL owns
    // ncolors (256) entries in the window palette.
    SDL_SetPaletteColors(
        sdl_surf->format->palette,
        // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
        std::span(window_pal->colors, base::ToSize(window_pal->ncolors))
            .subspan(1)
            .data(),
        1, 255);
    // An 8-bit surface's palette has 256 entries. SDL_CreateColorCursor()
    // converts the surface to ARGB through it, so an alpha of 0 on entry 0 is
    // what makes color 0 see-through.
    sdl_surf->format->palette->colors->a = 0;
  }

  SDLCursorPtr sdl_cursor(
      SDL_CreateColorCursor(sdl_surf.get(), scaled_hotx, scaled_hoty));

  SDL_SetCursor(sdl_cursor.get());
  PrevCursor = cursor.data();
  sdl_cursor_ = std::move(sdl_cursor);
  sdl_surface_ = std::move(sdl_surf);
  MouseXHot = xhotspot;
  MouseYHot = yhotspot;
}

// Only the transitions between a count of 0 and 1 reach SDL.
void WWMouseClass::Hide_Mouse() {
  if (!State++) {
    SDL_ShowCursor(SDL_DISABLE);
  }
}

void WWMouseClass::Show_Mouse() {
  if (!State) {
    return;
  }
  if (--State == 0) {
    if (PaletteDirty) {
      Update_Palette();
    }
    SDL_ShowCursor(SDL_ENABLE);
  }
}

void WWMouseClass::Conditional_Hide_Mouse(int /*x1*/, int /*y1*/, int /*x2*/,
                                          int /*y2*/) {}

void WWMouseClass::Conditional_Show_Mouse() {}

int WWMouseClass::Get_Mouse_State() const { return State; }

int WWMouseClass::Get_Mouse_X() const { return LastX; }

int WWMouseClass::Get_Mouse_Y() const { return LastY; }

void WWMouseClass::Draw_Mouse(PixelView* /*scr*/) {
  // No-op: SDL hardware cursor is drawn by the OS, not by us.
}

void WWMouseClass::Erase_Mouse(PixelView* /*scr*/, bool /*forced*/) {}

void WWMouseClass::Set_Cursor_Clip() {
  if (!NoMouseGrab) {
    engine::window::TheDisplay().SetMouseGrab(true);
  }
}

void WWMouseClass::Clear_Cursor_Clip() {
  engine::window::TheDisplay().SetMouseGrab(false);
}

void WWMouseClass::Update_Palette() {
  if (!engine::window::HasDisplay() ||
      engine::window::TheDisplay().palette() == nullptr || !sdl_surface_) {
    return;
  }

  if (State) {
    // Show_Mouse() comes back here once the count reaches 0.
    PaletteDirty = true;
    return;
  }

  PaletteDirty = false;

  const auto* window_pal =
      static_cast<const SDL_Palette*>(engine::window::TheDisplay().palette());
  // Color 0 keeps the alpha of 0 Set_Cursor() gave it. SDL owns ncolors (256)
  // entries in the window palette.
  SDL_SetPaletteColors(
      sdl_surface_->format->palette,
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(window_pal->colors, base::ToSize(window_pal->ncolors))
          .subspan(1)
          .data(),
      1, 255);

  const int scaled_hotx = MouseXHot * CurrentScale;
  const int scaled_hoty = MouseYHot * CurrentScale;
  SDLCursorPtr sdl_cursor(
      SDL_CreateColorCursor(sdl_surface_.get(), scaled_hotx, scaled_hoty));
  SDL_SetCursor(sdl_cursor.get());

  sdl_cursor_ = std::move(sdl_cursor);
}

void WWMouseClass::Update_Pos(int x, int y) {
  LastX = x;
  LastY = y;
}

// The free functions the games call, forwarding to Mouse.

void Hide_Mouse() {
  if (Mouse) {
    Mouse->Hide_Mouse();
  }
}

void Show_Mouse() {
  if (Mouse) {
    Mouse->Show_Mouse();
  }
}

void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2) {
  if (Mouse) {
    Mouse->Conditional_Hide_Mouse(x1, y1, x2, y2);
  }
}

void Conditional_Show_Mouse() {
  if (Mouse) {
    Mouse->Conditional_Show_Mouse();
  }
}

int Get_Mouse_State() {
  if (Mouse) {
    return Mouse->Get_Mouse_State();
  }
  return 0;
}
void Set_Mouse_Cursor(int hotx, int hoty, std::span<const std::byte> cursor) {
  if (Mouse) {
    Mouse->Set_Cursor(hotx, hoty, cursor);
  }
}

bool IsLeftButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_LMASK) != 0;
}

bool IsRightButtonDown() {
  return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_RMASK) != 0;
}

int Get_Mouse_X() {
  if (Mouse) {
    return Mouse->Get_Mouse_X();
  }
  return 0;
}

int Get_Mouse_Y() {
  if (Mouse) {
    return Mouse->Get_Mouse_Y();
  }
  return 0;
}

void Update_Mouse_Palette() {
  if (Mouse) {
    Mouse->Update_Palette();
  }
}

void Update_Mouse_Pos(int x, int y) {
  if (Mouse) {
    Mouse->Update_Pos(x, y);
  }
}
