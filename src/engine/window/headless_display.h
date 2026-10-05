// File: HeadlessDisplay, a Display with no window behind it.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_HEADLESS_DISPLAY_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_HEADLESS_DISPLAY_H_

#include <cstdint>
#include <optional>
#include <span>

#include "engine/gfx/ico_image.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pixel_surface.h"
#include "engine/window/display.h"
#include "engine/window/display_client.h"

namespace engine::window {

// The paletted surface with nothing behind it: no window, no renderer and no
// SDL subsystem. The game draws on the surface and sets its palette as usual,
// and ending a frame presents nothing: it tells the observer, then pumps the
// client. No SDL event is polled, no timer runs and nothing waits for a
// display refresh, so a loop runs as fast as it computes. A scaled frame or
// backdrop is not kept; presenting one only ends the frame.
//
// For tests of the code that draws and waits for input, which install a
// ScriptedPlayer as the observer.
//
// Example:
//   HeadlessDisplay display;
//   display.SetVideoMode(640, 400);
//   page.Init(640, 400, display);
class HeadlessDisplay final : public Display {
 public:
  // A HeadlessDisplay that drops the game's events.
  HeadlessDisplay();
  // A HeadlessDisplay that pumps `client`.
  explicit HeadlessDisplay(DisplayClient& client);
  ~HeadlessDisplay() override;

  HeadlessDisplay(const HeadlessDisplay&) = delete;
  HeadlessDisplay& operator=(const HeadlessDisplay&) = delete;
  HeadlessDisplay(HeadlessDisplay&&) = delete;
  HeadlessDisplay& operator=(HeadlessDisplay&&) = delete;

  // There is no window to open: always true.
  bool Init(const char* title, int width, int height) override;
  void SetIcon(engine::gfx::IcoImage icon) override;

  bool SetVideoMode(int width, int height) override;
  void ResetVideoMode() override;

  std::optional<Pixels> Lock() override;
  void Unlock() override;

  [[nodiscard]] void* renderer() const override { return nullptr; }

  void SetPalette(const engine::gfx::Palette& palette) override;
  [[nodiscard]] const void* palette() const override;

  void EndFrame() override;

  // Only end the frame, if the frame is one PresentScaledFrame() accepts.
  void PresentScaledFrame(std::span<const uint8_t> frame, int width,
                          int height) override;
  void PresentBackdrop(std::span<const uint8_t> frame, int width,
                       int height) override;
  void DropScaledFrame() override {}
  void ClearBackdrop() override {}

  // Nothing ever waits: drawing is "presented" as it is drawn.
  [[nodiscard]] bool redraw_pending() const override { return false; }

  // Tells the observer and pumps the client; there are no events to hand it.
  void PumpEvents() override;

  void Restore() override {}
  [[nodiscard]] bool HasInputFocus() const override { return true; }
  [[nodiscard]] float PictureScale() const override { return 1.0F; }
  void SetMouseGrab(bool /*grab*/) override {}

 private:
  // SDL_Surface*, held as void* so that this header pulls in no SDL headers;
  // null before SetVideoMode(). A surface is plain memory to SDL, and its
  // palette is what palette() hands out.
  void* palette_surface_ = nullptr;
};

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_HEADLESS_DISPLAY_H_
