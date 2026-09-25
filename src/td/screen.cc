// Implements Screen: setting the video mode and sizing the pages to it.

#include "td/screen.h"

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/display.h"

// The views start out covering a 640x480 page that does not exist yet, as the
// original globals did; Init() attaches them to the real game area.
Screen::Screen()
    : visible_view_(&visible_page_, 0, 0, kWidth, 480),
      hidden_view_(&hidden_page_, 0, 0, kWidth, 480),
      sys_mem_page_(kDefaultScreenWidth, kDefaultScreenHeight) {}

bool Screen::Init() {
  bool mode_set = TheDisplay().SetVideoMode(kWidth, mode_height_);
  if (!mode_set && mode_height_ == kHeight) {
    mode_set = TheDisplay().SetVideoMode(kWidth, 480);
    if (mode_set) {
      mode_height_ = 480;
    }
  }
  if (!mode_set) {
    return false;
  }

  visible_page_.Init(kWidth, mode_height_, TheDisplay());
  hidden_page_.Init(kWidth, mode_height_, {}, 0);

  // A 480-line mode letterboxes the 400-line game area in the middle.
  const int letterbox_top = (mode_height_ - kHeight) / 2;
  visible_view_.Attach(&visible_page_, 0, letterbox_top, kWidth, kHeight);
  hidden_view_.Attach(&hidden_page_, 0, letterbox_top, kWidth, kHeight);
  return true;
}
