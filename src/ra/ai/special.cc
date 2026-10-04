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

#include <cstddef>
#include <iterator>
#include <memory>
#include <string>

#include "base/array.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/ui/gadget.h"
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
#define kOptionWidth (236 * 2)
#define kOptionHeight (162 * 2)
#define kOptionX ((640 - kOptionWidth) / 2)
#define kOptionY ((400 - kOptionHeight) / 2)

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

void RunSpecialOptionsDialog(bool simple) {
  // A copy the dialog edits; the installed flags change only on OK.
  SpecialOptions edited_options = TheSpecialOptions();
  game::GadgetClass* gadgets = nullptr;
  // The flags the dialog offers, each named by the text ID of its label.
  // "checked" follows the check box and is what OK applies.
  struct {
    int label;
    bool checked;
    std::unique_ptr<CheckBoxClass> check_box;
  } options[] = {
      {TXT_THREE_POINT, false, nullptr},
      {TXT_SPEED_BUILD, false, nullptr},
  };

  TextButtonClass ok_button(200, TXT_OK, kButtonFont, kButtonFlags,
                            kOptionX + 30, kOptionY + kOptionHeight - 30);
  TextButtonClass cancel_button(201, TXT_CANCEL, kButtonFont, kButtonFlags,
                                kOptionX + kOptionWidth - 120,
                                kOptionY + kOptionHeight - 30);
  gadgets = &ok_button;
  cancel_button.Add(*gadgets);

  // Check box IDs are 100 plus the option's index; the input loop maps an ID
  // back to its option the same way.
  for (int index = 0; index < std::ssize(options); index++) {
    base::At(options, index).check_box = std::make_unique<CheckBoxClass>(
        static_cast<unsigned>(100 + index), kOptionX + 34,
        kOptionY + 40 + (index * 20));
    if (base::At(options, index).check_box) {
      base::At(options, index).check_box->Add(*gadgets);

      bool value = false;
      switch (base::At(options, index).label) {
        case TXT_THREE_POINT:
          value = TheSpecialOptions().three_point_turns;
          break;

        case TXT_SPEED_BUILD:
          value = TheSpecialOptions().speed_build;
          break;
        default:
          break;
      }

      base::At(options, index).checked = value;
      if (value) {
        base::At(options, index).check_box->Turn_On();
      } else {
        base::At(options, index).check_box->Turn_Off();
      }
    }
  }

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

      for (const auto& option : options) {
        Fancy_Text_Print(view, option.label, option.check_box->X + 20,
                         option.check_box->Y, Get_Color_Scheme(), kTBlack,
                         FontType::k6PointGradient,
                         TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      gadgets->Draw_All(view);
      game::TheMouse().Show();
    }

    const engine::window::KeyNumber input = gadgets->Input(view).key;
    switch (static_cast<int>(input)) {
      case KN_ESC:
      case engine::window::ButtonKey(200):
        process = false;
        for (const auto& option : options) {
          const bool setting = option.checked;
          switch (option.label) {
            case TXT_THREE_POINT:
              edited_options.three_point_turns = setting;
              break;

            case TXT_SPEED_BUILD:
              edited_options.speed_build = setting;
              break;
            default:
              break;
          }
        }
        // Queued as an event rather than assigned, so that every machine
        // changes its flags on the same frame.
        if (!simple) {
          TheNetwork().out_list().Add(EventClass(edited_options));
        } else {
          TheSpecialOptions() = edited_options;
        }
        break;

      case engine::window::ButtonKey(201):
        process = false;
        break;

      case KN_NONE:
        break;

      // A check box was toggled, or a key was pressed. The unsigned compare
      // rejects the negative index that an ID below 100 gives.
      default:
        const int index = ButtonId(input) - 100;
        if (static_cast<unsigned>(index) <
            sizeof(options) / sizeof(options[0])) {
          base::At(options, index).checked =
              base::At(options, index).check_box->IsOn;
        }
        break;
    }
  }

  if (!simple) {
    TheMap().Revert_Mouse_Shape();
    TheScreen().hidden_view().Clear();
    TheMap().Flag_To_Redraw(true);
    TheMap().Render();
  }
}

int PromptForDifficulty(bool is_expansion_mission) {
  const int dialog_width = 500;
  const int dialog_height = 160;
  const int dialog_x = (640 / 2) - (dialog_width / 2);
  const int dialog_y = (400 / 2) - (dialog_height / 2);
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
  game::GadgetClass* gadgets = &ok_button;

  // Five stops when the rules ask for fine difficulty, otherwise three; either
  // way the slider starts in the middle, on normal.
  SliderClass slider(2, dialog_x + 40, dialog_y + dialog_height - 58,
                     dialog_width - 80, 16, true);
  if (TheRules().IsFineDifficulty) {
    slider.Set_Maximum(5);
    slider.Set_Value(2);
  } else {
    slider.Set_Maximum(3);
    slider.Set_Value(1);
  }
  slider.Add(*gadgets);

  bool redraw = true;
  bool process = true;
  while (process) {
    if (redraw) {
      redraw = false;

      game::TheMouse().Hide();
      Dialog_Box(dialog_x, dialog_y, dialog_width, dialog_height);

      // The prompt.
      Fancy_Text_Print(view, prompt.c_str(), dialog_x + 40, dialog_y + 30,
                       Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                       TPF_NOSHADOW);

      // Label the slider's ends and its middle.
      Fancy_Text_Print(view, TXT_HARD, slider.X + slider.Width, slider.Y - 18,
                       Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                       TPF_RIGHT | TPF_DROPSHADOW);
      Fancy_Text_Print(view, TXT_EASY, slider.X, slider.Y - 18,
                       Get_Color_Scheme(), kTBlack, FontType::k6PointGradient,
                       TPF_DROPSHADOW);
      Fancy_Text_Print(view, TXT_NORMAL, slider.X + (slider.Width / 2),
                       slider.Y - 18, Get_Color_Scheme(), kTBlack,
                       FontType::k6PointGradient, TPF_CENTER | TPF_DROPSHADOW);

      if (gadgets) {
        gadgets->Draw_All(view);
      }
      game::TheMouse().Show();
    }

    // Keeps the music, the timers and the window serviced while waiting.
    ServiceRealTime();

    const engine::window::KeyNumber input = gadgets->Input(view).key;

    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case 1U | kButtonFlag:
        process = false;
        break;

      default:
        break;
    }
  }

  // The coarse slider's 0..2 is spread over the same 0..4 scale.
  return slider.Get_Value() * (TheRules().IsFineDifficulty ? 1 : 2);
}

}  // namespace ra
