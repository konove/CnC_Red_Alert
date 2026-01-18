#include "sdllib/include/ww_mouse.h"

#include <SDL_events.h>
#include <SDL_mouse.h>
#include <SDL_pixels.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_video.h>

#include <cstdio>

#include "sdllib/include/gbuffer.h"
#include "sdllib/include/iff.h"
#include "sdllib/include/shape.h"
#include "sdllib/include/ww_win.h"

// Global flag to disable mouse grabbing (for debugging)
bool NoMouseGrab = false;

static WWMouseClass* _Mouse = nullptr;

// Scale cursor using nearest-neighbor interpolation.
[[nodiscard]] static uint8_t* Scale_Cursor_Nearest(const uint8_t* src,
                                                   int src_w, int src_h,
                                                   int scale, int* out_w,
                                                   int* out_h) {
  int dst_w = src_w * scale;
  int dst_h = src_h * scale;
  auto dst = new uint8_t[dst_w * dst_h];

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

// Calculate scale factor based on display resolution.
// Original game designed for ~400 pixel vertical resolution (accounting for
// the fact that game graphics are already scaled via SDL logical rendering).
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

WWMouseClass::WWMouseClass(GraphicViewPortClass* /*scr*/, int mouse_max_width,
                           int mouse_max_height)
    : MaxWidth(mouse_max_width), MaxHeight(mouse_max_height), State(0) {
  Set_Cursor_Clip();
  _Mouse = this;

  MouseCursor = new uint8_t[MaxWidth * MaxHeight];
}

WWMouseClass::~WWMouseClass() {
  Clear_Cursor_Clip();

  if (SDLCursor) {
    SDL_FreeCursor(static_cast<SDL_Cursor*>(SDLCursor));
  }
  if (SDLSurface) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(SDLSurface));
  }

  delete[] MouseCursor;
  delete[] OriginalCursor;
}

void* WWMouseClass::Set_Cursor(int xhotspot, int yhotspot, void* cursor) {
  if (!cursor || PrevCursor == cursor) return cursor;

  auto cursor_shape = (Shape_Type*)cursor;

  if (cursor_shape->Width > MaxWidth ||
      cursor_shape->OriginalHeight > MaxHeight)
    return PrevCursor;

  // don't handle 16-color or uncompressed
  if (cursor_shape->ShapeType != 0) {
    printf("Set_Cursor type %i\n", cursor_shape->ShapeType);
    return PrevCursor;
  }

  // decompress it
  auto decompressed_data = new uint8_t[cursor_shape->DataLength];
  LCW_Uncompress((uint8_t*)cursor + 10, decompressed_data,
                 cursor_shape->DataLength);

  // now we have an uncmpressed, but still encoded shape
  auto inptr = decompressed_data;

  int remaining = cursor_shape->Width * cursor_shape->OriginalHeight;
  auto outptr = MouseCursor;

  do {
    uint8_t pixel = *inptr++;
    if (pixel) {
      *outptr++ = pixel;
      remaining--;
    } else {
      // run of zeros
      int count = *inptr++;
      remaining -= count;
      while (count--) {
        *outptr++ = 0;
      }
    }
  } while (remaining);

  delete[] decompressed_data;

  // Store original cursor for palette updates
  OriginalWidth = cursor_shape->Width;
  OriginalHeight = cursor_shape->OriginalHeight;
  int original_size = OriginalWidth * OriginalHeight;

  delete[] OriginalCursor;
  OriginalCursor = new uint8_t[original_size];
  memcpy(OriginalCursor, MouseCursor, original_size);

  // Scale cursor based on display resolution
  CurrentScale = Get_Display_Scale();
  int scaled_width, scaled_height;
  uint8_t* scaled_cursor =
      Scale_Cursor_Nearest(OriginalCursor, OriginalWidth, OriginalHeight,
                           CurrentScale, &scaled_width, &scaled_height);

  // Scale hotspot coordinates
  int scaled_hotx = xhotspot * CurrentScale;
  int scaled_hoty = yhotspot * CurrentScale;

  // Create SDL surface - use SDL_CreateRGBSurface and copy data for proper
  // memory ownership (SDL_CreateRGBSurfaceFrom doesn't copy the pixel data)
  auto sdl_surf =
      SDL_CreateRGBSurface(0, scaled_width, scaled_height, 8, 0, 0, 0, 0);
  if (!sdl_surf) {
    delete[] scaled_cursor;
    return PrevCursor;
  }
  // Copy row by row to respect SDL surface pitch (may differ from width)
  for (int y = 0; y < scaled_height; ++y) {
    memcpy(static_cast<uint8_t*>(sdl_surf->pixels) + y * sdl_surf->pitch,
           scaled_cursor + y * scaled_width, scaled_width);
  }
  delete[] scaled_cursor;

  if (WindowBuffer) {
    // copy palette from window surface, but make index 0 transparent
    auto window_pal = (const SDL_Palette*)WindowBuffer->Get_Palette();
    SDL_SetPaletteColors(sdl_surf->format->palette, window_pal->colors + 1, 1,
                         255);
    sdl_surf->format->palette->colors[0].a = 0;
  }

  auto sdl_cursor = SDL_CreateColorCursor(sdl_surf, scaled_hotx, scaled_hoty);

  // set it and clean up
  auto old_cursor = PrevCursor;
  SDL_SetCursor(sdl_cursor);

  if (SDLCursor) SDL_FreeCursor((SDL_Cursor*)SDLCursor);

  if (SDLSurface) SDL_FreeSurface((SDL_Surface*)SDLSurface);

  PrevCursor = (char*)cursor;
  SDLCursor = sdl_cursor;
  SDLSurface = sdl_surf;
  MouseXHot = xhotspot;
  MouseYHot = yhotspot;
  return old_cursor;
}

void WWMouseClass::Hide_Mouse(void) {
  if (!State++) SDL_ShowCursor(SDL_DISABLE);
}

void WWMouseClass::Show_Mouse(void) {
  if (!State) return;
  if (--State == 0) {
    if (PaletteDirty) Update_Palette();
    SDL_ShowCursor(SDL_ENABLE);
  }
}

void WWMouseClass::Conditional_Hide_Mouse(int /*x1*/, int /*y1*/, int /*x2*/,
                                          int /*y2*/) {}

void WWMouseClass::Conditional_Show_Mouse(void) {}

int WWMouseClass::Get_Mouse_State(void) { return State; }

int WWMouseClass::Get_Mouse_X(void) { return LastX; }

int WWMouseClass::Get_Mouse_Y(void) { return LastY; }

void WWMouseClass::Draw_Mouse(GraphicViewPortClass* /*scr*/) {
  // we're using a "hardware" cursor, so don't need to do anything
}

void WWMouseClass::Erase_Mouse(GraphicViewPortClass* /*scr*/, bool /*forced*/) {
}

void WWMouseClass::Set_Cursor_Clip(void) {
  if (!NoMouseGrab) {
    SDL_SetWindowGrab((SDL_Window*)MainWindow, SDL_TRUE);
  }
}

void WWMouseClass::Clear_Cursor_Clip(void) {
  SDL_SetWindowGrab((SDL_Window*)MainWindow, SDL_FALSE);
}

void WWMouseClass::Update_Palette() {
  if (!WindowBuffer || !SDLSurface) return;

  if (State) {
    // don't do anything now if cursor is hidden anyway
    PaletteDirty = true;
    return;
  }

  PaletteDirty = false;

  auto sdl_surf = (SDL_Surface*)SDLSurface;

  // copy palette from window surface
  auto window_pal = (const SDL_Palette*)WindowBuffer->Get_Palette();
  SDL_SetPaletteColors(sdl_surf->format->palette, window_pal->colors + 1, 1,
                       255);

  // recreate and set cursor with scaled hotspot
  int scaled_hotx = MouseXHot * CurrentScale;
  int scaled_hoty = MouseYHot * CurrentScale;
  auto sdl_cursor = SDL_CreateColorCursor(sdl_surf, scaled_hotx, scaled_hoty);
  SDL_SetCursor(sdl_cursor);

  // clean up old cursor
  if (SDLCursor) SDL_FreeCursor((SDL_Cursor*)SDLCursor);
  SDLCursor = sdl_cursor;
}

void WWMouseClass::Update_Pos(int x, int y) {
  LastX = x;
  LastY = y;
}

void Hide_Mouse(void) {
  if (_Mouse) _Mouse->Hide_Mouse();
}

void Show_Mouse(void) {
  if (_Mouse) _Mouse->Show_Mouse();
}

void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2) {
  if (_Mouse) _Mouse->Conditional_Hide_Mouse(x1, y1, x2, y2);
}

void Conditional_Show_Mouse(void) {
  if (_Mouse) _Mouse->Conditional_Show_Mouse();
}

int Get_Mouse_State(void) {
  if (_Mouse) return _Mouse->Get_Mouse_State();
  return 0;
}

void* Set_Mouse_Cursor(int hotx, int hoty, void* cursor) {
  if (_Mouse) return _Mouse->Set_Cursor(hotx, hoty, cursor);
  return nullptr;
}

int Get_Mouse_X(void) {
  if (_Mouse) return _Mouse->Get_Mouse_X();
  return 0;
}

int Get_Mouse_Y(void) {
  if (_Mouse) return _Mouse->Get_Mouse_Y();
  return 0;
}

void Update_Mouse_Palette() {
  if (_Mouse) _Mouse->Update_Palette();
}

void Update_Mouse_Pos(int x, int y) {
  if (_Mouse) _Mouse->Update_Pos(x, y);
}
