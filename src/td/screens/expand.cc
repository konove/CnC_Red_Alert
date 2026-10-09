/*
**	Command & Conquer(tm)
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

/* $Header$ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : EXPAND.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 11/03/95 *
 *                                                                                             *
 *                  Last Update : November 3, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/screens/expand.h"

#include <array>
#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "base/array.h"
#include "base/numeric.h"
#include "base/strings/safe_string.h"
#include "engine/file/game_file.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/stream/byte_stream.h"
#include "engine/stream/span_source.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/sim/enums.h"
#include "game/ui/gadget.h"
#include "game/ui/gadget_list.h"
#include "game/ui/input.h"
#include "game/ui/standard_colors.h"
#include "td/assets/screen.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"
#include "td/data/ini.h"
#include "td/data/text.h"
#include "td/map/world.h"
#include "td/screens/goptions.h"
#include "td/startup/conquer.h"
#include "td/startup/game_state.h"
#include "td/startup/init.h"
#include "td/ui/dialog.h"
#include "td/ui/list.h"
#include "td/ui/textbtn.h"

namespace td {

namespace {

// Stores the [Basic] Name of the scenario file `file` in `name`, "x" when it
// has none. Only the head of the file is read, which is where the section is.
void ReadScenarioName(ByteStream& file, std::span<char> name) {
  // The head of the file, plus a line end and the terminator.
  constexpr int kHeadSize = 1000;
  std::array<char, kHeadSize + 3> head{};
  file.Read(std::span<char>(head), kHeadSize);
  base::At(head, kHeadSize) = '\r';
  base::At(head, kHeadSize + 1) = '\n';
  base::At(head, kHeadSize + 2) = '\0';

  // The text ends at its first NUL, as the old scan of it did.
  const std::string_view text(head.data());
  SpanSource source(std::as_bytes(std::span(text)));
  engine::file::IniFile scenario;
  scenario.Load(source);
  base::SafeCopy(name, scenario.GetString("Basic", "Name", "x"));
}

}  // namespace

using enum engine::window::KeyNumber;

#ifdef NEWMENU

bool Expansion_Present() { return GameFileExists("EXPAND.DAT"); }

// List box of expansion scenarios: each line's scenario number is kept
// alongside its text.
class EListClass : public ListClass {
 public:
  EListClass(int id, int x, int y, int w, int h, FontType font,
             TextPrintType flags, std::span<const std::byte> up,
             std::span<const std::byte> down)
      : ListClass(id, x, y, w, h, font, flags, up, down) {}
  // Appends a line for `scenario`, returning its index.
  int Add_Scenario(int scenario, const std::string& text) {
    Scenarios.push_back(scenario);
    return ListClass::Add_Item(text.c_str());
  }
  // The selected line's scenario number. Only valid while the list is not
  // empty.
  [[nodiscard]] int Current_Scenario() const {
    return Scenarios.at(base::ToSize(Current_Index()));
  }
  void Remove_Item(int index) override {
    if (index >= 0 && index < Count()) {
      Scenarios.erase(Scenarios.begin() + index);
      ListClass::Remove_Item(index);
    }
  }

 protected:
  void Draw_Entry(PixelView& view, int index, int x, int y, int width,
                  bool selected) override;

 private:
  // One per item, parallel to List.
  std::vector<int> Scenarios;
};

void EListClass::Draw_Entry(PixelView& view, int index, int x, int y, int width,
                            bool selected) {
  if (HasGradientFontBits(TextFont)) {
    TextPrintType flags = TextFlags;

    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      view.FillRect(x, y, x + width - 1, y + LineHeight - 1, kCcGreenShadow);
    } else {
      if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
        flags = flags | TPF_MEDIUM_COLOR;
      }
    }

    Conquer_Clip_Text_Print(view, Get_Item(index), x, y, kCcGreen,
                            game::kTransparentBlack, TextFont, flags, width,
                            Tabs);

  } else {
    Conquer_Clip_Text_Print(
        view, Get_Item(index), x, y, selected ? game::kBlue : game::kWhite,
        game::kTransparentBlack, TextFont, TextFlags, width, Tabs);
  }
}

bool Expansion_Dialog() {
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  const int option_width = 236 * factor;
  const int option_height = 162 * factor;
  const int option_x = ((320 * factor) - option_width) / 2;
  const int option_y = ((200 * factor) - option_height) / 2;

  game::GadgetList buttons;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (TheGameState().in_main_loop()) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  TextButtonClass ok(200, TXT_OK, FontType::k6PointGradient, TPF_NOSHADOW,
                     option_x + (25 * factor),
                     option_y + option_height - (15 * factor));
  TextButtonClass cancel(201, TXT_CANCEL, FontType::k6PointGradient,
                         TPF_NOSHADOW, option_x + option_width - (50 * factor),
                         option_y + option_height - (15 * factor));
  EListClass list(202, option_x + (10 * factor), option_y + (20 * factor),
                  option_width - (20 * factor), option_height - (40 * factor),
                  FontType::k6PointGradient, TPF_NOSHADOW, up_button,
                  down_button);

  buttons.Add(ok);
  buttons.AddAfter(cancel, ok);
  buttons.AddAfter(list, ok);

  /*
  **	Add in all the expansion scenarios.
  */
  for (int index = 20; index < 60; index++) {
    char buffer[128];

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_GDI, game::SCEN_DIR_EAST,
                      SCEN_VAR_A);
    base::SafeAppend(buffer, ".INI");
    if (const auto file = OpenGameFile(buffer)) {
      ReadScenarioName(*file, buffer);
      list.Add_Scenario(index, std::format("GDI: {}", buffer));
    }
  }

  for (int index = 20; index < 60; index++) {
    char buffer[128];

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_NOD, game::SCEN_DIR_EAST,
                      SCEN_VAR_A);
    base::SafeAppend(buffer, ".INI");
    if (const auto file = OpenGameFile(buffer)) {
      ReadScenarioName(*file, buffer);
      list.Add_Scenario(index, std::format("NOD: {}", buffer));
    }
  }

  PixelView& view = TheScreen().visible_view();
  bool display = true;
  bool process = true;
  bool okval = true;
  while (process) {
    Call_Back();

    if (display) {
      display = false;

      game::TheMouse().Hide();

      /*
      **	Load the background picture.
      */
      Load_Title_Page(true);

      Dialog_Box(view, option_x, option_y, option_width, option_height);
      Draw_Caption(view, TXT_MISSION_DESCRIPTION, option_x, option_y,
                   option_width);
      buttons.DrawAll(view);
      game::TheMouse().Show();
    }

    const engine::window::KeyNumber input = buttons.Input(view).key;
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case engine::window::ButtonKey(200):
        if (std::string_view(list.Current_Item()).starts_with('G')) {
          TheWorld().scen_player() = SCEN_PLAYER_GDI;
        } else {
          TheWorld().scen_player() = SCEN_PLAYER_NOD;
        }
        TheWorld().scen_dir() = game::SCEN_DIR_EAST;
        TheWorld().whom() = HOUSE_GOOD;
        TheWorld().scenario() = list.Current_Scenario();
        process = false;
        okval = true;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(201):
        TheWorld().scen_player() = SCEN_PLAYER_GDI;
        TheWorld().scen_dir() = game::SCEN_DIR_EAST;
        TheWorld().whom() = HOUSE_GOOD;
        TheWorld().scenario() = list.Current_Scenario();
        process = false;
        okval = false;
        break;

      default:
        break;
    }
  }

  return okval;
}

/***********************************************************************************************
 * Bonus_Dialog -- Asks the user which bonus mission he wants to play *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 3/26/97 11:07AM ST : Created *
 *=============================================================================================*/
bool Bonus_Dialog() {
  const int factor = TheScreen().visible_view().width() == 320 ? 1 : 2;

  const int option_width = 236 * factor;
  const int option_height = 162 * factor;
  const int option_x = ((320 * factor) - option_width) / 2;
  const int option_y = ((200 * factor) - option_height) / 2;

  game::GadgetList buttons;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (TheGameState().in_main_loop()) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  TextButtonClass ok(200, TXT_OK, FontType::k6PointGradient, TPF_NOSHADOW,
                     option_x + (25 * factor),
                     option_y + option_height - (15 * factor));
  TextButtonClass cancel(201, TXT_CANCEL, FontType::k6PointGradient,
                         TPF_NOSHADOW, option_x + option_width - (50 * factor),
                         option_y + option_height - (15 * factor));
  EListClass list(202, option_x + (10 * factor), option_y + (20 * factor),
                  option_width - (20 * factor), option_height - (40 * factor),
                  FontType::k6PointGradient, TPF_NOSHADOW, up_button,
                  down_button);

  buttons.Add(ok);
  buttons.AddAfter(cancel, ok);
  buttons.AddAfter(list, ok);

  /*
  **	Add in all the expansion scenarios.
  */
  const int gdi_scen_names[3] = {TXT_BONUS_MISSION_1, TXT_BONUS_MISSION_2,
                                 TXT_BONUS_MISSION_3};

  const int nod_scen_names[2] = {TXT_BONUS_MISSION_4, TXT_BONUS_MISSION_5};

  for (int index = 60; index < 63; index++) {
    char buffer[128];

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_GDI, game::SCEN_DIR_EAST,
                      SCEN_VAR_A);
    base::SafeAppend(buffer, ".INI");
    if (GameFileExists(buffer)) {
      list.Add_Scenario(
          index, std::format("GDI: {}", Text_String(base::At(gdi_scen_names,
                                                             index - 60))));
    }
  }

  for (int index = 60; index < 62; index++) {
    char buffer[128];

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_NOD, game::SCEN_DIR_EAST,
                      SCEN_VAR_A);
    base::SafeAppend(buffer, ".INI");
    if (GameFileExists(buffer)) {
      list.Add_Scenario(
          index, std::format("NOD: {}", Text_String(base::At(nod_scen_names,
                                                             index - 60))));
    }
  }

  PixelView& view = TheScreen().visible_view();
  bool display = true;
  bool process = true;
  bool okval = true;
  while (process) {
    Call_Back();

    if (display) {
      display = false;

      game::TheMouse().Hide();

      /*
      **	Load the background picture.
      */
      Load_Title_Page(true);

      Dialog_Box(view, option_x, option_y, option_width, option_height);
      Draw_Caption(view, TXT_BONUS_MISSIONS, option_x, option_y, option_width);
      buttons.DrawAll(view);
      game::TheMouse().Show();
    }

    const engine::window::KeyNumber input = buttons.Input(view).key;
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case engine::window::ButtonKey(200):
        if (std::string_view(list.Current_Item()).starts_with('G')) {
          TheWorld().scen_player() = SCEN_PLAYER_GDI;
        } else {
          TheWorld().scen_player() = SCEN_PLAYER_NOD;
        }
        TheWorld().scen_dir() = game::SCEN_DIR_EAST;
        TheWorld().whom() = HOUSE_GOOD;
        TheWorld().scenario() = list.Current_Scenario();
        process = false;
        okval = true;
        break;

      case KN_ESC:
      case engine::window::ButtonKey(201):
        TheWorld().scen_player() = SCEN_PLAYER_GDI;
        TheWorld().scen_dir() = game::SCEN_DIR_EAST;
        TheWorld().whom() = HOUSE_GOOD;
        TheWorld().scenario() = list.Current_Scenario();
        process = false;
        okval = false;
        break;

      default:
        break;
    }
  }

  return okval;
}

#endif

}  // namespace td
