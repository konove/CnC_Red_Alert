// Implements Display: opening the window, presenting it, and the pacing that
// keeps a loop that presents while waiting for input from spinning a core.

#include "sdllib/display.h"

#include <SDL.h>
#include <SDL_events.h>
#include <SDL_render.h>
#include <SDL_stdinc.h>
#include <SDL_timer.h>
#include <SDL_video.h>

#include <chrono>
#include <cstdint>
#include <thread>

#include "sdllib/pixel_buffer.h"

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
  CancelRedrawTimer();
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

void Display::AttachWindowPage(PixelBuffer& page) { window_page_ = &page; }

void Display::DetachWindowPage() { window_page_ = nullptr; }

void Display::EndFrame() {
  if (window_page_ != nullptr) {
    window_page_->Present(true);
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
