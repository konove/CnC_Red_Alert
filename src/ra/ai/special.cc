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

#include "ra/ai/special.h"

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
#include "ra/core/inline.h"
#include "ra/data/rules.h"
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

// The options dialog, centred on the 640x400 screen.
constexpr int kOptionWidth = 236 * 2;
constexpr int kOptionHeight = 162 * 2;
constexpr int kOptionX = (Screen::kWidth - kOptionWidth) / 2;
constexpr int kOptionY = (Screen::kHeight - kOptionHeight) / 2;

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
  // A copy the dialog edits. The installed flags change only when the event OK
  // queues is executed.
  SpecialOptions edited_options = TheSpecialOptions();

  TextButtonClass ok_button(200, TXT_OK, kButtonFont, kButtonFlags,
                            kOptionX + 30, kOptionY + kOptionHeight - 30);
  TextButtonClass cancel_button(201, TXT_CANCEL, kButtonFont, kButtonFlags,
                                kOptionX + kOptionWidth - 120,
                                kOptionY + kOptionHeight - 30);
  cancel_button.Add(ok_button);

  // One check box per flag the dialog offers. A box toggles itself when
  // clicked, so the flags are only read back from the boxes on OK.
  CheckBoxClass three_point_box(100, kOptionX + 34, kOptionY + 40);
  CheckBoxClass speed_build_box(101, kOptionX + 34, kOptionY + 60);
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
      Dialog_Box(kOptionX, kOptionY, kOptionWidth, kOptionHeight);
      Draw_Caption(view, TXT_SPECIAL_OPTIONS, kOptionX, kOptionY, kOptionWidth);

      for (const auto& label : labels) {
        Fancy_Text_Print(view, label.label, label.check_box.X + 20,
                         label.check_box.Y, Get_Color_Scheme(), kTBlack,
                         FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      ok_button.Draw_All(view);
      game::TheMouse().Show();
    }

    switch (static_cast<int>(ok_button.Input(view).key)) {
      case KN_ESC:
      case engine::window::ButtonKey(200):
        process = false;
        edited_options.three_point_turns = three_point_box.IsOn;
        edited_options.speed_build = speed_build_box.IsOn;
        // Queued as an event rather than assigned, so that every machine
        // changes its flags on the same frame.
        TheNetwork().out_list().Add(EventClass(edited_options));
        break;

      case engine::window::ButtonKey(201):
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

DifficultyChoice DifficultyForLevel(int level) {
  // The player's side gets easier as the computer's gets harder; only the
  // middle level has both on normal.
  static constexpr std::array<DifficultyChoice, 5> kChoices = {{
      {.player = DIFF_EASY, .computer = DIFF_HARD},
      {.player = DIFF_NORMAL, .computer = DIFF_HARD},
      {.player = DIFF_NORMAL, .computer = DIFF_NORMAL},
      {.player = DIFF_NORMAL, .computer = DIFF_EASY},
      {.player = DIFF_HARD, .computer = DIFF_EASY},
  }};
  return kChoices.at(base::ToSize(level));
}

DifficultyChoice PromptForDifficulty(bool is_expansion_mission) {
  const int dialog_width = 500;
  const int dialog_height = 160;
  const int dialog_x = (Screen::kWidth - dialog_width) / 2;
  const int dialog_y = (Screen::kHeight - dialog_height) / 2;
  const int button_width = 60;

  // Format_Window_String() breaks the prompt into lines by writing into it,
  // so work on a copy of the string table's text.
  std::string prompt = Text_String(TXT_DIFFICULTY);
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
                       prompt, dialog_width - 120, text_width, text_height);

  // The OK button is gadget 1 and the slider gadget 2.
  TextButtonClass ok_button(1, TXT_OK, kButtonFont, kButtonFlags,
                            dialog_x + dialog_width - (button_width + 40),
                            dialog_y + dialog_height - 36, button_width);

  // Five stops when the rules ask for fine difficulty, otherwise three; either
  // way the slider starts in the middle, on normal.
  const int stops = TheRules().IsFineDifficulty ? 5 : 3;
  SliderClass slider(2, dialog_x + 40, dialog_y + dialog_height - 58,
                     dialog_width - 80, 16, true);
  slider.Set_Maximum(stops);
  slider.Set_Value(stops / 2);
  slider.Add(ok_button);

  game::TheMouse().Hide();
  Dialog_Box(dialog_x, dialog_y, dialog_width, dialog_height);
  Fancy_Text_Print(view, prompt.c_str(), dialog_x + 40, dialog_y + 30,
                   Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                   TPF_NOSHADOW);

  // Label the slider's ends and its middle.
  Fancy_Text_Print(view, TXT_HARD, slider.X + slider.Width, slider.Y - 18,
                   Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                   TPF_RIGHT | TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_EASY, slider.X, slider.Y - 18, Get_Color_Scheme(),
                   kTBlack, FontType::k6PointGradient, TPF_DROPSHADOW);
  Fancy_Text_Print(view, TXT_NORMAL, slider.X + (slider.Width / 2),
                   slider.Y - 18, Get_Color_Scheme(), kTBlack,
                   FontType::k6PointGradient, TPF_CENTER | TPF_DROPSHADOW);
  ok_button.Draw_All(view);
  game::TheMouse().Show();

  engine::window::KeyNumber input = KN_NONE;
  while (input != KN_RETURN && input != engine::window::ButtonKey(1)) {
    // Keeps the music, the timers and the window serviced while waiting.
    ServiceRealTime();
    input = ok_button.Input(view).key;
  }

  // The coarse slider's 0..2 is spread over the same five levels.
  return DifficultyForLevel(slider.Get_Value() *
                            (TheRules().IsFineDifficulty ? 1 : 2));
}

}  // namespace ra
