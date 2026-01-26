// SDL2-based mouse cursor management. Handles decoding game cursor shapes,
// scaling them for high-DPI displays, and synchronizing with palette changes.

#include "sdllib/include/ww_mouse.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_video.h>

#include <cstring>
#include <utility>

#include "absl/log/log.h"
#include "sdllib/include/gbuffer.h"
#include "sdllib/include/iff.h"
#include "sdllib/include/shape.h"
#include "sdllib/include/ww_win.h"

// Global flag to disable mouse grabbing (for debugging)
bool NoMouseGrab = false;

void SDLCursorDeleter::operator()(SDL_Cursor* p) const noexcept {
  SDL_FreeCursor(p);
}

void SDLSurfaceDeleter::operator()(SDL_Surface* p) const noexcept {
  SDL_FreeSurface(p);
}

static WWMouseClass* _Mouse = nullptr;

// Nearest-neighbor scaling preserves the crisp pixel art look of game cursors.
[[nodiscard]] static uint8_t* Scale_Cursor_Nearest(const uint8_t* src,
                                                   int src_w, int src_h,
                                                   int scale, int* out_w,
                                                   int* out_h) {
  if (src_w <= 0 || src_h <= 0 || scale <= 0) {
    *out_w = 0;
    *out_h = 0;
    return nullptr;
  }

  int dst_w = src_w * scale;
  int dst_h = src_h * scale;
  auto* dst = new uint8_t[dst_w * dst_h];

  for (int y = 0; y < dst_h; ++y) {
    int src_y = y / scale;
    for (int x = 0; x < dst_w; ++x) {
      int src_x = x / scale;
      dst[y * dst_w + x] = src[src_y * src_w + src_x];
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
    int idx = SDL_GetWindowDisplayIndex(static_cast<SDL_Window*>(MainWindow));
    if (idx >= 0) display_index = idx;
  }

  SDL_DisplayMode mode;
  if (SDL_GetCurrentDisplayMode(display_index, &mode) != 0) {
    return 1;  // Fallback if can't get display info
  }

  constexpr int kLogicalHeight = 500;
  constexpr int kMaxScale = 4;
  int scale = mode.h / kLogicalHeight;
  if (scale < 1) return 1;
  if (scale > kMaxScale) return kMaxScale;
  return scale;
}

WWMouseClass::WWMouseClass([[maybe_unused]] GraphicViewPortClass* scr,
                           int max_width, int max_height)
    : MouseCursor(max_width * max_height),
      MaxWidth(max_width),
      MaxHeight(max_height),
      State(0) {
  Set_Cursor_Clip();
  _Mouse = this;
}

WWMouseClass::~WWMouseClass() { Clear_Cursor_Clip(); }

void WWMouseClass::Set_Cursor(int xhotspot, int yhotspot, void* cursor) {
  if (cursor == nullptr || PrevCursor == cursor) {
    return;
  }

  const auto* cursor_shape = static_cast<Shape_Type*>(cursor);

  if (cursor_shape->Width == 0 || cursor_shape->OriginalHeight == 0 ||
      cursor_shape->Width > MaxWidth ||
      cursor_shape->OriginalHeight > MaxHeight) {
    return;
  }

  // Only ShapeType 0 (LCW compressed, 256-color) is supported here.
  if (cursor_shape->ShapeType != 0) {
    DLOG(INFO) << "Set_Cursor type " << cursor_shape->ShapeType;
    return;
  }

  // Shape data is LCW compressed starting at byte 10 (after the header).
  auto* decompressed_data = new uint8_t[cursor_shape->DataLength];
  LCW_Uncompress(static_cast<uint8_t*>(cursor) + 10, decompressed_data,
                 cursor_shape->DataLength);

  // After LCW decompression, the shape is still RLE encoded.
  auto* inptr = decompressed_data;

  int remaining = cursor_shape->Width * cursor_shape->OriginalHeight;
  auto* outptr = MouseCursor.data();

  // Pre-zero buffer: RLE decoding may not write every pixel explicitly.
  memset(MouseCursor.data(), 0, remaining);

  do {
    uint8_t pixel = *inptr++;
    if (pixel) {
      *outptr++ = pixel;
      remaining--;
    } else {
      // RLE: zero byte followed by count of zeros to emit.
      int count = *inptr++;
      remaining -= count;
      while (count--) {
        *outptr++ = 0;
      }
    }
  } while (remaining);

  delete[] decompressed_data;

  // Keep the unscaled cursor for palette updates. When the game palette
  // changes, we recreate the SDL cursor from this copy.
  OriginalWidth = cursor_shape->Width;
  OriginalHeight = cursor_shape->OriginalHeight;
  int original_size = OriginalWidth * OriginalHeight;

  OriginalCursor.assign(MouseCursor.begin(),
                        MouseCursor.begin() + original_size);

  CurrentScale = Get_Display_Scale();
  int scaled_width, scaled_height;
  uint8_t* scaled_cursor =
      Scale_Cursor_Nearest(OriginalCursor.data(), OriginalWidth, OriginalHeight,
                           CurrentScale, &scaled_width, &scaled_height);
  if (!scaled_cursor) {
    return;
  }

  int scaled_hotx = xhotspot * CurrentScale;
  int scaled_hoty = yhotspot * CurrentScale;

  // SDL_CreateRGBSurfaceFrom doesn't copy pixel data, so we must use
  // SDL_CreateRGBSurface and copy manually to own the memory.
  SDLSurfacePtr sdl_surf(
      SDL_CreateRGBSurface(0, scaled_width, scaled_height, 8, 0, 0, 0, 0));
  if (!sdl_surf) {
    delete[] scaled_cursor;
    return;
  }
  // SDL surface pitch may include padding, so copy row by row.
  for (int y = 0; y < scaled_height; ++y) {
    memcpy(static_cast<uint8_t*>(sdl_surf->pixels) + y * sdl_surf->pitch,
           scaled_cursor + y * scaled_width, scaled_width);
  }
  delete[] scaled_cursor;

  if (WindowBuffer) {
    // Sync cursor palette with game palette. Index 0 is transparent.
    const auto* window_pal =
        static_cast<const SDL_Palette*>(WindowBuffer->Get_Palette());
    SDL_SetPaletteColors(sdl_surf->format->palette, window_pal->colors + 1, 1,
                         255);
    sdl_surf->format->palette->colors[0].a = 0;
  }

  SDLCursorPtr sdl_cursor(
      SDL_CreateColorCursor(sdl_surf.get(), scaled_hotx, scaled_hoty));

  SDL_SetCursor(sdl_cursor.get());

  PrevCursor = static_cast<char*>(cursor);
  sdl_cursor_ = std::move(sdl_cursor);
  sdl_surface_ = std::move(sdl_surf);
  MouseXHot = xhotspot;
  MouseYHot = yhotspot;
}

// Hide/Show use reference counting so nested hide/show pairs work correctly.
// The cursor is only actually hidden/shown when the count transitions to/from
// 0.

void WWMouseClass::Hide_Mouse() {
  if (!State++) SDL_ShowCursor(SDL_DISABLE);
}

void WWMouseClass::Show_Mouse() {
  if (!State) return;
  if (--State == 0) {
    if (PaletteDirty) Update_Palette();
    SDL_ShowCursor(SDL_ENABLE);
  }
}

void WWMouseClass::Conditional_Hide_Mouse(int /*x1*/, int /*y1*/, int /*x2*/,
                                          int /*y2*/) {}

void WWMouseClass::Conditional_Show_Mouse() {}

int WWMouseClass::Get_Mouse_State() { return State; }

int WWMouseClass::Get_Mouse_X() { return LastX; }

int WWMouseClass::Get_Mouse_Y() { return LastY; }

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
  if (!WindowBuffer || !sdl_surface_) return;

  if (State) {
    // Defer update until cursor is shown again.
    PaletteDirty = true;
    return;
  }

  PaletteDirty = false;

  const auto* window_pal =
      static_cast<const SDL_Palette*>(WindowBuffer->Get_Palette());
  SDL_SetPaletteColors(sdl_surface_->format->palette, window_pal->colors + 1, 1,
                       255);

  int scaled_hotx = MouseXHot * CurrentScale;
  int scaled_hoty = MouseYHot * CurrentScale;
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
  if (_Mouse) _Mouse->Hide_Mouse();
}

void Show_Mouse() {
  if (_Mouse) _Mouse->Show_Mouse();
}

void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2) {
  if (_Mouse) _Mouse->Conditional_Hide_Mouse(x1, y1, x2, y2);
}

void Conditional_Show_Mouse() {
  if (_Mouse) _Mouse->Conditional_Show_Mouse();
}

int Get_Mouse_State() {
  if (_Mouse) return _Mouse->Get_Mouse_State();
  return 0;
}

void Set_Mouse_Cursor(int hotx, int hoty, void* cursor) {
  if (_Mouse) {
    _Mouse->Set_Cursor(hotx, hoty, cursor);
  }
}

int Get_Mouse_X() {
  if (_Mouse) return _Mouse->Get_Mouse_X();
  return 0;
}

int Get_Mouse_Y() {
  if (_Mouse) return _Mouse->Get_Mouse_Y();
  return 0;
}

void Update_Mouse_Palette() {
  if (_Mouse) _Mouse->Update_Palette();
}

void Update_Mouse_Pos(int x, int y) {
  if (_Mouse) _Mouse->Update_Pos(x, y);
}
