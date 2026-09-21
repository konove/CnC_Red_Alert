// File: Display, the program's window and what is shown in it.

#ifndef CNC_RED_ALERT_SDLLIB_DISPLAY_H_
#define CNC_RED_ALERT_SDLLIB_DISPLAY_H_

#include <chrono>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/installed.h"

class PixelBuffer;

// Owns the one window the program opens, the renderer that presents it, and
// the page whose pixels are the window's surface. Each game's Game owns the
// one Display, declared before its Screen, and everything else reaches it
// through TheDisplay().
//
// Constructing a Display opens nothing: Init() creates the window once the
// configured video mode is known. A test builds one and either leaves it
// closed or adopts a window and renderer it made itself.
//
// Example:
//   Display& display = TheDisplay();
//   display.SetPalette(palette);
//   display.EndFrame();
class Display {
 public:
  Display();
  // Adopts an SDL_Window and SDL_Renderer the caller made and keeps owning.
  // For tests that need a renderer without a game window.
  Display(void* window ABSL_ATTRIBUTE_LIFETIME_BOUND,
          void* renderer ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~Display();

  Display(const Display&) = delete;
  Display& operator=(const Display&) = delete;
  Display(Display&&) = delete;
  Display& operator=(Display&&) = delete;

  // Opens the window and its renderer, scaled up from the game's resolution,
  // and registers the event the redraw timer posts. Returns false if SDL
  // could not create either.
  bool Init(const char* title, int width, int height);

  // Records the page whose pixels are the window surface, the one palette
  // changes and presents act on. Screen::Init() attaches its visible page
  // once the video mode is set; ~PixelBuffer detaches a page that dies while
  // still attached, so nothing is left pointing at freed pixels.
  void AttachWindowPage(PixelBuffer& page);
  void DetachWindowPage();
  // The window's page, or nullptr before Screen::Init() attaches one and
  // after it is gone.
  [[nodiscard]] PixelBuffer* window_page() const { return window_page_; }

  // The SDL_Renderer, as a void* so that callers need no SDL header. For
  // sdllib's own drawing code; nullptr before Init().
  [[nodiscard]] void* renderer() const { return renderer_; }

  // Sets the 256 RGB triples the paletted pixels are shown through and
  // rebuilds the mouse cursor, which bakes its colors in. Does nothing before
  // a window page is attached.
  void SetPalette(std::span<const uint8_t> palette);

  // Presents the window page immediately, ending the frame.
  void EndFrame();

  // Presents the renderer, at most 70 times a second. With working vsync the
  // present itself waits for the display refresh and this never sleeps.
  // Without it (SDL's software renderer, used by the dummy video driver,
  // reports vsync but does not wait) this sleeps out the rest of the
  // interval, so the many loops that present while waiting for input do not
  // spin a core.
  void PresentFrame();

  // Posts the event that makes the event loop end the frame, so that a page
  // that armed a redraw timer is presented even if nothing else draws.
  // Called from an SDL timer thread.
  void PostRedrawEvent();
  // Whether an event the loop pulled is that redraw event.
  [[nodiscard]] bool IsRedrawEvent(uint32_t event_type) const;

  // Brings the window back from minimized and raises it to the front.
  void Restore();
  // Whether the window has the keyboard focus.
  [[nodiscard]] bool HasInputFocus() const;

  // The display the window is on, for sizing the mouse cursor. Returns 0
  // before Init() opens a window.
  [[nodiscard]] int DisplayIndex() const;
  // Confines the mouse to the window, or releases it.
  void SetMouseGrab(bool grab);

 private:
  // SDL types, held as void* so that this header pulls in no SDL headers.
  void* window_ = nullptr;    // SDL_Window*, the program's one window
  void* renderer_ = nullptr;  // SDL_Renderer*, what presents it
  // Whether this Display created window_ and renderer_ and must destroy them.
  bool owns_window_ = false;
  // The page whose pixels are the window surface. Not owned: Screen holds it.
  PixelBuffer* window_page_ = nullptr;
  // The SDL event type PostRedrawEvent() posts, 0 before Init().
  uint32_t redraw_event_ = 0;
  // The earliest time the next present may happen.
  std::chrono::steady_clock::time_point next_present_;
};

// Returns the Display that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Display& TheDisplay() { return base::Installed<Display>::Get(); }

// Whether a Display is installed. Code that also runs before the window
// exists, or after it is gone, asks this first.
inline bool HasDisplay() { return base::Installed<Display>::IsInstalled(); }

#endif  // CNC_RED_ALERT_SDLLIB_DISPLAY_H_
