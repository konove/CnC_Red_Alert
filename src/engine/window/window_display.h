// File: WindowDisplay, the Display that is the program's window, and
// ScriptedWindowDisplay, the same window when a script plays the game.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_DISPLAY_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_DISPLAY_H_

#include <chrono>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/gfx/ico_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display.h"
#include "engine/window/display_client.h"

namespace engine::window {

// Owns the one window the program opens, the SDL renderer that presents it,
// and the 8-bit paletted surface the game draws the window's contents on.
//
// It also runs the SDL event loop: PumpEvents() presents the frames the
// redraw timer asks for and hands every other event to the game's
// DisplayClient.
//
// Constructing one opens nothing: Init() creates the window once the
// configured video mode is known, and SetVideoMode() the surface. A test
// builds one and either leaves it closed or adopts a window and renderer it
// made itself.
//
// It keeps time in two places, which ScriptedWindowDisplay replaces: a timer
// presents drawing that nothing ended the frame for, and PresentFrame() holds
// the frame rate to 70 a second.
class WindowDisplay : public Display {
 public:
  // A WindowDisplay whose event loop drops the game's events. For tests.
  WindowDisplay();
  // A WindowDisplay whose event loop hands the game's events to `client`.
  explicit WindowDisplay(DisplayClient& client);
  // Adopts an SDL_Window and SDL_Renderer the caller made and keeps owning,
  // and drops the game's events. For tests that need a renderer without a
  // game window.
  WindowDisplay(void* window ABSL_ATTRIBUTE_LIFETIME_BOUND,
                void* renderer ABSL_ATTRIBUTE_LIFETIME_BOUND);
  ~WindowDisplay() override;

  WindowDisplay(const WindowDisplay&) = delete;
  WindowDisplay& operator=(const WindowDisplay&) = delete;
  WindowDisplay(WindowDisplay&&) = delete;
  WindowDisplay& operator=(WindowDisplay&&) = delete;

  // Opens the window and its renderer and registers the event the redraw
  // timer posts. Returns false if SDL could not create either.
  bool Init(const char* title, int width, int height) override;
  // Taken by value because SDL wants the pixels writable, though it only
  // copies them.
  void SetIcon(engine::gfx::IcoImage icon) override;

  // Also creates the texture the surface is presented through. Needs the
  // renderer, from Init() or the adopting constructor.
  bool SetVideoMode(int width, int height) override;
  void ResetVideoMode() override;

  // Locks the paletted surface for a page to draw on. Fails before
  // SetVideoMode() or if SDL cannot lock it.
  std::optional<Pixels> Lock() override;
  // Unlocks the surface, drops a movie frame so that what was drawn shows
  // instead (a backdrop stays under it), and schedules a redraw to present
  // it.
  void Unlock() override;

  [[nodiscard]] void* renderer() const override { return renderer_; }

  void SetPalette(const engine::gfx::Palette& palette) override;
  [[nodiscard]] const void* palette() const override;

  void EndFrame() override;

  void PresentScaledFrame(std::span<const uint8_t> frame, int width,
                          int height) override;
  void DropScaledFrame() override;
  void PresentBackdrop(std::span<const uint8_t> frame, int width,
                       int height) override;
  void ClearBackdrop() override;

  [[nodiscard]] DisplayPicture Capture() const override;

  // Posts the event that makes the event loop end the frame, so that drawing
  // which scheduled a redraw is presented even if nothing else draws. Called
  // from an SDL timer thread.
  void PostRedrawEvent();

  // Whether the redraw timer is armed.
  [[nodiscard]] bool redraw_pending() const override;

  // Drains SDL's event queue: runs the client's Pump(), then ends the frame
  // on each redraw event and passes every other event to the client's
  // HandleEvent(). Works before Init(), with no window to post events.
  //
  // Ending the frame presents, and presenting pumps again, so a redraw event
  // runs a nested loop that drains the rest of the queue.
  void PumpEvents() override;

  void Restore() override;
  [[nodiscard]] bool HasInputFocus() const override;

  // SDL draws the picture at a whole number of screen pixels per game pixel,
  // the largest that fits the window; on a high-DPI display a window unit
  // spans several screen pixels, so the result can be fractional.
  [[nodiscard]] float PictureScale() const override;
  void SetMouseGrab(bool grab) override;

 protected:
  // Arranges for drawing left in the surface to be presented if nothing ends
  // the frame first: arms a one-shot timer that posts the redraw event a
  // thirtieth of a second from now, unless one is already pending. The game
  // draws in bursts between waits for input; without this the last burst
  // before a wait would not reach the window until the wait ended.
  virtual void ScheduleRedraw();
  // Takes back a scheduled redraw. Called by whatever presents the window
  // first, and when the video mode goes.
  virtual void CancelRedraw();
  // Called by PumpEvents() between pumping the client and polling SDL. Does
  // nothing: the timer's event does the presenting.
  virtual void PresentScheduledRedraw();
  // Called before every present. Holds the frame rate to 70 a second: with
  // working vsync the present itself waits for the display refresh and this
  // never sleeps. Without it (SDL's software renderer, used by the dummy
  // video driver, reports vsync but does not wait) it sleeps out the rest of
  // the interval, so the many loops that present while waiting for input do
  // not spin a core.
  virtual void WaitForPresentSlot();

 private:
  // Tells the observer a frame has ended and pumps events. Every path that
  // ends a frame finishes here.
  void FrameEnded();

  // Presents what the window should show. With `end_frame` false it only
  // schedules a redraw, so that a burst of drawing is presented once.
  void Present(bool end_frame);

  // Presents the renderer, after WaitForPresentSlot().
  void PresentFrame();

  // What ResetVideoMode() and DropScaledFrame() do, apart from cancelling a
  // redraw, as plain functions the destructor can call.
  void FreeVideoMode();
  void FreeScaledFrame();

  // Stores `frame` as the scaled frame, a backdrop if `backdrop`, and
  // presents it; PresentScaledFrame() and PresentBackdrop() differ only in
  // `backdrop`.
  void ShowScaledFrame(std::span<const uint8_t> frame, int width, int height,
                       bool backdrop);

  // Converts the paletted surface to window_texture_ in the current palette.
  // With `see_through` set, color 0 becomes transparent, for drawing the
  // surface over a backdrop; otherwise every pixel is opaque.
  void UploadSurface(bool see_through);

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
  // SDL_Texture*, the surface converted to ARGB for presenting, with alpha
  // blending so that it can be drawn over a backdrop; null exactly when
  // palette_surface_ is.
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
  // Whether the scaled frame is a backdrop, shown under the surface, rather
  // than a movie frame shown instead of it. False while there is none.
  bool backdrop_ = false;
  // The SDL event type PostRedrawEvent() posts, 0 before Init().
  uint32_t redraw_event_ = 0;
  // SDL_TimerID of the pending redraw, 0 when none is armed.
  int redraw_timer_ = 0;
  // The earliest time the next present may happen.
  std::chrono::steady_clock::time_point next_present_;
};

// The window of a game that a script is playing (the -SCRIPT switch). It
// shows the same picture, but keeps no time of its own, so that a run does
// the same thing every time and as fast as the machine allows: drawing left
// in the surface is presented by the next PumpEvents() instead of by a timer
// on another thread, and a present waits for nothing.
class ScriptedWindowDisplay final : public WindowDisplay {
 public:
  // As WindowDisplay's: for the game, and adopting a renderer for tests.
  explicit ScriptedWindowDisplay(DisplayClient& client);
  ScriptedWindowDisplay(void* window, void* renderer);

  [[nodiscard]] bool redraw_pending() const override;

 protected:
  void ScheduleRedraw() override;
  void CancelRedraw() override;
  void PresentScheduledRedraw() override;
  void WaitForPresentSlot() override;

 private:
  // Whether the next PumpEvents() presents the surface.
  bool redraw_on_pump_ = false;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_WINDOW_DISPLAY_H_
