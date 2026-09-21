/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// File: The out-of-line members of PixelView and PixelBuffer: attaching a view
// to a page, giving a page its pixels, and - for the one page the window shows
// - the SDL surface and textures behind it and the presenting done through
// them. The drawing primitives themselves are the free Buffer_* functions in
// drawbuff.cc.

#include "sdllib/pixel_buffer.h"

#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_timer.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/bitmap.h"
#include "sdllib/drawbuff.h"
#include "sdllib/ww_win.h"

PixelBuffer* WindowBuffer = nullptr;

PixelView::~PixelView() {
  if (LogicPage == this) {
    LogicPage = nullptr;
  }
}

PixelView* SetLogicPage(PixelView* page) {
  std::swap(LogicPage, page);
  return page;
}

PixelView* SetLogicPage(PixelView& page) { return SetLogicPage(&page); }

PixelView::PixelView(PixelBuffer* buffer, int x, int y, int width, int height) {
  Attach(buffer, x, y, width, height);
}

void PixelView::DrawRect(int x1, int y1, int x2, int y2, uint8_t color) {
  Lock();
  DrawLine(x1, y1, x2, y1, color);
  DrawLine(x1, y2, x2, y2, color);
  DrawLine(x1, y1, x1, y2, color);
  DrawLine(x2, y1, x2, y2, color);
  Unlock();
}

void PixelView::Attach(PixelBuffer* buffer, int x, int y, int width,
                       int height) {
  if (this == buffer_) {
    return;
  }

  // Clamp the corner into the buffer. A buffer that Init() has not sized yet
  // has no last pixel to clamp to, so the corner stays at the origin and the
  // width and height below come out zero; Screen builds its views against
  // such pages and attaches them again once the video mode is known.
  x = std::clamp(x, 0, std::max(buffer->width() - 1, 0));
  y = std::clamp(y, 0, std::max(buffer->height() - 1, 0));

  if (x + width > buffer->width()) {
    width = buffer->width() - x;
  }

  if (y + height > buffer->height()) {
    height = buffer->height() - y;
  }

  /*======================================================================*/
  /* Get a pointer to the top left edge of the buffer.
   */
  /*======================================================================*/
  offset_ =
      buffer->Get_Bytes().empty()
          ? nullptr
          : buffer->Get_Bytes()
                .subspan(base::ToSize((static_cast<base::ssize>(
                                           buffer->width() + buffer->pitch()) *
                                       y) +
                                      x))
                .data();

  /*======================================================================*/
  /* Copy over all of the variables that we need to store.
   */
  /*======================================================================*/
  x_pos_ = x;
  y_pos_ = y;
  x_add_ = buffer->width() - width;
  width_ = width;
  height_ = height;
  pitch_ = buffer->pitch();
  buffer_ = buffer;
}

PixelBuffer::PixelBuffer(int width, int height, std::span<uint8_t> buffer,
                         int32_t byte_count)
    : PixelBuffer() {
  Init(width, height, buffer, byte_count, BUFFER_NONE);
}

PixelBuffer::PixelBuffer(int width, int height, std::span<uint8_t> buffer)
    : PixelBuffer(width, height, buffer, width * height) {}

PixelBuffer::PixelBuffer() { buffer_ = this; }

PixelBuffer::~PixelBuffer() {
  ReleaseSurfaces();
  if (WindowBuffer == this) {
    WindowBuffer = nullptr;
  }
}

void PixelBuffer::Init(int width, int height, std::span<uint8_t> buffer,
                       int32_t byte_count, PixelBufferFlags flags) {
  CHECK_GE(width, 0);
  CHECK_GE(height, 0);
  CHECK_GE(byte_count, 0);
  const auto pixel_count = base::ToSize(width) * base::ToSize(height);
  if (!base::Any(flags & BUFFER_VISIBLE)) {
    CHECK_LE(pixel_count,
             buffer.empty()
                 ? (byte_count == 0 ? pixel_count : base::ToSize(byte_count))
                 : buffer.size());
  }
  Size = byte_count;
  width_ = width;
  height_ = height;
  pitch_ = 0;
  x_add_ = 0;
  x_pos_ = y_pos_ = 0;

  if (base::Any(flags & BUFFER_VISIBLE)) {
    CreateDisplaySurface();

    WindowBuffer = this;
  } else {
    // regular allocation
    Allocated = buffer.empty();
    bytes_ = buffer;
    Buffer = buffer.data();

    if (buffer.empty()) {
      if (byte_count == 0) {
        Size = width * height;
      } else {
        Size = byte_count;
      }
      Buffer = new uint8_t[base::ToSize(Size)];
      // This allocation contains exactly Size bytes.
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      bytes_ = std::span(static_cast<uint8_t*>(Buffer), base::ToSize(Size));
    }

    offset_ = static_cast<uint8_t*>(Buffer);
  }
}

void PixelBuffer::ReleaseSurfaces() { DestroyDisplaySurface(); }

static Uint32 Force_Redraw_Timer(Uint32 /*interval*/, void* /*unused*/) {
  // something has been draw and not displayed for 33ms
  // go tell the main thread it should probably display that
  SDL_Event ev;
  ev.type = ForceRenderEventID;
  SDL_PushEvent(&ev);

  return 0;
}

bool PixelBuffer::LockSurface() {
  if (!palette_surface_) {
    return true;
  }

  if (!lock_count_) {
    if (SDL_LockSurface(static_cast<SDL_Surface*>(palette_surface_)) != 0) {
      return false;
    }
    const auto* surface = static_cast<SDL_Surface*>(palette_surface_);
    offset_ = static_cast<uint8_t*>(surface->pixels);
    // SDL_LockSurface exposes pitch bytes for each of the surface's rows until
    // it is unlocked.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    bytes_ = std::span(offset_,
                       base::ToSize(surface->pitch) * base::ToSize(surface->h));
  }

  lock_count_++;
  return true;
}

bool PixelBuffer::UnlockSurface() {
  if (!palette_surface_ || !lock_count_) {
    return true;
  }

  lock_count_--;

  if (!lock_count_) {
    SDL_UnlockSurface(static_cast<SDL_Surface*>(palette_surface_));
    offset_ = nullptr;
    bytes_ = {};
    // Content was drawn to palette_surface_ - clear VQA texture to switch back
    // to normal rendering mode
    if (scaled_frame_texture_) {
      DropScaledFrame();
    }
    Present(false);
  }

  return true;
}

void PixelBuffer::Present(bool end_frame) {
  // If VQA texture exists, keep presenting it (for animations like map select
  // that need to preserve the last frame indefinitely)
  if (scaled_frame_texture_) {
    if (redraw_timer_) {
      SDL_RemoveTimer(redraw_timer_);
      redraw_timer_ = 0;
    }

    if (!end_frame) {
      return;  // Just skip timer setup during VQA
    }

    // Present the VQA frame
    SDL_RenderClear(SDLRenderer);
    SDL_RenderCopy(SDLRenderer,
                   static_cast<SDL_Texture*>(scaled_frame_texture_), nullptr,
                   nullptr);
    PresentFrame();
    SDL_Event_Loop();
    return;
  }

  auto* window_tex = static_cast<SDL_Texture*>(window_texture_);

  if (!end_frame) {
    if (!redraw_timer_) {
      redraw_timer_ = SDL_AddTimer(1000 / 30, Force_Redraw_Timer, nullptr);
    }
    return;
  }

  if (redraw_timer_) {
    SDL_RemoveTimer(redraw_timer_);
    redraw_timer_ = 0;
  }

  // blit from paletted surface
  SDL_Surface* tmp_surf = nullptr;
  SDL_LockTextureToSurface(window_tex, nullptr, &tmp_surf);
  SDL_BlitSurface(static_cast<SDL_Surface*>(palette_surface_), nullptr,
                  tmp_surf, nullptr);
  SDL_UnlockTexture(window_tex);

  // copy to screen
  SDL_RenderClear(SDLRenderer);
  SDL_RenderCopy(SDLRenderer, window_tex, nullptr, nullptr);
  PresentFrame();

  // update the event loop here too for now
  SDL_Event_Loop();
}

void PixelBuffer::UpdatePalette(std::span<const uint8_t> palette) {
  auto* sdl_pal = static_cast<SDL_Surface*>(palette_surface_)->format->palette;
  if (palette.size() / 3 < base::ToSize(sdl_pal->ncolors)) {
    return;
  }
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_pal->colors, base::ToSize(sdl_pal->ncolors));

  bool changed = false;

  for (int i = 0; i < sdl_pal->ncolors; i++) {
    // convert from 6-bit
    const int new_r = (base::At(palette, base::ToSize((i * 3) + 0)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 0)) / 16);
    const int new_g = (base::At(palette, base::ToSize((i * 3) + 1)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 1)) / 16);
    const int new_b = (base::At(palette, base::ToSize((i * 3) + 2)) * 4) +
                      (base::At(palette, base::ToSize((i * 3) + 2)) / 16);
    changed = changed ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).r, new_r) ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).g, new_g) ||
              std::cmp_not_equal(base::At(colors, base::ToSize(i)).b, new_b);
    base::At(colors, base::ToSize(i)).r = static_cast<Uint8>(new_r);
    base::At(colors, base::ToSize(i)).g = static_cast<Uint8>(new_g);
    base::At(colors, base::ToSize(i)).b = static_cast<Uint8>(new_b);
  }

  if (!changed) {
    return;
  }

  // make sure it gets updated
  SDL_SetPaletteColors(sdl_pal, sdl_pal->colors, 0, sdl_pal->ncolors);

  // A scaled frame holds baked colors; the next end of frame presents it.
  if (scaled_frame_texture_) {
    UploadScaledFrame();
  }

  Present(false);
}

const void* PixelBuffer::palette() const {
  return static_cast<SDL_Surface*>(palette_surface_)->format->palette;
}

void PixelBuffer::CreateDisplaySurface() {
  window_texture_ =
      SDL_CreateTexture(SDLRenderer, SDL_PIXELFORMAT_RGB888,
                        SDL_TEXTUREACCESS_STREAMING, width_, height_);
  palette_surface_ = SDL_CreateRGBSurface(0, width_, height_, 8, 0, 0, 0, 0);
}

void PixelBuffer::DestroyDisplaySurface() {
  if (redraw_timer_) {
    SDL_RemoveTimer(redraw_timer_);
    redraw_timer_ = 0;
  }
  DropScaledFrame();
  if (window_texture_) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(window_texture_));
    window_texture_ = nullptr;
  }
  if (palette_surface_) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
    palette_surface_ = nullptr;
  }
}

void PixelBuffer::PresentScaledFrame(std::span<const uint8_t> frame, int width,
                                     int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  const auto frame_width = base::ToSize(width);
  const auto frame_height = base::ToSize(height);
  if (frame_width > frame.size() / frame_height || !palette_surface_) {
    return;
  }
  // Cancel any pending redraw timer
  if (redraw_timer_) {
    SDL_RemoveTimer(redraw_timer_);
    redraw_timer_ = 0;
  }

  // Create intermediate texture on first use or if size changed
  if (!scaled_frame_texture_ || scaled_frame_width_ != width ||
      scaled_frame_height_ != height) {
    if (scaled_frame_texture_) {
      SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    }
    scaled_frame_texture_ =
        SDL_CreateTexture(SDLRenderer, SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureScaleMode(static_cast<SDL_Texture*>(scaled_frame_texture_),
                            SDL_ScaleModeBest);
    scaled_frame_width_ = width;
    scaled_frame_height_ = height;
  }

  scaled_frame_.assign(
      frame.begin(),
      frame.begin() + static_cast<std::ptrdiff_t>(frame_width * frame_height));
  if (!UploadScaledFrame()) {
    return;
  }

  // Trigger immediate present via Present
  Present(true);
}

bool PixelBuffer::UploadScaledFrame() {
  const auto frame_width = base::ToSize(scaled_frame_width_);
  const std::span<const uint8_t> frame = scaled_frame_;

  // Get the palette already set via UpdatePalette (already 8-bit RGB)
  const auto* sdl_pal =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;

  // Convert paletted pixels to RGBA and upload to intermediate texture
  void* pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(static_cast<SDL_Texture*>(scaled_frame_texture_), nullptr,
                      &pixels, &pitch) != 0) {
    return false;
  }
  // SDL_LockTexture exposes pitch bytes for each texture row.
  const auto dest =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint32_t*>(pixels),
                base::ToSize(pitch / 4) * base::ToSize(scaled_frame_height_));
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_pal->colors, base::ToSize(sdl_pal->ncolors));
  for (int y = 0; y < scaled_frame_height_; y++) {
    for (int x = 0; x < scaled_frame_width_; x++) {
      const uint8_t idx =
          base::At(frame, (base::ToSize(y) * frame_width) + base::ToSize(x));
      // Use palette already converted to 8-bit by UpdatePalette
      const uint8_t r = base::At(colors, idx).r;
      const uint8_t g = base::At(colors, idx).g;
      const uint8_t b = base::At(colors, idx).b;
      base::At(dest,
               (base::ToSize(y) * base::ToSize(pitch / 4)) + base::ToSize(x)) =
          0xFFU << 24U | uint32_t{b} << 16U | uint32_t{g} << 8U | r;
    }
  }
  SDL_UnlockTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
  return true;
}

void PixelBuffer::DropScaledFrame() {
  if (scaled_frame_texture_) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    scaled_frame_texture_ = nullptr;
    scaled_frame_width_ = 0;
    scaled_frame_height_ = 0;
    scaled_frame_.clear();
  }
}

// Walks the destination rather than the source: every destination pixel is
// mapped back through the inverse transform to the bitmap pixel it came
// from. Walking the source instead would scatter its pixels and leave holes
// wherever the scale stretches the image.
void PixelBuffer::DrawScaledRotated(const BitmapClass& bitmap,
                                    const TPoint2D& center, const int32_t scale,
                                    const uint8_t angle) {
  if (scale == 0) {
    return;
  }

  // The game measures angles in 256ths of a circle, like DirType.
  const double radians = angle * 2.0 * std::numbers::pi / 256.0;
  const double cos_a = std::cos(radians);
  const double sin_a = std::sin(radians);

  // scale is 24.8 fixed point, so 256 / scale is its reciprocal, which is
  // what the destination-to-source direction needs.
  const double inv_S = 256.0 / scale;
  const double cx_bmp = bitmap.Width / 2.0;
  const double cy_bmp = bitmap.Height / 2.0;

  // Rows in this buffer are width_ apart: DrawScaledRotated is a member of the
  // buffer rather than of a view, and Init() leaves x_add_ and pitch_ zero
  // for every buffer the games allocate.
  const auto dst_buf = Get_Bytes();

  for (int y2 = 0; y2 < height_; y2++) {
    for (int x2 = 0; x2 < width_; x2++) {
      const double rx = x2 - center.x;
      const double ry = y2 - center.y;

      // The matrix is [[sin, cos], [-cos, sin]], a rotation by angle minus a
      // quarter turn; the caller adds that quarter turn back when it converts
      // its facing. Its determinant is 1, so undoing the scale is the single
      // multiply by inv_S.
      const int bx =
          static_cast<int>((((sin_a * rx) + (cos_a * ry)) * inv_S) + cx_bmp);
      const int by =
          static_cast<int>((((-cos_a * rx) + (sin_a * ry)) * inv_S) + cy_bmp);

      // Destination pixels that map outside the bitmap keep what was there,
      // as does pixel 0, which is the transparent index.
      if (bx >= 0 && bx < bitmap.Width && by >= 0 && by < bitmap.Height) {
        const uint8_t pixel =
            base::At(bitmap.Data, base::ToSize((by * bitmap.Width) + bx));
        if (pixel != 0) {
          base::At(dst_buf, base::ToSize((y2 * width_) + x2)) = pixel;
        }
      }
    }
  }
}

void Video_End_Frame() {
  if (WindowBuffer) {
    WindowBuffer->Present(true);
  }
}
