/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ra/screens/difficulty.h"

#include <array>
#include <cstddef>
#include <string>

#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/rules.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/startup/conquer.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/slider.h"
#include "ra/ui/textbtn.h"

namespace ra {

using enum engine::window::KeyNumber;

// The difficulty scale has five levels; the coarse slider offers only its two
// ends and its middle, so one of its stops is worth kCoarseLevelStep levels.
constexpr int kDifficultyLevels = 5;
constexpr int kCoarseDifficultyStops = 3;
constexpr int kCoarseLevelStep =
    (kDifficultyLevels - 1) / (kCoarseDifficultyStops - 1);

DifficultyChoice DifficultyForLevel(const int level) {
  // The player's side gets easier as the computer's gets harder; only the
  // middle level has both on normal.
  static constexpr std::array<DifficultyChoice, kDifficultyLevels> kChoices = {{
      {.player = DifficultyLevel::kEasy, .computer = DifficultyLevel::kHard},
      {.player = DifficultyLevel::kNormal, .computer = DifficultyLevel::kHard},
      {.player = DifficultyLevel::kNormal,
       .computer = DifficultyLevel::kNormal},
      {.player = DifficultyLevel::kNormal, .computer = DifficultyLevel::kEasy},
      {.player = DifficultyLevel::kHard, .computer = DifficultyLevel::kEasy},
  }};
  return kChoices.at(base::ToSize(level));
}

DifficultyChoice AskDifficulty(const bool is_expansion_mission) {
  // The dialog, centred on the screen.
  constexpr int kDialogWidth = 500;
  constexpr int kDialogHeight = 160;
  constexpr int kDialogX = (Screen::kWidth - kDialogWidth) / 2;
  constexpr int kDialogY = (Screen::kHeight - kDialogHeight) / 2;

  // The prompt, the slider and the OK button's right edge all keep this far
  // from the dialog's sides.
  constexpr int kMargin = 40;

  constexpr int kPromptX = kDialogX + kMargin;
  constexpr int kPromptY = kDialogY + 30;
  constexpr int kPromptWrapWidth = kDialogWidth - 120;

  constexpr int kSliderX = kDialogX + kMargin;
  constexpr int kSliderY = kDialogY + kDialogHeight - 58;
  constexpr int kSliderWidth = kDialogWidth - (2 * kMargin);
  constexpr int kSliderHeight = 16;
  constexpr int kSliderLabelY = kSliderY - 18;  // The ends' and middle's names.

  constexpr int kButtonWidth = 60;
  constexpr int kButtonX = kDialogX + kDialogWidth - kMargin - kButtonWidth;
  constexpr int kButtonY = kDialogY + kDialogHeight - 36;

  // Gadget ids; Input() reports a clicked button as ButtonKey(id).
  constexpr int kOkButtonId = 1;
  constexpr int kSliderId = 2;

  // Format_Window_String() breaks the prompt into lines by writing into it,
  // so work on a copy of the string table's text.
  std::string prompt = TextString(TXT_DIFFICULTY);
  // An expansion mission is not part of a campaign: keep only the first
  // sentence, since the rest of the prompt talks about the campaign.
  if (is_expansion_mission) {
    const size_t period = prompt.find('.');
    if (period != std::string::npos) {
      prompt.erase(period + 1);
    }
  }
  PixelView& view = TheScreen().visible_view();
  int text_width = 0;
  int text_height = 0;
  Format_Window_String(TextFontStyle(FontType::k6PointGradient, TPF_NOSHADOW),
                       prompt, kPromptWrapWidth, text_width, text_height);

  TextButtonClass ok_button(kOkButtonId, TXT_OK, kButtonFont, kButtonFlags,
                            kButtonX, kButtonY, kButtonWidth);

  // Every level when the rules ask for fine difficulty, otherwise the coarse
  // stops; either way the slider starts in the middle, on normal.
  const int stops =
      TheRules().IsFineDifficulty ? kDifficultyLevels : kCoarseDifficultyStops;
  SliderClass slider(kSliderId, kSliderX, kSliderY, kSliderWidth, kSliderHeight,
                     game::SliderStyle::kSunkenBar);
  slider.Set_Maximum(stops);
  slider.Set_Value(stops / 2);
  game::GadgetList gadgets;
  gadgets.Add(ok_button);
  gadgets.Add(slider);

  game::TheMouse().Hide();
  Dialog_Box(kDialogX, kDialogY, kDialogWidth, kDialogHeight);
  Fancy_Text_Print(view, prompt.c_str(), kPromptX, kPromptY, Get_Color_Scheme(),
                   game::kTransparentBlack, FontType::k6PointGradient,
                   TPF_NOSHADOW);

  // Label the slider's ends and its middle.
  Fancy_Text_Print(view, TXT_HARD, slider.X + slider.Width, kSliderLabelY,
                   Get_Color_Scheme(), game::kTransparentBlack,
                   FontType::k6PointGradient, TPF_RIGHT | TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_EASY, slider.X, kSliderLabelY, Get_Color_Scheme(),
                   game::kTransparentBlack, FontType::k6PointGradient,
                   TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_NORMAL, slider.X + (slider.Width / 2),
                   kSliderLabelY, Get_Color_Scheme(), game::kTransparentBlack,
                   FontType::k6PointGradient, TPF_CENTER | TPF_DROPSHADOW);
  gadgets.DrawAll(view);
  game::TheMouse().Show();

  engine::window::KeyNumber input = KN_NONE;
  while (input != KN_RETURN &&
         input != engine::window::ButtonKey(kOkButtonId)) {
    // Keeps the music, the timers and the window serviced while waiting.
    ServiceRealTime();
    input = gadgets.Input(view).key;
  }

  // The coarse slider's stops are spread over the whole scale.
  return DifficultyForLevel(slider.Get_Value() * (TheRules().IsFineDifficulty
                                                      ? 1
                                                      : kCoarseLevelStep));
}

}  // namespace ra
