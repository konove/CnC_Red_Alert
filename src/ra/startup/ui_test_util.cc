#include "ra/startup/ui_test_util.h"

#include "absl/log/check.h"
#include "engine/gfx/font_test_util.h"
#include "engine/window/display.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/assets.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/text_ids.h"

namespace ra {

UiFixture::UiFixture()
    : font_(MakeBlockFont(6, 10)),
      player_(engine::window::TheDisplay(), game::TheKeyboard(),
              game::TheMouse(), clock_) {
  CHECK(TheScreen().InitHeadless());
  // Dialogs copy their strings without asking whether there are any.
  SetText(TXT_NONE, "");
  for (const FontType type : magic_enum::enum_values<FontType>()) {
    TheAssets().set_font(type, font_);
  }
}

UiFixture::~UiFixture() = default;

}  // namespace ra
