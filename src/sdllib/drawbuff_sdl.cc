#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_timer.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "base/array.h"
#include "base/numeric.h"
#include "sdllib/graphic_buffer.h"
#include "sdllib/ww_win.h"

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
