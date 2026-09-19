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

/* $Header: /CounterStrike/LOADDLG.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LOADDLG.CPP *
 *                                                                                             *
 *                   Programmer : Maria Legg, Joe Bostic, Bill Randolph *
 *                                                                                             *
 *                   Start Date : March 19, 1995 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LoadOptionsClass::LoadOptionsClass -- class constructor *
 *   LoadOptionsClass::~LoadOptionsClass -- class destructor *
 *   LoadOptionsClass::Process -- main processing routine *
 *   LoadOptionsClass::Clear_List -- clears the list box & Files arrays *
 *   LoadOptionsClass::Fill_List -- fills the list box & GameNum arrays *
 *   LoadOptionsClass::Num_From_Ext -- clears the list box & GameNum arrays *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/loaddlg.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

#include "absl/strings/match.h"
#include "absl/strings/str_format.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/platform.h"
#include "port/safe_string.h"
#include "ra/audio.h"
#include "ra/config.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/edit.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/msgbox.h"
#include "ra/palette.h"
#include "ra/saveload.h"
#include "ra/session.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/theme.h"
#include "ra/toggle.h"
#include "sdllib/file.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/wwstd.h"
#include "tech/ftimer.h"
#include "tech/mix_archive.h"
#include "tech/number_parse.h"
#include "tech/readline.h"

#ifdef _WIN32
#include <io.h>  // for unlink
#else
#include <unistd.h>
#endif

/***********************************************************************************************
 * LoadOptionsClass::LoadOptionsClass -- class constructor *
 *                                                                                             *
 * INPUT: * style      style for this load/save dialog (LOAD/SAVE/DELETE) *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
LoadOptionsClass::LoadOptionsClass(LoadStyleType style) : Style(style) {
  Files.Clear();
}

/***********************************************************************************************
 * LoadOptionsClass::~LoadOptionsClass -- class destructor *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
LoadOptionsClass::~LoadOptionsClass() {
  for (int i = 0; i < Files.Count(); i++) {
    delete Files.at(i);
  }
  Files.Clear();
}

/***********************************************************************************************
 * LoadOptionsClass::Process -- main processing routine *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * false = User cancelled, true = operation completed *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
bool LoadOptionsClass::Process() {
  /*
  **	Dialog & button dimensions
  */
  const int d_dialog_w = 500;                             // dialog width
  const int d_dialog_h = 312;                             // dialog height
  const int d_dialog_x = (640 - d_dialog_w) / 2;          // centered x-coord
  const int d_dialog_y = (400 - d_dialog_h) / 2;          // centered y-coord
  const int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);  // coord of x-center

  const int d_txt8_h = 22;  // ht of 8-pt text
  const int d_margin = 14;  // margin width/height
  const int x_margin = 32;  // margin width/height

  const int d_list_w = d_dialog_w - (x_margin * 2);
  const int d_list_h = 208;
  const int d_list_x = d_dialog_x + x_margin;
  const int d_list_y = d_dialog_y + d_margin + d_txt8_h + d_margin;

  const int d_edit_w = d_dialog_w - (x_margin * 2);
  const int d_edit_x = d_dialog_x + x_margin;
  const int d_edit_y = d_list_y + d_list_h - 60 + d_margin + d_txt8_h;

  const int d_button_w = config::kIsEnglish ? 80 : 100;
  const int d_button_h = 26;
  const int d_button_x = d_dialog_cx - d_button_w - d_margin;
  const int d_button_y = d_dialog_y + d_dialog_h - d_button_h - d_margin;

  const int d_cancel_w = config::kIsEnglish ? 80 : 120;
  const int d_cancel_h = 26;
  const int d_cancel_x = d_dialog_cx + d_margin;
  const int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin;

  /*
  **	Button enumerations
  */
  constexpr int kButtonLoad = 100;
  constexpr int kButtonSave = 101;
  constexpr int kButtonDelete = 102;
  constexpr int kButtonCancel = 103;
  constexpr int kButtonList = 104;
  constexpr int kButtonEdit = 105;

  /*
  **	Redraw values: in order from "top" to "bottom" layer of the dialog
  */
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*
  **	Dialog variables
  */
  bool cancel = false;     // true = user cancels
  int list_ht = d_list_h;  // adjusted list box height

  /*
  **	Other Variables
  */
  int btn_txt = 0;                     // text on the 'OK' button
  unsigned btn_id = 0;                 // ID of 'OK' button
  int caption = 0;                     // dialog caption
  int game_idx = 0;                    // index of game to save/load/etc
  int game_num = 0;                    // file number of game to load/save/etc
  char game_descr[kDescripMax] = {0};  // save-game description
  char fname[port::kMaxFname +
             port::kMaxExt];  // for generating filename to delete

  /*
  **	Buttons
  */
  ControlClass* commands = nullptr;  // the button list

  switch (Style) {
    case LOAD:
      btn_txt = TXT_LOAD_BUTTON;
      btn_id = kButtonLoad;
      caption = TXT_LOAD_MISSION;
      break;

    case SAVE:
      btn_txt = TXT_SAVE_BUTTON;
      btn_id = kButtonSave;
      caption = TXT_SAVE_MISSION;
      list_ht -= 30;
      break;

    case LoadStyleType::NONE:
    case LoadStyleType::WWDELETE:
    default:
      btn_txt = TXT_DELETE_BUTTON;
      btn_id = kButtonDelete;
      caption = TXT_DELETE_MISSION;
      break;
  }

  TextButtonClass button(btn_id, btn_txt, kTpfButton, d_button_x, d_button_y,
                         d_button_w);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, kTpfButton, d_cancel_x,
                            d_cancel_y, d_cancel_w);

  ListClass listbtn(kButtonList, d_list_x, d_list_y, d_list_w, list_ht,
                    TPF_6PT_GRAD | TPF_NOSHADOW,
                    MixArchive::RetrieveData("BTN-UP.SHP"),
                    MixArchive::RetrieveData("BTN-DN.SHP"));

  EditClass editbtn(kButtonEdit, game_descr, sizeof(game_descr) - 4,
                    TPF_6PT_GRAD | TPF_NOSHADOW, d_edit_x, d_edit_y, d_edit_w,
                    -1, EditClass::kAlphanumeric);

  /*
  **	Initialize.
  */
  Set_Logic_Page(SeenBuff);

  Fill_List(&listbtn);

  /*
  **	Do nothing if list is empty.
  */
  if ((Style == LOAD || Style == WWDELETE) && listbtn.Count() == 0) {
    Clear_List(&listbtn);
    WWMessageBox().Process(TXT_NO_SAVES);
    return false;
  }

  /*
  **	Create the button list.
  */
  commands = &button;
  cancelbtn.Add_Tail(*commands);
  listbtn.Add_Tail(*commands);
  if (Style == SAVE) {
    editbtn.Add_Tail(*commands);
    editbtn.Set_Focus();
  }

  /*
  **	Main Processing Loop.
  */
  Keyboard->Clear();
  bool firsttime = true;
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback.
    */
    if (Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH) {
      ServiceRealTime();
    } else {
      if (RunFrame()) {
        process = false;
        cancel = true;
      }
    }

    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = true;
    }

    /*
    **	Refresh display if needed.
    */
    if (display) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
      Draw_Caption(caption, d_dialog_x, d_dialog_y, d_dialog_w);

      if (Style == SAVE) {
        Fancy_Text_Print(TXT_MISSION_DESCRIPTION, d_dialog_cx,
                         d_edit_y - d_txt8_h, GadgetClass::Get_Color_Scheme(),
                         kTBlack, kTpfText | TPF_CENTER);
      }

      /*
      **	Redraw the buttons.
      */
      commands->Flag_List_To_Redraw();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input.
    */
    KeyNumType input = commands->Input();

    /*
    **	The first time through the processing loop, set the edit
    **	gadget to have the focus if this is the save dialog. The
    **	focus must be set here since the gadget list has changed
    **	and this change will cause any previous focus setting to be
    **	cleared by the input processing routine.
    */
    if (firsttime && Style == SAVE) {
      firsttime = false;
      editbtn.Set_Focus();
      editbtn.Flag_To_Redraw();
    }

    /*
    **	If the <RETURN> key was pressed, then default to the appropriate
    **	action button according to the style of this dialog box.
    */
    if (input == KN_RETURN || input == ButtonKey(kButtonEdit)) {
      ToggleClass* toggle = nullptr;
      switch (Style) {
        case SAVE:
          input = ButtonKey(kButtonSave);
          cancelbtn.Turn_Off();
          //					cancelbtn.IsOn = false;
          toggle =
              dynamic_cast<ToggleClass*>(commands->Extract_Gadget(kButtonSave));
          if (toggle != nullptr) {
            toggle->Turn_On();
            //						toggle->IsOn = true;
            toggle->IsPressed = true;
          }
          break;

        case LOAD:
          input = ButtonKey(kButtonLoad);
          //					cancelbtn.IsOn = false;
          cancelbtn.Turn_Off();
          toggle =
              dynamic_cast<ToggleClass*>(commands->Extract_Gadget(kButtonLoad));
          if (toggle != nullptr) {
            toggle->IsOn = true;
            toggle->IsPressed = true;
          }
          break;

        case WWDELETE:
          input = ButtonKey(kButtonDelete);
          //					cancelbtn.IsOn = false;
          cancelbtn.Turn_Off();
          toggle = dynamic_cast<ToggleClass*>(
              commands->Extract_Gadget(kButtonDelete));
          if (toggle != nullptr) {
            toggle->IsOn = true;
            toggle->IsPressed = true;
          }
          break;
        case LoadStyleType::NONE:
        default:
          break;
      }
      Hide_Mouse();
      commands->Draw_All(true);
      Show_Mouse();
    }

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
      /*
      ** Load: if load fails, present a message, and stay in the dialog
      ** to allow the user to try another game
      */
      case ButtonKey(kButtonLoad):
        game_idx = listbtn.Current_Index();
        if (game_idx < 0 || game_idx >= Files.Count()) {
          break;
        }
        game_num = Files.at(game_idx)->Num;
        if (Files.at(game_idx)->Valid) {
          // Shown in case a load is ever slow; it no longer holds the screen
          // for a second, since loading takes milliseconds.
          WWMessageBox().Process(TXT_LOADING, TXT_NONE);
          Theme.Fade_Out();
          const bool rc = Load_Game(game_num);  // return code
          Keyboard->Clear();

          if (!rc) {
            WWMessageBox().Process(TXT_ERROR_LOADING_GAME);
          } else {
            // "Mission loaded" plays over the fade into the mission rather
            // than holding the load screen until it finishes.
            Speak(VOX_LOAD1);
            Hide_Mouse();
            SeenBuff.Clear();
            GamePalette.Set();
            //						Set_Palette(GamePalette);
            Show_Mouse();
            process = false;
          }
        } else {
          WWMessageBox().Process(TXT_OBSOLETE_SAVEGAME);
        }
        break;

      /*
      ** Save: Save the game & exit the dialog
      */
      case ButtonKey(kButtonEdit):

      case ButtonKey(kButtonSave):
        if (std::string_view(game_descr).empty()) {
          WWMessageBox().Process(TXT_MUSTENTER_DESCRIPTION);
          firsttime = true;
          display = true;
          break;
        }
        game_idx = listbtn.Current_Index();
        if (Disk_Space_Available() < kSaveGameDiskSpace && game_idx == 0) {
          WWMessageBox().Process(TXT_SPACE_CANT_SAVE);
          firsttime = true;
          display = true;
          break;
        }
        if (game_idx < 0 || game_idx >= Files.Count()) {
          break;
        }

        game_num = Files.at(game_idx)->Num;
        if (!Save_Game(game_num, game_descr)) {
          WWMessageBox().Process(TXT_ERROR_SAVING_GAME);
        } else {
          // "Mission saved" plays while the message is up, rather than
          // before it; it finishes after the dialog closes if need be.
          Speak(VOX_SAVE1);
          Timer<SystemTickSource> timer;
          timer.Set(int64_t{kTicksPerSecond} * 4);  // 60 ticks: one second

          WWMessageBox().Process(TXT_GAME_WAS_SAVED, TXT_NONE, TXT_NONE);

          // Give the player a second to read the message.
          while (timer.HasTimeLeft()) {
            ServiceRealTime();
          }
          Keyboard->Clear();
        }
        process = false;
        break;

      /*
      ** Delete: delete the file & stay in the dialog, to allow the user
      ** to delete multiple files.
      */
      case ButtonKey(kButtonDelete):
        game_idx = listbtn.Current_Index();
        if (game_idx < 0 || game_idx >= Files.Count()) {
          break;
        }
        game_num = Files.at(game_idx)->Num;
        if (WWMessageBox().Process(TXT_DELETE_FILE_QUERY, TXT_YES, TXT_NO) ==
            0) {
          absl::SNPrintF(fname, sizeof(fname), "SAVEGAME.%03d", game_num);
          unlink(fname);
          Clear_List(&listbtn);
          Fill_List(&listbtn);
          if (listbtn.Count() == 0) {
            process = false;
          } else {
            auto* toggle = dynamic_cast<ToggleClass*>(
                commands->Extract_Gadget(kButtonDelete));
            if (toggle != nullptr) {
              //							toggle->IsOn
              //= false;
              toggle->Turn_Off();
              toggle->IsPressed = false;
              toggle->Flag_To_Redraw();
            }
          }
        }
        display = true;
        break;

      /*
      ** If the user clicks on the list, see if the there is a new current
      ** item; if so, and if we're in SAVE mode, copy the list item into
      ** the save-game description field.
      */
      case ButtonKey(kButtonList):
        if (Style != SAVE) {
          break;
        }

        if (listbtn.Count() && listbtn.Current_Index() != game_idx) {
          game_idx = listbtn.Current_Index();

          /*
          ** Copy the game's description, UNLESS it's the empty slot; if
          ** it is, set the edit buffer to empty.
          */
          if (game_idx != 0) {
            port::SafeCopy(game_descr, listbtn.Get_Item(game_idx));

            /*
            **	Strip any leading parenthesis off of the description.
            */
            if (game_descr[0] == '(') {
              const auto separator = std::string_view(game_descr).find(')');
              if (separator != std::string_view::npos) {
                const auto remainder =
                    std::span(game_descr).subspan(separator + 1);
                base::MoveBytes(base::ObjectBytes(game_descr),
                                std::as_bytes(remainder),
                                std::string_view(remainder.data()).size() + 1);
                strtrim(game_descr);
              }
            }

          } else {
            game_descr[0] = 0;
          }
          editbtn.Set_Text(game_descr, 40);
        }
        break;

      /*
      ** ESC/Cancel: break
      */
      case KN_ESC:
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      default:
        break;
    }
  }

  Clear_List(&listbtn);

  return !cancel;
}

/***********************************************************************************************
 * LoadOptionsClass::Clear_List -- clears the list box & Files arrays *
 *                                                                                             *
 * This step is essential, because it frees all the strings allocated for list
 *items.          *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
void LoadOptionsClass::Clear_List(ListClass* list) {
  /*
  ** For every item in the list, free its buffer & remove it from the list.
  */
  const int j = list->Count();
  for (int i = 0; i < j; i++) {
    list->Remove_Item(list->Get_Item(0));
  }

  /*
  ** Clear the array of game numbers
  */
  for (int i = 0; i < Files.Count(); i++) {
    delete Files.at(i);
  }
  Files.Clear();
}

/***********************************************************************************************
 * LoadOptionsClass::Fill_List -- fills the list box & GameNum arrays *
 *                                                                                             *
 * INPUT: * none. *
 *                                                                                             *
 * OUTPUT: * none. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. * 06/25/1995 JLB : Shows which saved
 *games are "(old)".                                     *
 *=============================================================================================*/
void LoadOptionsClass::Fill_List(ListClass* list) {
  FileEntryClass* fdata = nullptr;  // for adding entries to 'Files'
  char descr[kDescripMax + 32];
  unsigned scenario = 0;  // scenario #
  HousesType house = HOUSE_NONE;
  FindFileState find_state{};
  int id = 0;

  /*
  ** Make sure the list is empty
  */
  Clear_List(list);

  /*
  ** Add the Empty Slot entry
  */
  if (Style == SAVE) {
    fdata = new FileEntryClass;
    port::SafeCopy(fdata->Descr, Text_String(TXT_EMPTY_SLOT));
    fdata->DateTime = 0xffffffff;  // will always be first
    Files.Add(fdata);
  }

  /*
  ** Find all savegame files
  */
  bool found = Find_First_File("SAVEGAME.*", find_state);

  while (found) {
    if (!absl::EqualsIgnoreCase(find_state.name, kNetSaveFileName)) {
      /*
      ** Extract the game ID from the filename
      */
      id = Num_From_Ext(find_state.name);

      /*
      ** get the game's info; if success, add it to the list
      */
      const bool ok =
          Get_Savefile_Info(id, descr, sizeof(descr), &scenario, &house);

      fdata = new FileEntryClass;

      fdata->Descr[0] = '\0';
      if (!ok) {
        port::SafeCopy(fdata->Descr, Text_String(TXT_OLD_GAME));
      } else {
        if (IsSovietHouse(house)) {
          absl::SNPrintF(fdata->Descr, sizeof(fdata->Descr), "(%s) ",
                         Text_String(TXT_SOVIET));
        } else {
          absl::SNPrintF(fdata->Descr, sizeof(fdata->Descr), "(%s) ",
                         Text_String(TXT_ALLIES));
        }
      }
      port::SafeAppend(fdata->Descr, descr);
      fdata->Valid = ok;
      fdata->Scenario = scenario;
      fdata->House = house;
      fdata->Num = id;
      fdata->DateTime = find_state.mod_time;
      Files.Add(fdata);
    }

    /*
    ** Find the next file
    */
    found = Find_Next_File(find_state);
  }

  /*
  ** If saving a game, determine a unique file ID for the empty slot
  */
  if (Style == SAVE) {
    /*
    ** Find an un-used number to associate with the Empty Slot by looking in
    ** GameNum for each number from 0 to 'N', where 'N' is the # of entries
    ** in the list; if any number isn't found, use that number; otherwise,
    ** use 'N + 1'.
    */
    int i = 0;
    for (i = 0; i < Files.Count(); i++) {  // i = the # we're searching for
      id = -1;                             // mark as 'not found'
      for (int j = 0; j < Files.Count(); j++) {  // loop through all game ID's
        if (Files.at(j)->Num == i) {             // if found, mark as found
          id = j;
          break;
        }
      }
      if (id == -1) {
        break;  // if ID not found, use this one
      }
    }

    if (Files.Count() > 0) {
      Files.at(0)->Num = i;  // set the empty slot's ID
    }
  }

  /*
  ** Now sort the list in order of Date/Time (newest first, oldest last)
  */
  if (Files.Count() > 0) {
    std::vector<FileEntryClass*> sorted;
    sorted.reserve(base::ToSize(Files.Count()));
    for (int i = 0; i < Files.Count(); ++i) {
      sorted.push_back(Files.at(i));
    }
    std::ranges::sort(
        sorted, [](const FileEntryClass* left, const FileEntryClass* right) {
          return left->DateTime > right->DateTime;
        });
    for (int i = 0; i < Files.Count(); ++i) {
      Files.at(i) = sorted.at(base::ToSize(i));
    }
  }

  /*
  ** Now add every file's name to the list box
  */
  for (int i = 0; i < Files.Count(); i++) {
    list->Add_Item(Files.at(i)->Descr);
  }
}

/***********************************************************************************************
 * LoadOptionsClass::Num_From_Ext -- clears the list box & GameNum arrays *
 *                                                                                             *
 * INPUT: * fname      filename to parse *
 *                                                                                             *
 * OUTPUT: * File number for this name. *
 *                                                                                             *
 * WARNINGS: * none. *
 *                                                                                             *
 * HISTORY: * 02/14/1995 BR : Created. *
 *=============================================================================================*/
int LoadOptionsClass::Num_From_Ext(const char* fname) {
  const auto ext = std::filesystem::path(fname).extension().string();

  int num = 0;
  if (ext.size() > 1) {  // Has more than just '.'
    num = tech::ParseInteger<int>(std::string_view(ext).substr(1)).value_or(0);
  }
  return num;
}
