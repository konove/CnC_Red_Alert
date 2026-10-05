// Implements WindowDisplay: opening the window, the paletted surface the game
// draws on and the textures that present it, and the pacing that keeps a loop
// that presents while waiting for input from spinning a core.

#include "engine/window/window_display.h"

#include <SDL.h>
#include <SDL_blendmode.h>
#include <SDL_events.h>
#include <SDL_pixels.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_surface.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <thread>
#include <utility>

#include "base/array.h"
#include "base/numeric.h"
#include "engine/gfx/ico_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display.h"
#include "engine/window/display_client.h"

namespace engine::window {

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
// enough, and the present that answers it cancels the timer. `display` is the
// WindowDisplay that armed the timer, which cancels it before it goes.
Uint32 PostRedraw(Uint32 /*interval*/, void* display) {
  static_cast<WindowDisplay*>(display)->PostRedrawEvent();
  return 0;
}

}  // namespace

WindowDisplay::WindowDisplay() = default;

WindowDisplay::WindowDisplay(DisplayClient& client) : Display(client) {}

WindowDisplay::WindowDisplay(void* window, void* renderer)
    : window_(window), renderer_(renderer) {}

WindowDisplay::~WindowDisplay() {
  // Before the renderer goes: the textures belong to it. Not through the
  // virtual functions: whatever derived from this is already gone.
  WindowDisplay::CancelRedraw();
  FreeVideoMode();
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

bool WindowDisplay::Init(const char* title, int width, int height) {
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

void WindowDisplay::SetIcon(engine::gfx::IcoImage icon) {
  if (window_ == nullptr ||
      icon.rgba.size() != base::ToSize(icon.width * icon.height * 4)) {
    return;
  }
  // The surface borrows the pixels; SDL_SetWindowIcon copies them.
  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
      icon.rgba.data(), icon.width, icon.height, 32, icon.width * 4,
      SDL_PIXELFORMAT_RGBA32);
  if (surface == nullptr) {
    return;
  }
  SDL_SetWindowIcon(static_cast<SDL_Window*>(window_), surface);
  SDL_FreeSurface(surface);
}

bool WindowDisplay::SetVideoMode(const int width, const int height) {
  ResetVideoMode();
  SDL_Texture* texture = SDL_CreateTexture(
      static_cast<SDL_Renderer*>(renderer_), SDL_PIXELFORMAT_ARGB8888,
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
  // Opaque everywhere unless a backdrop is up, so blending only matters then.
  SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
  window_texture_ = texture;
  palette_surface_ = surface;
  return true;
}

void WindowDisplay::ResetVideoMode() {
  CancelRedraw();
  FreeVideoMode();
}

void WindowDisplay::FreeVideoMode() {
  FreeScaledFrame();
  if (window_texture_ != nullptr) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(window_texture_));
    window_texture_ = nullptr;
  }
  if (palette_surface_ != nullptr) {
    SDL_FreeSurface(static_cast<SDL_Surface*>(palette_surface_));
    palette_surface_ = nullptr;
  }
}

std::optional<PixelSurface::Pixels> WindowDisplay::Lock() {
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

void WindowDisplay::Unlock() {
  if (palette_surface_ == nullptr) {
    return;
  }
  SDL_UnlockSurface(static_cast<SDL_Surface*>(palette_surface_));
  // The game drew on the surface, so it is what the window shows from now on,
  // over a backdrop or instead of a movie frame.
  if (!backdrop_) {
    DropScaledFrame();
  }
  Present(false);
}

void WindowDisplay::EndFrame() {
  if (palette_surface_ != nullptr) {
    Present(true);
  }
}

void WindowDisplay::FrameEnded() {
  NotifyFrameEnd();
  // The game presents from its loops that wait for input, so pumping events
  // here keeps them answering.
  PumpEvents();
}

void WindowDisplay::Present(const bool end_frame) {
  auto* renderer = static_cast<SDL_Renderer*>(renderer_);
  // A movie frame stays up until the game draws again (the mission map
  // select keeps the movie's last frame on screen indefinitely), and only an
  // end of frame presents it.
  if (scaled_frame_texture_ != nullptr && !backdrop_) {
    CancelRedraw();

    if (!end_frame) {
      return;
    }

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(scaled_frame_texture_),
                   nullptr, nullptr);
    PresentFrame();
    FrameEnded();
    return;
  }

  // Nothing asked for the frame to end, so leave the drawing in the surface
  // and let the redraw timer present it.
  if (!end_frame) {
    ScheduleRedraw();
    return;
  }

  CancelRedraw();

  UploadSurface(backdrop_);
  SDL_RenderClear(renderer);
  if (backdrop_) {
    SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(scaled_frame_texture_),
                   nullptr, nullptr);
  }
  SDL_RenderCopy(renderer, static_cast<SDL_Texture*>(window_texture_), nullptr,
                 nullptr);
  PresentFrame();
  FrameEnded();
}

void WindowDisplay::UploadSurface(const bool see_through) {
  auto* surface = static_cast<SDL_Surface*>(palette_surface_);
  auto* window_texture = static_cast<SDL_Texture*>(window_texture_);
  SDL_Surface* texture_surface = nullptr;
  if (SDL_LockTextureToSurface(window_texture, nullptr, &texture_surface) !=
      0) {
    return;
  }
  // A keyed blit skips color 0, leaving the transparent fill under it. The
  // other pixels take their alpha from the palette, which SDL makes opaque.
  SDL_SetColorKey(surface, see_through ? SDL_TRUE : SDL_FALSE, 0);
  if (see_through) {
    SDL_FillRect(texture_surface, nullptr, 0);
  }
  SDL_BlitSurface(surface, nullptr, texture_surface, nullptr);
  SDL_UnlockTexture(window_texture);
}

void WindowDisplay::SetPalette(const engine::gfx::Palette& palette) {
  if (palette_surface_ == nullptr) {
    return;
  }
  SDL_Palette* sdl_palette =
      static_cast<SDL_Surface*>(palette_surface_)->format->palette;
  // SDL owns exactly ncolors entries in the surface palette.
  const auto colors =
      // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
      std::span(sdl_palette->colors, base::ToSize(sdl_palette->ncolors));

  bool changed = false;

  for (int i = 0;
       i < std::min(sdl_palette->ncolors, engine::gfx::Palette::kColorCount);
       i++) {
    const int new_r = palette.at(i).red();
    const int new_g = palette.at(i).green();
    const int new_b = palette.at(i).blue();
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

const void* WindowDisplay::palette() const {
  if (palette_surface_ == nullptr) {
    return nullptr;
  }
  return static_cast<SDL_Surface*>(palette_surface_)->format->palette;
}

void WindowDisplay::PresentScaledFrame(const std::span<const uint8_t> frame,
                                       const int width, const int height) {
  ShowScaledFrame(frame, width, height, /*backdrop=*/false);
}

void WindowDisplay::PresentBackdrop(const std::span<const uint8_t> frame,
                                    const int width, const int height) {
  ShowScaledFrame(frame, width, height, /*backdrop=*/true);
}

void WindowDisplay::ClearBackdrop() {
  if (backdrop_) {
    DropScaledFrame();
  }
}

void WindowDisplay::ShowScaledFrame(const std::span<const uint8_t> frame,
                                    const int width, const int height,
                                    const bool backdrop) {
  if (width <= 0 || height <= 0) {
    return;
  }
  const auto frame_width = base::ToSize(width);
  const auto frame_height = base::ToSize(height);
  if (frame_width > frame.size() / frame_height ||
      palette_surface_ == nullptr) {
    return;
  }
  CancelRedraw();

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
  backdrop_ = backdrop;
  if (!UploadScaledFrame()) {
    return;
  }

  Present(true);
}

bool WindowDisplay::UploadScaledFrame() {
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

void WindowDisplay::DropScaledFrame() { FreeScaledFrame(); }

void WindowDisplay::FreeScaledFrame() {
  if (scaled_frame_texture_ != nullptr) {
    SDL_DestroyTexture(static_cast<SDL_Texture*>(scaled_frame_texture_));
    scaled_frame_texture_ = nullptr;
    scaled_frame_width_ = 0;
    scaled_frame_height_ = 0;
    scaled_frame_.clear();
  }
  backdrop_ = false;
}

void WindowDisplay::PresentFrame() {
  WaitForPresentSlot();
  SDL_RenderPresent(static_cast<SDL_Renderer*>(renderer_));
}

void WindowDisplay::WaitForPresentSlot() {
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
}

// Pushes onto SDL's event queue, state that lives outside this object.
// NOLINTNEXTLINE(readability-make-member-function-const)
void WindowDisplay::PostRedrawEvent() {
  SDL_Event event;
  event.type = redraw_event_;
  SDL_PushEvent(&event);
}

void WindowDisplay::PumpEvents() {
  NotifyPump();
  client().Pump();
  PresentScheduledRedraw();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    // PostRedraw() posts this from its timer thread when drawing has waited in
    // the surface long enough; the game never sees it.
    if (redraw_event_ != 0 && event.type == redraw_event_) {
      EndFrame();
      continue;
    }
    client().HandleEvent(event);
  }
}

void WindowDisplay::ScheduleRedraw() {
  if (redraw_timer_ == 0) {
    redraw_timer_ = SDL_AddTimer(kRedrawDelayMs, PostRedraw, this);
  }
}

void WindowDisplay::CancelRedraw() {
  if (redraw_timer_ != 0) {
    SDL_RemoveTimer(redraw_timer_);
    redraw_timer_ = 0;
  }
}

bool WindowDisplay::redraw_pending() const { return redraw_timer_ != 0; }

void WindowDisplay::PresentScheduledRedraw() {}

void WindowDisplay::Restore() {
  auto* window = static_cast<SDL_Window*>(window_);
  SDL_RestoreWindow(window);
  SDL_RaiseWindow(window);
}

bool WindowDisplay::HasInputFocus() const {
  return (SDL_GetWindowFlags(static_cast<SDL_Window*>(window_)) &
          SDL_WINDOW_INPUT_FOCUS) != 0;
}

// SDL_RenderGetScale() is in the renderer's output pixels, which is what the
// window's size is in too unless the display is high-DPI; the ratio of the two
// widths converts it. A Display adopted without a window has no window units,
// so its output pixels stand in for them.
float WindowDisplay::PictureScale() const {
  auto* renderer = static_cast<SDL_Renderer*>(renderer_);
  if (renderer == nullptr) {
    return 1.0F;
  }
  float scale = 1.0F;
  SDL_RenderGetScale(renderer, &scale, nullptr);
  int output_width = 0;
  if (window_ == nullptr ||
      SDL_GetRendererOutputSize(renderer, &output_width, nullptr) != 0 ||
      output_width <= 0) {
    return scale;
  }
  int window_width = 0;
  SDL_GetWindowSize(static_cast<SDL_Window*>(window_), &window_width, nullptr);
  return scale * static_cast<float>(window_width) /
         static_cast<float>(output_width);
}

void WindowDisplay::SetMouseGrab(bool grab) {
  SDL_SetWindowGrab(static_cast<SDL_Window*>(window_),
                    grab ? SDL_TRUE : SDL_FALSE);
}

ScriptedWindowDisplay::ScriptedWindowDisplay(DisplayClient& client)
    : WindowDisplay(client) {}

ScriptedWindowDisplay::ScriptedWindowDisplay(void* window, void* renderer)
    : WindowDisplay(window, renderer) {}

void ScriptedWindowDisplay::ScheduleRedraw() { redraw_on_pump_ = true; }

void ScriptedWindowDisplay::CancelRedraw() { redraw_on_pump_ = false; }

bool ScriptedWindowDisplay::redraw_pending() const { return redraw_on_pump_; }

void ScriptedWindowDisplay::PresentScheduledRedraw() {
  if (redraw_on_pump_) {
    EndFrame();
  }
}

void ScriptedWindowDisplay::WaitForPresentSlot() {}

}  // namespace engine::window
