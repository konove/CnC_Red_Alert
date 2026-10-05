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

#include "ra/ai/special_options.h"

#include <array>
#include <cstddef>
#include <string>

#include "base/numeric.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/input.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/data/rules.h"
#include "ra/data/text.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/net/event.h"
#include "ra/net/network.h"
#include "ra/net/session.h"
#include "ra/startup/conquer.h"
#include "ra/ui/checkbox.h"
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

void SpecialOptions::Reset() {
  shroud_regrows = false;
  speed_build = false;
  from_install = false;
  capture_the_flag = false;
  inert_weapons = false;
  three_point_turns = false;
  ore_grows = true;
  ore_spreads = true;
}

void RunSpecialOptionsDialog() {
  // The dialog, centred on the screen.
  constexpr int kDialogWidth = 472;
  constexpr int kDialogHeight = 324;
  constexpr int kDialogX = (Screen::kWidth - kDialogWidth) / 2;
  constexpr int kDialogY = (Screen::kHeight - kDialogHeight) / 2;

  // OK sits in the bottom left corner and Cancel in the bottom right.
  constexpr int kOkButtonX = kDialogX + 30;
  constexpr int kCancelButtonX = kDialogX + kDialogWidth - 120;
  constexpr int kButtonY = kDialogY + kDialogHeight - 30;

  // The check boxes form a column, each with its label to the right.
  constexpr int kCheckBoxX = kDialogX + 34;
  constexpr int kFirstCheckBoxY = kDialogY + 40;
  constexpr int kCheckBoxSpacing = 20;
  constexpr int kLabelOffsetX = 20;  // From the box's left edge.

  // Gadget ids; Input() reports a clicked button as ButtonKey(id).
  constexpr int kThreePointBoxId = 100;
  constexpr int kSpeedBuildBoxId = 101;
  constexpr int kOkButtonId = 200;
  constexpr int kCancelButtonId = 201;

  // A copy the dialog edits. The installed flags change only when the event OK
  // queues is executed.
  SpecialOptions edited_options = TheSpecialOptions();

  TextButtonClass ok_button(kOkButtonId, TXT_OK, kButtonFont, kButtonFlags,
                            kOkButtonX, kButtonY);
  TextButtonClass cancel_button(kCancelButtonId, TXT_CANCEL, kButtonFont,
                                kButtonFlags, kCancelButtonX, kButtonY);
  cancel_button.Add(ok_button);

  // One check box per flag the dialog offers. A box toggles itself when
  // clicked, so the flags are only read back from the boxes on OK.
  CheckBoxClass three_point_box(kThreePointBoxId, kCheckBoxX, kFirstCheckBoxY);
  CheckBoxClass speed_build_box(kSpeedBuildBoxId, kCheckBoxX,
                                kFirstCheckBoxY + kCheckBoxSpacing);
  three_point_box.Add(ok_button);
  speed_build_box.Add(ok_button);
  if (edited_options.three_point_turns) {
    three_point_box.Turn_On();
  }
  if (edited_options.speed_build) {
    speed_build_box.Turn_On();
  }
  const struct {
    int label;  // String-table number of the text beside the box.
    const CheckBoxClass& check_box;
  } labels[] = {
      {TXT_THREE_POINT, three_point_box},
      {TXT_SPEED_BUILD, speed_build_box},
  };

  TheMap().Override_Mouse_Shape(MOUSE_NORMAL);
  PixelView& view = TheScreen().visible_view();
  bool redraw = true;
  bool process = true;
  while (process) {
    // A multiplayer game cannot pause for a dialog: the other players' frames
    // keep coming, so the game runs on behind it. RunFrame() returns true when
    // that game is over, which closes the dialog without applying anything.
    if (TheSession().Type == GAME_NORMAL) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
      }
    }

    if (redraw) {
      redraw = false;

      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogWidth, kDialogHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kDialogX, kDialogY, kDialogWidth);

      for (const auto& [label, check_box] : labels) {
        Fancy_Text_Print(view, label, check_box.X + kLabelOffsetX, check_box.Y,
                         Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      ok_button.Draw_All(view);
      game::TheMouse().Show();
    }

    switch (static_cast<int>(ok_button.Input(view).key)) {
      case engine::window::ButtonKey(kOkButtonId):
        process = false;
        edited_options.three_point_turns = three_point_box.IsOn;
        edited_options.speed_build = speed_build_box.IsOn;
        // Queued as an event rather than assigned, so that every machine
        // changes its flags on the same frame.
        TheNetwork().out_list().Add(EventClass(edited_options));
        break;

      case KN_ESC:
      case engine::window::ButtonKey(kCancelButtonId):
        process = false;
        break;

      // No input, a check box (which has already toggled itself) or a key.
      default:
        break;
    }
  }

  TheMap().Revert_Mouse_Shape();
  TheScreen().hidden_view().Clear();
  TheMap().Flag_To_Redraw(true);
  TheMap().Render();
}

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

DifficultyChoice PromptForDifficulty(const bool is_expansion_mission) {
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
                     true);
  slider.Set_Maximum(stops);
  slider.Set_Value(stops / 2);
  slider.Add(ok_button);

  game::TheMouse().Hide();
  Dialog_Box(kDialogX, kDialogY, kDialogWidth, kDialogHeight);
  Fancy_Text_Print(view, prompt.c_str(), kPromptX, kPromptY, Get_Color_Scheme(),
                   kTBlack, FontType::k6PointGradient, TPF_NOSHADOW);

  // Label the slider's ends and its middle.
  Fancy_Text_Print(view, TXT_HARD, slider.X + slider.Width, kSliderLabelY,
                   Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                   TPF_RIGHT | TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_EASY, slider.X, kSliderLabelY, Get_Color_Scheme(),
                   kTBlack, FontType::k6PointGradient, TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_NORMAL, slider.X + (slider.Width / 2),
                   kSliderLabelY, Get_Color_Scheme(), kTBlack,
                   FontType::k6PointGradient, TPF_CENTER | TPF_DROPSHADOW);
  ok_button.Draw_All(view);
  game::TheMouse().Show();

  engine::window::KeyNumber input = KN_NONE;
  while (input != KN_RETURN &&
         input != engine::window::ButtonKey(kOkButtonId)) {
    // Keeps the music, the timers and the window serviced while waiting.
    ServiceRealTime();
    input = ok_button.Input(view).key;
  }

  // The coarse slider's stops are spread over the whole scale.
  return DifficultyForLevel(slider.Get_Value() * (TheRules().IsFineDifficulty
                                                      ? 1
                                                      : kCoarseLevelStep));
}

}  // namespace ra
