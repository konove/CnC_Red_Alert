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

#include "td/expand.h"

#include <cstddef>
#include <format>
#include <span>
#include <string>
#include <vector>

#include "base/array.h"
#include "base/numeric.h"
#include "port/bytes_of.h"
#include "port/safe_string.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/shape.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "td/conquer.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/ini.h"
#include "td/init.h"
#include "td/jshell.h"
#include "td/list.h"
#include "td/profile.h"
#include "td/text.h"
#include "td/textbtn.h"
#include "tech/game_file.h"

#ifdef NEWMENU

bool Expansion_Present() {
  GameFile file("EXPAND.DAT");

  return file.IsAvailable();
}

// List box of expansion scenarios: each line's scenario number is kept
// alongside its text.
class EListClass : public ListClass {
 public:
  EListClass(int id, int x, int y, int w, int h, TextPrintType flags,
             std::span<const std::byte> up, std::span<const std::byte> down)
      : ListClass(id, x, y, w, h, flags, up, down) {}
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
  void Draw_Entry(int index, int x, int y, int width, bool selected) override;

 private:
  // One per item, parallel to List.
  std::vector<int> Scenarios;
};

void EListClass::Draw_Entry(int index, int x, int y, int width, bool selected) {
  if (base::Any(TextFlags & TPF_6PT_GRAD)) {
    TextPrintType flags = TextFlags;

    if (selected) {
      flags = flags | TPF_BRIGHT_COLOR;
      LogicPage->Fill_Rect(x, y, x + width - 1, y + LineHeight - 1,
                           kCcGreenShadow);
    } else {
      if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
        flags = flags | TPF_MEDIUM_COLOR;
      }
    }

    Conquer_Clip_Text_Print(Get_Item(index), x, y, kCcGreen, kTBlack, flags,
                            width, Tabs);

  } else {
    Conquer_Clip_Text_Print(Get_Item(index), x, y, selected ? kBlue : kWhite,
                            kTBlack, TextFlags, width, Tabs);
  }
}

bool Expansion_Dialog() {
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  const int option_width = 236 * factor;
  const int option_height = 162 * factor;
  const int option_x = ((320 * factor) - option_width) / 2;
  const int option_y = ((200 * factor) - option_height) / 2;

  GadgetClass* buttons = nullptr;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD | TPF_NOSHADOW,
                     option_x + (25 * factor),
                     option_y + option_height - (15 * factor));
  TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD | TPF_NOSHADOW,
                         option_x + option_width - (50 * factor),
                         option_y + option_height - (15 * factor));
  EListClass list(202, option_x + (10 * factor), option_y + (20 * factor),
                  option_width - (20 * factor), option_height - (40 * factor),
                  TPF_6PT_GRAD | TPF_NOSHADOW, up_button, down_button);

  buttons = &ok;
  cancel.Add(*buttons);
  list.Add(*buttons);

  /*
  **	Add in all the expansion scenarios.
  */
  const auto sbuffer = port::CharBytes(ShapeBufferBytes);
  for (int index = 20; index < 60; index++) {
    char buffer[128];
    GameFile file;

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_GDI, SCEN_DIR_EAST,
                      SCEN_VAR_A);
    port::SafeAppend(buffer, ".INI");
    file.SetName(buffer);
    if (file.IsAvailable()) {
      file.Read(sbuffer, 1000);
      base::At(sbuffer, 1000) = '\r';
      base::At(sbuffer, 1000 + 1) = '\n';
      base::At(sbuffer, 1000 + 2) = '\0';

      WWGetPrivateProfileString("Basic", "Name", "x", buffer, sbuffer.data());
      list.Add_Scenario(index, std::format("GDI: {}", buffer));
    }
  }

  for (int index = 20; index < 60; index++) {
    char buffer[128];
    GameFile file;

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_NOD, SCEN_DIR_EAST,
                      SCEN_VAR_A);
    port::SafeAppend(buffer, ".INI");
    file.SetName(buffer);
    if (file.IsAvailable()) {
      file.Read(sbuffer, 1000);
      base::At(sbuffer, 1000) = '\r';
      base::At(sbuffer, 1000 + 1) = '\n';
      base::At(sbuffer, 1000 + 2) = '\0';

      WWGetPrivateProfileString("Basic", "Name", "x", buffer, sbuffer.data());
      list.Add_Scenario(index, std::format("NOD: {}", buffer));
    }
  }

  Set_Logic_Page(SeenBuff);
  bool display = true;
  bool process = true;
  bool okval = true;
  while (process) {
    Call_Back();

    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    if (display) {
      display = false;

      Hide_Mouse();

      /*
      **	Load the background picture.
      */
      Load_Title_Page(true);

      Dialog_Box(option_x, option_y, option_width, option_height);
      Draw_Caption(TXT_MISSION_DESCRIPTION, option_x, option_y, option_width);
      buttons->Draw_All();
      Show_Mouse();
    }

    const KeyNumType input = buttons->Input();
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case ButtonKey(200):
        if (list.Current_Item()[0] == 'G') {
          ScenPlayer = SCEN_PLAYER_GDI;
        } else {
          ScenPlayer = SCEN_PLAYER_NOD;
        }
        ScenDir = SCEN_DIR_EAST;
        Whom = HOUSE_GOOD;
        Scenario = list.Current_Scenario();
        process = false;
        okval = true;
        break;

      case KN_ESC:
      case ButtonKey(201):
        ScenPlayer = SCEN_PLAYER_GDI;
        ScenDir = SCEN_DIR_EAST;
        Whom = HOUSE_GOOD;
        Scenario = list.Current_Scenario();
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
  const int factor = SeenBuff.Get_Width() == 320 ? 1 : 2;

  const int option_width = 236 * factor;
  const int option_height = 162 * factor;
  const int option_x = ((320 * factor) - option_width) / 2;
  const int option_y = ((200 * factor) - option_height) / 2;

  GadgetClass* buttons = nullptr;

  std::span<const std::byte> up_button;
  std::span<const std::byte> down_button;

  if (InMainLoop) {
    up_button = Hires_Retrieve("BTN-UP.SHP");
    down_button = Hires_Retrieve("BTN-DN.SHP");
  } else {
    up_button = Hires_Retrieve("BTN-UP2.SHP");
    down_button = Hires_Retrieve("BTN-DN2.SHP");
  }

  TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD | TPF_NOSHADOW,
                     option_x + (25 * factor),
                     option_y + option_height - (15 * factor));
  TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD | TPF_NOSHADOW,
                         option_x + option_width - (50 * factor),
                         option_y + option_height - (15 * factor));
  EListClass list(202, option_x + (10 * factor), option_y + (20 * factor),
                  option_width - (20 * factor), option_height - (40 * factor),
                  TPF_6PT_GRAD | TPF_NOSHADOW, up_button, down_button);

  buttons = &ok;
  cancel.Add(*buttons);
  list.Add(*buttons);

  /*
  **	Add in all the expansion scenarios.
  */
  const int gdi_scen_names[3] = {TXT_BONUS_MISSION_1, TXT_BONUS_MISSION_2,
                                 TXT_BONUS_MISSION_3};

  const int nod_scen_names[2] = {TXT_BONUS_MISSION_4, TXT_BONUS_MISSION_5};

  for (int index = 60; index < 63; index++) {
    char buffer[128];
    GameFile file;

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_GDI, SCEN_DIR_EAST,
                      SCEN_VAR_A);
    port::SafeAppend(buffer, ".INI");
    file.SetName(buffer);
    if (file.IsAvailable()) {
      list.Add_Scenario(
          index, std::format("GDI: {}", Text_String(base::At(gdi_scen_names,
                                                             index - 60))));
    }
  }

  for (int index = 60; index < 62; index++) {
    char buffer[128];
    GameFile file;

    Set_Scenario_Name(buffer, index, SCEN_PLAYER_NOD, SCEN_DIR_EAST,
                      SCEN_VAR_A);
    port::SafeAppend(buffer, ".INI");
    file.SetName(buffer);
    if (file.IsAvailable()) {
      list.Add_Scenario(
          index, std::format("NOD: {}", Text_String(base::At(nod_scen_names,
                                                             index - 60))));
    }
  }

  Set_Logic_Page(SeenBuff);
  bool display = true;
  bool process = true;
  bool okval = true;
  while (process) {
    Call_Back();

    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    if (display) {
      display = false;

      Hide_Mouse();

      /*
      **	Load the background picture.
      */
      Load_Title_Page(true);

      Dialog_Box(option_x, option_y, option_width, option_height);
      Draw_Caption(TXT_BONUS_MISSIONS, option_x, option_y, option_width);
      buttons->Draw_All();
      Show_Mouse();
    }

    const KeyNumType input = buttons->Input();
    switch (static_cast<int>(input)) {
      case KN_RETURN:
      case ButtonKey(200):
        if (list.Current_Item()[0] == 'G') {
          ScenPlayer = SCEN_PLAYER_GDI;
        } else {
          ScenPlayer = SCEN_PLAYER_NOD;
        }
        ScenDir = SCEN_DIR_EAST;
        Whom = HOUSE_GOOD;
        Scenario = list.Current_Scenario();
        process = false;
        okval = true;
        break;

      case KN_ESC:
      case ButtonKey(201):
        ScenPlayer = SCEN_PLAYER_GDI;
        ScenDir = SCEN_DIR_EAST;
        Whom = HOUSE_GOOD;
        Scenario = list.Current_Scenario();
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
