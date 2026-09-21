// Implements Screen: setting the video mode and sizing the pages to it.

#include "ra/screen.h"

#include "sdllib/display.h"
#include "sdllib/misc.h"
#include "sdllib/pixel_buffer.h"

// The views start out covering a 640x480 page that does not exist yet, as the
// original globals did; Init() attaches them to the real game area.
Screen::Screen()
    : visible_view_(&visible_page_, 0, 0, kWidth, 480),
      hidden_view_(&hidden_page_, 0, 0, kWidth, 480),
      sys_mem_page_(kDefaultScreenWidth, kDefaultScreenHeight),
      vq640_(kWidth, kHeight) {}

bool Screen::Init() {
  bool mode_set = Set_Video_Mode(kWidth, mode_height_, 8);
  if (!mode_set && mode_height_ == kHeight) {
    mode_set = Set_Video_Mode(kWidth, 480, 8);
    if (mode_set) {
      mode_height_ = 480;
    }
  }
  if (!mode_set) {
    return false;
  }

  visible_page_.Init(kWidth, mode_height_, {}, 0, BUFFER_VISIBLE);
  TheDisplay().AttachWindowPage(visible_page_);
  hidden_page_.Init(kWidth, mode_height_, {}, 0, BUFFER_NONE);

  // A 480-line mode letterboxes the 400-line game area in the middle.
  const int letterbox_top = (mode_height_ - kHeight) / 2;
  visible_view_.Attach(&visible_page_, 0, letterbox_top, kWidth, kHeight);
  hidden_view_.Attach(&hidden_page_, 0, letterbox_top, kWidth, kHeight);
  return true;
}
