// File: Display, the program's window and what is shown in it.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/base/installed.h"
#include "engine/gfx/pixel_surface.h"

// Owns the one window the program opens, the renderer that presents it, and
// the 8-bit paletted surface the game draws the window's contents on. Each
// game's Game owns the one Display, declared before its Screen, and
// everything else reaches it through TheDisplay().
//
// The Display is itself the PixelSurface behind the window page: the page
// borrows the surface's pixels while it draws, and giving them back presents
// them, soon if not at once. A 320x200 movie bypasses the surface:
// PresentScaledFrame() shows its frames stretched to the window until the
// game draws again.
//
// Constructing a Display opens nothing: Init() creates the window once the
// configured video mode is known, and SetVideoMode() the surface. A test
// builds one and either leaves it closed or adopts a window and renderer it
// made itself.
//
// Example:
//   Display& display = TheDisplay();
//   display.SetVideoMode(640, 400);
//   page.Init(640, 400, display);
//   display.SetPalette(palette);
//   display.EndFrame();
class Display : public PixelSurface {
 public:
  Display();
  // Adopts an SDL_Window and SDL_Renderer the caller made and keeps owning.
  // For tests that need a renderer without a game window.
  Display(void* window ABSL_ATTRIBUTE_LIFETIME_BOUND,
          void* renderer ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~Display() override;

  Display(const Display&) = delete;
  Display& operator=(const Display&) = delete;
  Display(Display&&) = delete;
  Display& operator=(Display&&) = delete;

  // Opens the window and its renderer, scaled up from the game's resolution,
  // and registers the event the redraw timer posts. Returns false if SDL
  // could not create either.
  bool Init(const char* title, int width, int height);

  // Creates the width x height paletted surface the game draws on and the
  // texture it is presented through, replacing those of an earlier mode.
  // Needs the renderer, from Init() or the adopting constructor. Returns
  // false, with no surface, if SDL refused either.
  bool SetVideoMode(int width, int height);
  // Destroys the surface and textures and cancels a pending redraw, leaving
  // the Display as it was before SetVideoMode(). The destructor calls it;
  // calling it again does nothing.
  void ResetVideoMode();

  // Locks the paletted surface for a page to draw on. Fails before
  // SetVideoMode() or if SDL cannot lock it.
  std::optional<Pixels> Lock() override;
  // Unlocks the surface, drops a scaled frame so that what was drawn shows
  // instead, and arms the redraw timer to present it.
  void Unlock() override;

  // The SDL_Renderer, as a void* so that callers need no SDL header. For
  // engine_window's own drawing code; nullptr before Init().
  [[nodiscard]] void* renderer() const { return renderer_; }

  // Sets the 256 RGB triples the paletted pixels are shown through and
  // rebuilds the mouse cursor, which bakes its colors in. The palette part
  // does nothing before SetVideoMode().
  void SetPalette(std::span<const uint8_t> palette);
  // The palette part of SetPalette(), without the cursor: sets the 256 6-bit
  // VGA RGB triples in `palette` and redraws with them. Anything already
  // presented changes color, the way a VGA palette write did. Does nothing
  // before SetVideoMode() or if `palette` holds fewer than 256 triples.
  void UpdatePalette(std::span<const uint8_t> palette);
  // The SDL_Palette of the paletted surface, as a void* so that callers need
  // no SDL header; nullptr before SetVideoMode().
  [[nodiscard]] const void* palette() const;

  // Presents the surface, or the scaled frame showing instead of it,
  // immediately, ending the frame. Does nothing before SetVideoMode().
  void EndFrame();

  // Presents `frame`, `width` x `height` pixels, stretched to the window by
  // SDL rather than by the game - this is how a 320x200 movie fills a 640x400
  // screen without the game scaling every frame itself. Uses the palette
  // already set. The frame stays on screen, following later palette changes
  // the way a VGA screen would, until something is drawn to the surface.
  // Does nothing before SetVideoMode(), or if `frame` holds fewer than
  // width * height pixels.
  void PresentScaledFrame(std::span<const uint8_t> frame, int width,
                          int height);
  // Drops the scaled frame, so the next present shows the surface again.
  // Unlock() calls it as soon as anything draws.
  void DropScaledFrame();

  // Presents the renderer, at most 70 times a second. With working vsync the
  // present itself waits for the display refresh and this never sleeps.
  // Without it (SDL's software renderer, used by the dummy video driver,
  // reports vsync but does not wait) this sleeps out the rest of the
  // interval, so the many loops that present while waiting for input do not
  // spin a core.
  void PresentFrame();

  // Posts the event that makes the event loop end the frame, so that drawing
  // which armed the redraw timer is presented even if nothing else draws.
  // Called from an SDL timer thread.
  void PostRedrawEvent();
  // Whether an event the loop pulled is that redraw event.
  [[nodiscard]] bool IsRedrawEvent(uint32_t event_type) const;

  // Whether the redraw timer is armed, that is whether drawing is waiting in
  // the surface to be presented.
  [[nodiscard]] bool redraw_pending() const { return redraw_timer_ != 0; }

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
  // Presents what the window should show. With `end_frame` false it only arms
  // the redraw timer, so that a burst of drawing is presented once, a
  // thirtieth of a second after it started, if nothing ends the frame first.
  void Present(bool end_frame);

  // Arms a one-shot timer that posts the redraw event a thirtieth of a second
  // from now, and does nothing if one is already pending. The game draws in
  // bursts between waits for input; without this the last burst before a wait
  // would not reach the window until the wait ended.
  void ArmRedrawTimer();
  // Cancels a pending redraw timer. Called by whatever presents the window
  // first, and by ~Display so that no callback outlives the Display it posts
  // its event to.
  void CancelRedrawTimer();

  // Converts scaled_frame_ to RGBA with the current palette and uploads it to
  // scaled_frame_texture_. Returns false if SDL refused the texture.
  bool UploadScaledFrame();

  // SDL types, held as void* so that this header pulls in no SDL headers.
  void* window_ = nullptr;    // SDL_Window*, the program's one window
  void* renderer_ = nullptr;  // SDL_Renderer*, what presents it
  // Whether this Display created window_ and renderer_ and must destroy them.
  bool owns_window_ = false;
  // SDL_Surface*, the 8-bit pixels the game draws on; null before
  // SetVideoMode().
  void* palette_surface_ = nullptr;
  // SDL_Texture*, the surface converted to the renderer's format for
  // presenting; null exactly when palette_surface_ is.
  void* window_texture_ = nullptr;
  // SDL_Texture* holding the scaled frame, stretched to the window when
  // presented; null while the surface is what the window shows.
  void* scaled_frame_texture_ = nullptr;
  int scaled_frame_width_ = 0;
  int scaled_frame_height_ = 0;
  // The paletted pixels behind scaled_frame_texture_, scaled_frame_width_ x
  // scaled_frame_height_. The texture holds baked colors, so a palette change
  // has to convert these again. Empty while there is no texture.
  std::vector<uint8_t> scaled_frame_;
  // The SDL event type PostRedrawEvent() posts, 0 before Init().
  uint32_t redraw_event_ = 0;
  // SDL_TimerID of the pending redraw, 0 when none is armed.
  int redraw_timer_ = 0;
  // The earliest time the next present may happen.
  std::chrono::steady_clock::time_point next_present_;
};

// Returns the Display that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Display& TheDisplay() { return base::Installed<Display>::Get(); }

// Whether a Display is installed. Code that also runs before the window
// exists, or after it is gone, asks this first.
inline bool HasDisplay() { return base::Installed<Display>::IsInstalled(); }

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_DISPLAY_H_
