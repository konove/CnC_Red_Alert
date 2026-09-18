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
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

static Uint32 Force_Redraw_Timer(Uint32 /*interval*/, void* /*unused*/) {
  // something has been draw and not displayed for 33ms
  // go tell the main thread it should probably display that
  SDL_Event ev;
  ev.type = ForceRenderEventID;
  SDL_PushEvent(&ev);

  return 0;
}

bool GraphicBufferClass::Lock_Surface() {
  if (!PaletteSurface) {
    return true;
  }

  if (!LockCount) {
    if (SDL_LockSurface(static_cast<SDL_Surface*>(PaletteSurface)) != 0) {
      return false;
    }
    const auto* surface = static_cast<SDL_Surface*>(PaletteSurface);
    Offset = static_cast<uint8_t*>(surface->pixels);
    // SDL_LockSurface exposes pitch bytes for each of h rows until unlock.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    bytes_ = std::span(Offset,
                       base::ToSize(surface->pitch) * base::ToSize(surface->h));
  }

  LockCount++;
  return true;
}

bool GraphicBufferClass::Unlock_Surface() {
  if (!PaletteSurface || !LockCount) {
    return true;
  }

  LockCount--;

  if (!LockCount) {
    SDL_UnlockSurface(static_cast<SDL_Surface*>(PaletteSurface));
    Offset = nullptr;
    bytes_ = {};
    // Content was drawn to PaletteSurface - clear VQA texture to switch back
    // to normal rendering mode
    if (VQATexture) {
      Destroy_VQA_Texture();
    }
    Update_Window_Surface(false);
  }

  return true;
}

void GraphicBufferClass::Update_Window_Surface(bool end_frame) {
  // If VQA texture exists, keep presenting it (for animations like map select
  // that need to preserve the last frame indefinitely)
  if (VQATexture) {
    if (RedrawTimer) {
      SDL_RemoveTimer(RedrawTimer);
      RedrawTimer = 0;
    }

    if (!end_frame) {
      return;  // Just skip timer setup during VQA
    }

    // Present the VQA frame
    SDL_RenderClear(SDLRenderer);
    SDL_RenderCopy(SDLRenderer, static_cast<SDL_Texture*>(VQATexture), nullptr,
                   nullptr);
    PresentFrame();
    SDL_Event_Loop();
    return;
  }

  auto* window_tex = static_cast<SDL_Texture*>(WindowTexture);

  if (!end_frame) {
    if (!RedrawTimer) {
      RedrawTimer = SDL_AddTimer(1000 / 30, Force_Redraw_Timer, nullptr);
    }
    return;
  }

  if (RedrawTimer) {
    SDL_RemoveTimer(RedrawTimer);
    RedrawTimer = 0;
  }

  // blit from paletted surface
  SDL_Surface* tmp_surf = nullptr;
  SDL_LockTextureToSurface(window_tex, nullptr, &tmp_surf);
  SDL_BlitSurface(static_cast<SDL_Surface*>(PaletteSurface), nullptr, tmp_surf,
                  nullptr);
  SDL_UnlockTexture(window_tex);

  // copy to screen
  SDL_RenderClear(SDLRenderer);
  SDL_RenderCopy(SDLRenderer, window_tex, nullptr, nullptr);
  PresentFrame();

  // update the event loop here too for now
  SDL_Event_Loop();
}
void GraphicBufferClass::Update_Palette(std::span<const uint8_t> palette) {
  auto* sdl_pal = static_cast<SDL_Surface*>(PaletteSurface)->format->palette;
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
  if (VQATexture) {
    Upload_Scaled_Frame();
  }

  Update_Window_Surface(false);
}

const void* GraphicBufferClass::Get_Palette() const {
  return static_cast<SDL_Surface*>(PaletteSurface)->format->palette;
}

void GraphicBufferClass::Init_Display_Surface() {
  WindowTexture = SDL_CreateTexture(SDLRenderer, SDL_PIXELFORMAT_RGB888,
                                    SDL_TEXTUREACCESS_STREAMING, Width, Height);
  PaletteSurface = SDL_CreateRGBSurface(0, Width, Height, 8, 0, 0, 0, 0);
}
void GraphicBufferClass::Render_Scaled_Frame(
    std::span<const uint8_t> paletted_data, int width, int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  const auto frame_width = base::ToSize(width);
  const auto frame_height = base::ToSize(height);
  if (frame_width > paletted_data.size() / frame_height || !PaletteSurface) {
    return;
  }
  // Cancel any pending redraw timer
  if (RedrawTimer) {
    SDL_RemoveTimer(RedrawTimer);
    RedrawTimer = 0;
  }

  // Create intermediate texture on first use or if size changed
  if (!VQATexture || VQATextureWidth != width || VQATextureHeight != height) {
    if (VQATexture) {
      SDL_DestroyTexture(static_cast<SDL_Texture*>(VQATexture));
    }
    VQATexture = SDL_CreateTexture(SDLRenderer, SDL_PIXELFORMAT_RGBA32,
                                   SDL_TEXTUREACCESS_STREAMING, width, height);
    SDL_SetTextureScaleMode(static_cast<SDL_Texture*>(VQATexture),
                            SDL_ScaleModeBest);
    VQATextureWidth = width;
    VQATextureHeight = height;
  }

  scaled_frame_.assign(paletted_data.begin(),
                       paletted_data.begin() + static_cast<std::ptrdiff_t>(
                                                   frame_width * frame_height));
  if (!Upload_Scaled_Frame()) {
    return;
  }

  // Trigger immediate present via Update_Window_Surface
  Update_Window_Surface(true);
}

bool GraphicBufferClass::Upload_Scaled_Frame() {
  const auto frame_width = base::ToSize(VQATextureWidth);
  const std::span<const uint8_t> frame = scaled_frame_;

  // Get the palette already set via Update_Palette (already 8-bit RGB)
  const auto* sdl_pal =
      static_cast<SDL_Surface*>(PaletteSurface)->format->palette;

  // Convert paletted pixels to RGBA and upload to intermediate texture
  void* pixels = nullptr;
  int pitch = 0;
  if (SDL_LockTexture(static_cast<SDL_Texture*>(VQATexture), nullptr, &pixels,
                      &pitch) != 0) {
    return false;
  }
  // SDL_LockTexture exposes pitch bytes for each texture row.
  const auto dest =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint32_t*>(pixels),
                base::ToSize(pitch / 4) * base::ToSize(VQATextureHeight));
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_pal->colors, base::ToSize(sdl_pal->ncolors));
  for (int y = 0; y < VQATextureHeight; y++) {
    for (int x = 0; x < VQATextureWidth; x++) {
      const uint8_t idx =
          base::At(frame, (base::ToSize(y) * frame_width) + base::ToSize(x));
      // Use palette already converted to 8-bit by Update_Palette
      const uint8_t r = base::At(colors, idx).r;
      const uint8_t g = base::At(colors, idx).g;
      const uint8_t b = base::At(colors, idx).b;
      base::At(dest,
               (base::ToSize(y) * base::ToSize(pitch / 4)) + base::ToSize(x)) =
          0xFFU << 24U | uint32_t{b} << 16U | uint32_t{g} << 8U | r;
    }
  }
  SDL_UnlockTexture(static_cast<SDL_Texture*>(VQATexture));
  return true;
}

void GraphicBufferClass::Destroy_VQA_Texture() {
  if (VQATexture) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(VQATexture));
    VQATexture = nullptr;
    VQATextureWidth = 0;
    VQATextureHeight = 0;
    scaled_frame_.clear();
  }
}
