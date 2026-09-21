// File: Screen, Tiberian Dawn's video pages and the views the game draws
// through.

#ifndef CNC_RED_ALERT_TD_SCREEN_H_
#define CNC_RED_ALERT_TD_SCREEN_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "sdllib/graphic_buffer.h"

// Owns the full-screen pages, the 640x400 game-area views into them and the
// staging page movies decode into. Game owns the one Screen; everything
// else reaches it through TheScreen().
//
// Constructing a Screen needs no window: the pages stay empty until Init()
// sets the video mode and sizes them. A test can build one and attach the
// views to its own buffers.
//
// Example:
//   Screen& screen = TheScreen();
//   screen.hidden_view().Blit(screen.visible_view());
class Screen {
 public:
  // The game area, whatever the video mode: 640x400.
  static constexpr int kWidth = 640;
  static constexpr int kHeight = 400;

  Screen();
  ~Screen() = default;

  Screen(const Screen&) = delete;
  Screen& operator=(const Screen&) = delete;
  Screen(Screen&&) = delete;
  Screen& operator=(Screen&&) = delete;

  // Sets a kWidth x mode_height() video mode, falling back from 400 to 480
  // lines, sizes visible_page() and hidden_page() to it, and attaches the
  // views to the game area, 40 lines down in a 480-line mode. Needs the main
  // window. Returns false if no mode could be set.
  bool Init();

  // The page on screen and the back buffer the game draws into before
  // blitting. Both are the size of the whole video mode.
  GraphicBufferClass& visible_page() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return visible_page_;
  }
  GraphicBufferClass& hidden_page() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return hidden_page_;
  }

  // The kWidth x kHeight game area within each page.
  GraphicViewPortClass& visible_view() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return visible_view_;
  }
  GraphicViewPortClass& hidden_view() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return hidden_view_;
  }

  // The 320x200 page movies decode into.
  GraphicBufferClass& sys_mem_page() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return sys_mem_page_;
  }

  // The number of lines of the video mode: 400, or 480 when the command line
  // or the config file asks for it or a 400-line mode is not available.
  [[nodiscard]] int mode_height() const { return mode_height_; }
  void set_mode_height(int mode_height) { mode_height_ = mode_height; }

  // Returns whether `view` is visible_view(), the view drawing to the
  // screen directly.
  [[nodiscard]] bool IsVisible(const GraphicViewPortClass* view) const {
    return view == &visible_view_;
  }

 private:
  int mode_height_ = kHeight;

  // The pages come before the views attached to them, so that the views go
  // first on destruction.
  GraphicBufferClass visible_page_;
  GraphicBufferClass hidden_page_;
  GraphicViewPortClass visible_view_;
  GraphicViewPortClass hidden_view_;
  GraphicBufferClass sys_mem_page_;
};

// Returns the Screen that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline Screen& TheScreen() { return base::Installed<Screen>::Get(); }

#endif  // CNC_RED_ALERT_TD_SCREEN_H_
