// File: Display, where the game's picture goes and its events come from.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_

#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "engine/gfx/ico_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display_client.h"
#include "engine/window/display_observer.h"

namespace engine::window {

// What the game shows its picture on: an 8-bit paletted surface it draws the
// frame on, the palette that surface is seen through, and the event loop the
// game pumps from wherever it waits. Each game's Game owns the one Display,
// made before its Screen, and everything else reaches it through
// TheDisplay().
//
// The Display is itself the PixelSurface behind the window page: the page
// borrows the surface's pixels while it draws, and giving them back presents
// them, soon if not at once. A 320x200 movie bypasses the surface:
// PresentScaledFrame() shows its frames stretched to the window until the
// game draws again. PresentBackdrop() stretches 320x200 art the same way but
// keeps the surface on top, so captions drawn on it at full resolution sit
// over the art.
//
// There are three kinds, made by MakeDisplay():
//   WindowDisplay          the program's window (window_display.h)
//   ScriptedWindowDisplay  the same window for a scripted run, which keeps no
//                          time of its own
//   HeadlessDisplay        the surface alone, for tests (headless_display.h)
//
// Example:
//   Display& display = TheDisplay();
//   display.SetVideoMode(640, 400);
//   page.Init(640, 400, display);
//   display.SetPalette(palette);
//   display.EndFrame();
class Display : public PixelSurface {
 public:
  ~Display() override;

  Display(const Display&) = delete;
  Display& operator=(const Display&) = delete;
  Display(Display&&) = delete;
  Display& operator=(Display&&) = delete;

  // Opens the window and whatever presents it, scaled up from the game's
  // resolution. Returns false if that failed. A display without a window has
  // nothing to open and returns true.
  virtual bool Init(const char* title, int width, int height) = 0;

  // Sets the picture the window manager shows for the window, in its title
  // bar and task switcher. Windows takes it from the executable's icon
  // resource unless this replaces it; Linux and macOS have no such resource,
  // so without this the window has none. Does nothing before Init().
  virtual void SetIcon(engine::gfx::IcoImage icon) = 0;

  // Creates the width x height paletted surface the game draws on, replacing
  // that of an earlier mode. Returns false, with no surface, on failure.
  virtual bool SetVideoMode(int width, int height) = 0;
  // Destroys the surface and cancels a pending redraw, leaving the Display as
  // it was before SetVideoMode(). Calling it again does nothing.
  virtual void ResetVideoMode() = 0;

  // The SDL_Renderer, as a void* so that callers need no SDL header. For
  // engine_window's own drawing code; nullptr when there is none.
  [[nodiscard]] virtual void* renderer() const = 0;

  // Sets the palette the paletted pixels are shown through, and redraws with
  // it. Anything already presented changes color, the way a VGA palette write
  // did. Does nothing before SetVideoMode(). The mouse cursor bakes its colors
  // in, so the games recolor it after this.
  virtual void SetPalette(const engine::gfx::Palette& palette) = 0;
  // The SDL_Palette of the paletted surface, as a void* so that callers need
  // no SDL header; nullptr before SetVideoMode().
  [[nodiscard]] virtual const void* palette() const = 0;

  // Presents the surface, or the scaled frame showing instead of it,
  // immediately, ending the frame: the observer is told, then events are
  // pumped. Does nothing before SetVideoMode().
  virtual void EndFrame() = 0;

  // Attaches the observer told of every frame ended and every event pump
  // from now on, replacing the one before; nullptr detaches it. It must stay
  // alive while attached.
  void set_observer(DisplayObserver* observer) { observer_ = observer; }

  // Presents `frame`, `width` x `height` pixels, stretched to the window -
  // this is how a 320x200 movie fills a 640x400 screen without the game
  // scaling every frame itself. Uses the palette already set. The frame stays
  // on screen, following later palette changes the way a VGA screen would,
  // until something is drawn to the surface. Does nothing before
  // SetVideoMode(), or if `frame` holds fewer than width * height pixels.
  virtual void PresentScaledFrame(std::span<const uint8_t> frame, int width,
                                  int height) = 0;
  // Drops the scaled frame, so the next present shows the surface again.
  // Unlock() calls it as soon as anything draws.
  virtual void DropScaledFrame() = 0;

  // Presents `frame` stretched to the window like PresentScaledFrame(), but
  // as a backdrop: the surface is drawn over it with color 0 see-through, and
  // drawing on the surface keeps the backdrop and is presented over it. It
  // stays until ClearBackdrop(), DropScaledFrame() or a movie frame. Rejects
  // the frames PresentScaledFrame() does.
  virtual void PresentBackdrop(std::span<const uint8_t> frame, int width,
                               int height) = 0;
  // Drops the scaled frame if it is a backdrop, so the next present shows the
  // surface alone; a movie frame stays.
  virtual void ClearBackdrop() = 0;

  // Whether drawing is waiting in the surface to be presented.
  [[nodiscard]] virtual bool redraw_pending() const = 0;

  // Runs the client's Pump(), then hands it every event that is waiting.
  // Returns once there are none left; it never waits for an event.
  //
  // It is re-entrant. Ending a frame pumps, so that the game's
  // wait-for-input loops keep answering.
  virtual void PumpEvents() = 0;

  // Brings the window back from minimized and raises it to the front.
  virtual void Restore() = 0;
  // Whether the window has the keyboard focus. A display without a window
  // always has it.
  [[nodiscard]] virtual bool HasInputFocus() const = 0;

  // How many window units one pixel of the game's picture spans. The OS draws
  // the mouse cursor in window units, which is what this sizes. 1 without a
  // renderer.
  [[nodiscard]] virtual float PictureScale() const = 0;
  // Confines the mouse to the window, or releases it.
  virtual void SetMouseGrab(bool grab) = 0;

 protected:
  // A Display whose event loop drops the game's events.
  Display();
  // A Display whose event loop hands the game's events to `client`.
  explicit Display(DisplayClient& client ABSL_ATTRIBUTE_LIFETIME_BOUND);

  // Where PumpEvents() sends the game's events.
  [[nodiscard]] DisplayClient& client() const { return *client_; }

  // Tells the observer, if there is one, that a frame has ended or that
  // events are about to be pumped.
  void NotifyFrameEnd();
  void NotifyPump();

 private:
  DisplayClient* client_;                // never null
  DisplayObserver* observer_ = nullptr;  // nullptr when nobody watches
};

// The kinds of Display there are.
enum class DisplayKind {
  kWindow,          // the program's window
  kScriptedWindow,  // the window of a scripted run
  kHeadless,        // no window: the surface alone
};

// Makes a Display of `kind` that hands the game's events to `client`, which
// must outlive it.
std::unique_ptr<Display> MakeDisplay(DisplayKind kind, DisplayClient& client);

// Returns the Display that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Display& TheDisplay() { return base::Installed<Display>::Get(); }

// Whether a Display is installed. Code that also runs before the window
// exists, or after it is gone, asks this first.
inline bool HasDisplay() { return base::Installed<Display>::IsInstalled(); }

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_
