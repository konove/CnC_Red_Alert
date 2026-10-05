#include "ra/startup/ui_test_util.h"

#include <algorithm>
#include <cstdint>
#include <numeric>

#include "absl/log/check.h"
#include "engine/gfx/font_test_util.h"
#include "engine/window/display.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/assets/assets.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/text_ids.h"

namespace ra {

UiFixture::UiFixture()
    : font_(MakeBlockFont(6, 10)),
      player_(engine::window::TheDisplay(), game::TheKeyboard(),
              game::TheMouse(), clock_) {
  CHECK(TheScreen().InitHeadless());
  // A remap straight out of the constructor draws everything in colour 0,
  // which is no colour at all on a cleared page.
  for (const PlayerColorType scheme :
       magic_enum::enum_values<PlayerColorType>()) {
    if (scheme == PCOLOR_NONE) {
      continue;
    }
    RemapControlType& remap = ThePalettes().color_remaps().at(scheme);
    const auto color = static_cast<uint8_t>(SchemeColor(scheme));
    remap.BrightColor = color;
    remap.Color = color;
    std::ranges::iota(remap.RemapTable, uint8_t{0});
    std::ranges::fill(remap.FontRemap, color);
    remap.Shadow = color;
    remap.Background = color;
    remap.Corners = color;
    remap.Highlight = color;
    remap.Box = color;
    remap.Bright = color;
    remap.Underline = color;
    remap.Bar = color;
  }
  // Dialogs copy their strings without asking whether there are any.
  SetText(TXT_NONE, "");
  for (const FontType type : magic_enum::enum_values<FontType>()) {
    TheAssets().set_font(type, font_);
  }
}

UiFixture::~UiFixture() = default;

}  // namespace ra
