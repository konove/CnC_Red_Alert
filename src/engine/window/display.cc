// Implements Display: opening the window, the paletted surface the game draws
// on and the textures that present it, and the pacing that keeps a loop that
// presents while waiting for input from spinning a core.

#include "engine/window/display.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <thread>
#include <utility>

#include "engine/base/array.h"
#include "engine/base/numeric.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/ww_win.h"

namespace {

// The game's resolution is scaled up by this much to get the window size.
constexpr int kWindowScale = 3;

// How long drawing may sit in the surface before the redraw timer presents it
// anyway. A thirtieth of a second, the rate the original game ran its frames
// at.
constexpr Uint32 kRedrawDelayMs = 1000 / 30;

// Runs on an SDL timer thread, so all it may do is post the event; the frame
// is ended on the main thread by the event loop that picks the event up.
// Returning 0 does not re-arm the timer - one pending redraw at a time is
// enough, and the present that answers it cancels the timer.
Uint32 PostRedraw(Uint32 /*interval*/, void* /*param*/) {
  TheDisplay().PostRedrawEvent();
  return 0;
}

}  // namespace

Display::Display() = default;

Display::Display(void* window, void* renderer)
    : window_(window), renderer_(renderer) {}

Display::~Display() {
  // Before the renderer goes: the textures belong to it.
  ResetVideoMode();
  if (!owns_window_) {
    return;
  }
  if (renderer_ != nullptr) {
    SDL_DestroyRenderer(static_cast<SDL_Renderer*>(renderer_));
  }
  if (window_ != nullptr) {
    SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
  }
}

bool Display::Init(const char* title, int width, int height) {
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

  // Created hidden and shown once the renderer exists: SDL's OpenGL renderer
  // destroys and recreates a window whose GL attributes don't match its own,
  // so a window shown here would flash up and be replaced by a second one.
  constexpr Uint32 kWindowFlags = Uint32{SDL_WINDOW_RESIZABLE} |
                                  Uint32{SDL_WINDOW_ALLOW_HIGHDPI} |
                                  Uint32{SDL_WINDOW_HIDDEN};
  SDL_Window* window = SDL_CreateWindow(
      title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      width * kWindowScale, height * kWindowScale, kWindowFlags);
  if (window == nullptr) {
    return false;
  }

  SDL_Renderer* renderer =
      SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    return false;
  }

  window_ = window;
  renderer_ = renderer;
  owns_window_ = true;
  redraw_event_ = SDL_RegisterEvents(1);

  // Keep the logical size at the game's resolution and let SDL scale to the
  // window.
  SDL_RenderSetLogicalSize(renderer, width, height);
  SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
  SDL_ShowWindow(window);

  // Sometimes the window is not created until it has content, so we get stuck
  // waiting for a focus it can never get because it does not exist.
  SDL_RenderClear(renderer);
  SDL_RenderPresent(renderer);
  return true;
}

bool Display::SetVideoMode(const int width, const int height) {
  ResetVideoMode();
  SDL_Texture* texture = SDL_CreateTexture(
      static_cast<SDL_Renderer*>(renderer_), SDL_PIXELFORMAT_RGB888,
      SDL_TEXTUREACCESS_STREAMING, width, height);
  SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 8, 0, 0, 0, 0);
  if (texture == nullptr || surface == nullptr) {
    if (texture != nullptr) {
      SDL_DestroyTexture(texture);
    }
    if (surface != nullptr) {
      SDL_FreeSurface(surface);
    }
    return false;
  }
  window_texture_ = texture;
  palette_surface_ = surface;
  return true;
}

void Display::ResetVideoMode() {
  CancelRedrawTimer();
  DropScaledFrame();
  if (window_texture_ != nullptr) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(window_texture_));
    window_texture_ = nullptr;
  }
  if (palette_surface_ != nullptr) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
    palette_surface_ = nullptr;
  }
}

std::optional<PixelSurface::Pixels> Display::Lock() {
  auto* surface = static_cast<SDL_Surface*>(palette_surface_);
  if (surface == nullptr || SDL_LockSurface(surface) != 0) {
    return std::nullopt;
  }
  // SDL_LockSurface exposes pitch bytes for each of the surface's rows until
  // it is unlocked.
  const auto bytes =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(static_cast<uint8_t*>(surface->pixels),
                base::ToSize(surface->pitch) * base::ToSize(surface->h));
  return Pixels{.bytes = bytes, .pitch = surface->pitch};
}

void Display::Unlock() {
  if (palette_surface_ == nullptr) {
    return;
  }
  SDL_UnlockSurface(static_cast<SDL_Surface*>(palette_surface_));
  // The game drew on the surface, so it is what the window shows from now on.
  DropScaledFrame();
  Present(false);
}

void Display::EndFrame() {
  if (palette_surface_ != nullptr) {
    Present(true);
  }
}

void Display::Present(const bool end_frame) {
  auto* renderer = static_cast<SDL_Renderer*>(renderer_);
  // A scaled frame stays up until the game draws again (the mission map
  // select keeps the movie's last frame on screen indefinitely), and only an
  // end of frame presents it.
  if (scaled_frame_texture_ != nullptr) {
    CancelRedrawTimer();

    if (!end_frame) {
      return;
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(scaled_frame_texture_),
                   nullptr, nullptr);
    PresentFrame();
    SDL_Event_Loop();
    return;
  }

  // Nothing asked for the frame to end, so leave the drawing in the surface
  // and let the redraw timer present it.
  if (!end_frame) {
    ArmRedrawTimer();
    return;
  }

  CancelRedrawTimer();

  // Convert the paletted surface to the texture's format through its palette.
  auto* window_texture = static_cast<SDL_Texture*>(window_texture_);
  SDL_Surface* texture_surface = nullptr;
  SDL_LockTextureToSurface(window_texture, nullptr, &texture_surface);
  SDL_BlitSurface(static_cast<SDL_Surface*>(palette_surface_), nullptr,
                  texture_surface, nullptr);
  SDL_UnlockTexture(window_texture);

  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, window_texture, nullptr, nullptr);
  PresentFrame();

  // The game presents from its loops that wait for input, so pumping events
  // here keeps them answering.
  SDL_Event_Loop();
}

void Display::UpdatePalette(const std::span<const uint8_t> palette) {
  if (palette_surface_ == nullptr) {
    return;
  }
  SDL_Palette* sdl_palette =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;
  if (palette.size() / 3 < base::ToSize(sdl_palette->ncolors)) {
    return;
  }
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_palette->colors, base::ToSize(sdl_palette->ncolors));

  bool changed = false;

  for (int i = 0; i < sdl_palette->ncolors; i++) {
    // Scale the 6-bit VGA guns to 8 bits, copying the top bits into the
    // bottom ones so that 63 becomes 255.
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

  // The colors were written in place; setting them again is what makes SDL
  // notice the change.
  SDL_SetPaletteColors(sdl_palette, sdl_palette->colors, 0,
                       sdl_palette->ncolors);

  // A scaled frame holds baked colors; the next end of frame presents it.
  if (scaled_frame_texture_ != nullptr) {
    UploadScaledFrame();
  }

  Present(false);
}

const void* Display::palette() const {
  if (palette_surface_ == nullptr) {
    return nullptr;
  }
  return static_cast<SDL_Surface*>(palette_surface_)->format->palette;
}

void Display::PresentScaledFrame(const std::span<const uint8_t> frame,
                                 const int width, const int height) {
  if (width <= 0 || height <= 0) {
    return;
  }
  const auto frame_width = base::ToSize(width);
  const auto frame_height = base::ToSize(height);
  if (frame_width > frame.size() / frame_height ||
      palette_surface_ == nullptr) {
    return;
  }
  CancelRedrawTimer();

  // The texture is made on the first frame and again whenever the size
  // changes.
  if (scaled_frame_texture_ == nullptr || scaled_frame_width_ != width ||
      scaled_frame_height_ != height) {
    if (scaled_frame_texture_ != nullptr) {
      SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    }
    scaled_frame_texture_ = SDL_CreateTexture(
        static_cast<SDL_Renderer*>(renderer_), SDL_PIXELFORMAT_RGBA32,
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

  Present(true);
}

bool Display::UploadScaledFrame() {
  const auto frame_width = base::ToSize(scaled_frame_width_);
  const std::span<const uint8_t> frame = scaled_frame_;

  // The surface's palette is already in 8-bit guns.
  const auto* sdl_palette =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;

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
      std::span(sdl_palette->colors, base::ToSize(sdl_palette->ncolors));
  for (int y = 0; y < scaled_frame_height_; y++) {
    for (int x = 0; x < scaled_frame_width_; x++) {
      const uint8_t idx =
          base::At(frame, (base::ToSize(y) * frame_width) + base::ToSize(x));
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

void Display::DropScaledFrame() {
  if (scaled_frame_texture_ != nullptr) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    scaled_frame_texture_ = nullptr;
    scaled_frame_width_ = 0;
    scaled_frame_height_ = 0;
    scaled_frame_.clear();
  }
}

void Display::PresentFrame() {
  // Shorter than a 60 Hz refresh, so on a vsync display, where the present
  // itself blocks for the refresh, the floor is never reached and cannot make
  // a frame miss its vblank. The renderer's vsync flag cannot tell the two
  // cases apart: the software renderer sets it without waiting.
  constexpr std::chrono::microseconds kMinInterval(1'000'000 / 70);
  const auto now = std::chrono::steady_clock::now();
  if (now < next_present_) {
    std::this_thread::sleep_until(next_present_);
    next_present_ += kMinInterval;
  } else {
    // Late, or the first present: restart the cadence from now rather than
    // presenting a burst of frames to catch up.
    next_present_ = now + kMinInterval;
  }
  SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

// Pushes onto SDL's event queue, state that lives outside this object.
// NOLINTNEXTLINE(readability-make-member-function-const)
void Display::PostRedrawEvent() {
  SDL_Event event;
  event.type = redraw_event_;
  SDL_PushEvent(&event);
}

bool Display::IsRedrawEvent(uint32_t event_type) const {
  return event_type == redraw_event_;
}

void Display::ArmRedrawTimer() {
  if (redraw_timer_ == 0) {
    redraw_timer_ = SDL_AddTimer(kRedrawDelayMs, PostRedraw, nullptr);
  }
}

void Display::CancelRedrawTimer() {
  if (redraw_timer_ != 0) {
    SDL_RemoveTimer(redraw_timer_);
    redraw_timer_ = 0;
  }
}

void Display::Restore() {
  auto* window = static_cast<SDL_Window*>(window_);
  SDL_RestoreWindow(window);
  SDL_RaiseWindow(window);
}

bool Display::HasInputFocus() const {
  return (SDL_GetWindowFlags(static_cast<SDL_Window*>(window_)) &
          SDL_WINDOW_INPUT_FOCUS) != 0;
}

int Display::DisplayIndex() const {
  if (window_ == nullptr) {
    return 0;
  }
  const int index =
      SDL_GetWindowDisplayIndex(static_cast<SDL_Window*>(window_));
  return index >= 0 ? index : 0;
}

void Display::SetMouseGrab(bool grab) {
  SDL_SetWindowGrab(static_cast<SDL_Window*>(window_),
                    grab ? SDL_TRUE : SDL_FALSE);
}
