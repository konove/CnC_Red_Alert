// SDL2-based mouse cursor management. Handles decoding game cursor shapes,
// scaling them for high-DPI displays, and synchronizing with palette changes.

#include "sdllib/ww_mouse.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_stdinc.h>
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
#include "sdllib/gbuffer.h"
#include "sdllib/iff.h"
#include "sdllib/shape.h"
#include "sdllib/ww_win.h"

// Global flag to disable mouse grabbing (for debugging)
bool NoMouseGrab = false;

void SDLCursorDeleter::operator()(SDL_Cursor* p) const noexcept {
  SDL_FreeCursor(p);
}

void SDLSurfaceDeleter::operator()(SDL_Surface* p) const noexcept {
  SDL_FreeSurface(p);
}

static WWMouseClass* Mouse = nullptr;

// Nearest-neighbor scaling preserves the crisp pixel art look of game cursors.
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

// The original game was designed for ~400px vertical resolution. On modern
// high-DPI displays, cursors need scaling to remain usable. SDL's logical
// rendering handles game graphics, but hardware cursors bypass it.
static int Get_Display_Scale() {
  int display_index = 0;
  if (MainWindow) {
    const int idx =
        SDL_GetWindowDisplayIndex(static_cast<SDL_Window*>(MainWindow));
    if (idx >= 0) {
      display_index = idx;
    }
  }

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(display_index, &mode) != 0) {
    return 1;  // Fallback if can't get display info
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

WWMouseClass::WWMouseClass([[maybe_unused]] GraphicViewPortClass* scr,
                           int max_width, int max_height)
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

  // Keep the unscaled cursor for palette updates. When the game palette
  // changes, we recreate the SDL cursor from this copy.
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

  // SDL_CreateRGBSurfaceFrom doesn't copy pixel data, so we must use
  // SDL_CreateRGBSurface and copy manually to own the memory.
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

  if (WindowBuffer) {
    // Sync cursor palette with game palette. Index 0 is transparent.
    const auto* window_pal =
        static_cast<const SDL_Palette*>(WindowBuffer->Get_Palette());
    // SDL owns ncolors color entries in this palette.
    SDL_SetPaletteColors(
        sdl_surf->format->palette,
        // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
        std::span(window_pal->colors, base::ToSize(window_pal->ncolors))
            .subspan(1)
            .data(),
        1, 255);
    sdl_surf->format->palette->colors[0].a = 0;
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

// Hide/Show use reference counting so nested hide/show pairs work correctly.
// The cursor is only actually hidden/shown when the count transitions to/from
// 0.

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

void WWMouseClass::Draw_Mouse(GraphicViewPortClass* /*scr*/) {
  // No-op: SDL hardware cursor is drawn by the OS, not by us.
}

void WWMouseClass::Erase_Mouse(GraphicViewPortClass* /*scr*/, bool /*forced*/) {
}

void WWMouseClass::Set_Cursor_Clip() {
  if (!NoMouseGrab) {
    SDL_SetWindowGrab(static_cast<SDL_Window*>(MainWindow), SDL_TRUE);
  }
}

void WWMouseClass::Clear_Cursor_Clip() {
  SDL_SetWindowGrab(static_cast<SDL_Window*>(MainWindow), SDL_FALSE);
}

// SDL bakes palette colors into the cursor at creation time, so we must
// recreate the cursor whenever the game palette changes.
void WWMouseClass::Update_Palette() {
  if (!WindowBuffer || !sdl_surface_) {
    return;
  }

  if (State) {
    // Defer update until cursor is shown again.
    PaletteDirty = true;
    return;
  }

  PaletteDirty = false;

  const auto* window_pal =
      static_cast<const SDL_Palette*>(WindowBuffer->Get_Palette());
  // SDL owns ncolors entries in the window palette.
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

// C-style API for legacy game code. These delegate to the singleton.

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
