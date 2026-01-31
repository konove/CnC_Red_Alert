#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_timer.h>

#include <cstdint>

#include "sdllib/include/gbuffer.h"
#include "sdllib/include/ww_win.h"

extern SDL_Renderer* SDLRenderer;

extern Uint32 ForceRenderEventID;
static Uint32 Force_Redraw_Timer(Uint32 /*interval*/, void*) {
  // something has been draw and not displayed for 33ms
  // go tell the main thread it should probably display that
  SDL_Event ev;
  ev.type = ForceRenderEventID;
  SDL_PushEvent(&ev);

  return 0;
}

bool GraphicBufferClass::Lock() {
  if (!PaletteSurface) return true;

  if (!LockCount) {
    SDL_LockSurface(static_cast<SDL_Surface*>(PaletteSurface));
    Offset = static_cast<uint8_t*>(((SDL_Surface*)PaletteSurface)->pixels);
  }

  LockCount++;
  return true;
}

bool GraphicBufferClass::Unlock() {
  if (!PaletteSurface || !LockCount) return true;

  LockCount--;

  if (!LockCount) {
    SDL_UnlockSurface(static_cast<SDL_Surface*>(PaletteSurface));
    Offset = nullptr;
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
    SDL_RenderPresent(SDLRenderer);
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
  SDL_Surface* tmp_surf;
  SDL_LockTextureToSurface(window_tex, nullptr, &tmp_surf);
  SDL_BlitSurface(static_cast<SDL_Surface*>(PaletteSurface), nullptr, tmp_surf,
                  nullptr);
  SDL_UnlockTexture(window_tex);

  // copy to screen
  SDL_RenderClear(SDLRenderer);
  SDL_RenderCopy(SDLRenderer, window_tex, nullptr, nullptr);
  SDL_RenderPresent(SDLRenderer);

  // update the event loop here too for now
  SDL_Event_Loop();
}

void GraphicBufferClass::Update_Palette(const uint8_t* palette) {
  auto* sdl_pal = static_cast<SDL_Surface*>(PaletteSurface)->format->palette;

  bool changed = false;

  for (int i = 0; i < sdl_pal->ncolors; i++) {
    // convert from 6-bit
    int new_r = palette[i * 3 + 0] << 2 | palette[i * 3 + 0] >> 4;
    int new_g = palette[i * 3 + 1] << 2 | palette[i * 3 + 1] >> 4;
    int new_b = palette[i * 3 + 2] << 2 | palette[i * 3 + 2] >> 4;

    changed = changed || sdl_pal->colors[i].r != new_r ||
              sdl_pal->colors[i].g != new_g || sdl_pal->colors[i].b != new_b;

    sdl_pal->colors[i].r = new_r;
    sdl_pal->colors[i].g = new_g;
    sdl_pal->colors[i].b = new_b;
  }

  if (!changed) return;

  // make sure it gets updated
  SDL_SetPaletteColors(sdl_pal, sdl_pal->colors, 0, sdl_pal->ncolors);

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

void GraphicBufferClass::Render_Scaled_Frame(const uint8_t* paletted_data,
                                             int width, int height) {
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

  // Get the palette already set via Update_Palette (already 8-bit RGB)
  auto* sdl_pal = static_cast<SDL_Surface*>(PaletteSurface)->format->palette;

  // Convert paletted pixels to RGBA and upload to intermediate texture
  void* pixels;
  int pitch;
  SDL_LockTexture(static_cast<SDL_Texture*>(VQATexture), nullptr, &pixels,
                  &pitch);

  auto* dest = static_cast<uint32_t*>(pixels);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      uint8_t idx = paletted_data[y * width + x];
      // Use palette already converted to 8-bit by Update_Palette
      uint8_t r = sdl_pal->colors[idx].r;
      uint8_t g = sdl_pal->colors[idx].g;
      uint8_t b = sdl_pal->colors[idx].b;
      dest[y * (pitch / 4) + x] = 0xFFU << 24 | b << 16 | g << 8 | r;
    }
  }
  SDL_UnlockTexture(static_cast<SDL_Texture*>(VQATexture));

  // Trigger immediate present via Update_Window_Surface
  Update_Window_Surface(true);
}

void GraphicBufferClass::Destroy_VQA_Texture() {
  if (VQATexture) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(VQATexture));
    VQATexture = nullptr;
    VQATextureWidth = 0;
    VQATextureHeight = 0;
  }
}
