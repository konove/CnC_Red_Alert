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

/* $Header: /CounterStrike/MAPEDTM.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDTM.CPP                              *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : December 7, 1994                         *
 *                                                                         *
 *                  Last Update : May 7, 1996 [JLB]                        *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Draw_Member -- Draws a member of the team dialog box.   *
 *   MapEditClass::Handle_Teams -- main team-dialog-handling function      *
 *   MapEditClass::Select_Team -- user selects a team from a list          *
 *   MapEditClass::Team_Members -- user picks makeup of a team             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstdint>
#include <string_view>

#include "absl/strings/ascii.h"
#include "base/array.h"
#include "base/pooled.h"
#include "engine/file/mix_archive.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/text_window.h"
#include "engine/gfx/wwstd.h"
#include "engine/platform/timer.h"
#include "engine/window/hardware_cursor.h"
#include "engine/window/keyboard.h"
#include "game/core/object_heap.h"
#include "game/ui/control.h"
#include "game/ui/gadget.h"
#include "game/ui/input.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/teamtype.h"
#include "ra/assets/palettes.h"
#include "ra/assets/screen.h"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/data/text_ids.h"
#include "ra/debug/debug.h"
#include "ra/editor/mapedit.h"
#include "ra/objects/tracker.h"
#include "ra/objects/type.h"
#include "ra/startup/conquer.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"
#include "ra/ui/msgbox.h"
#include "ra/ui/textbtn.h"
#include "ra/ui/tlist.h"

namespace ra {

using enum engine::window::KeyNumber;

/***************************************************************************
 * MapEditClass::Handle_Teams -- main team-dialog-handling function        *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/08/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Handle_Teams(const char* caption) {
  /*
  **	Team dialog processing loop:
  **	- Invoke the team selection dialog. If a team's selected, break
  **	  & return
  **	- If user wants to edit the current team, do so
  **	- If user wants to create new team, new a TeamType & edit it
  **	- If user wants to delete team, delete the current team
  **	- Keep looping until 'OK'
  */
  for (;;) {
    /*
    **	Select team
    */
    const int rc = Select_Team(caption);

    /*
    **	'OK'; break
    */
    if (rc == 0) {
      break;
    }
    /*
     **	'Edit'
     */
    if (rc == 1 && CurTeam) {
      if (CurTeam->Edit()) {
        Changed = true;
      }
      TheScreen().hidden_view().Clear();
      Flag_To_Redraw(true);
      Render();
    } else {
      /*
      **	'New'
      */
      if (rc == 2) {
        /*
        **	Create a new team
        */
        CurTeam = base::NewPooled<TeamType>();
        if (CurTeam) {
          /*
          **	delete it if user cancels
          */
          if (!CurTeam->Edit()) {
            base::DeletePooled(CurTeam);
            CurTeam = nullptr;
          } else {
            Changed = true;
          }
          TheScreen().hidden_view().Clear();
          Flag_To_Redraw(true);
          Render();
        } else {
          /*
          **	Unable to create; issue warning
          */
          WWMessageBox().Process("No more teams available.");
          TheScreen().hidden_view().Clear();
          Flag_To_Redraw(true);
          Render();
        }
      } else {
        /*
        **	'Delete'
        */
        if ((rc == 3) && CurTeam) {
          Detach_This_From_All(CurTeam->As_Target(), true);
          base::DeletePooled(CurTeam);
          // CurTeam->Remove();
          CurTeam = nullptr;
        }
      }
    }
  }
}

/***************************************************************************
 * MapEditClass::Select_Team -- user selects a team from a list            *
 *                                                                         *
 *    Ŀ           *
 *                             Teams                                     *
 *        Ŀ               *
 *         Name     House    Class:Count,Class:Count                 *
 *         Name     House    Class:Count,Class:Count  Ĵ               *
 *         Name     House    Class:Count,Class:Count                  *
 *         Name     House    Class:Count,Class:Count                  *
 *                                                                    *
 *                                                                    *
 *                                                    Ĵ               *
 *                                                                   *
 *                       *
 *                                                                       *
 *          [Edit]        [New]        [Delete]      [OK]                *
 *                                                                       *
 *               *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, 1 = Edit, 2 = New, 3 = Delete                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      Uses HIDBUFF.                                                      *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/08/1994 BR : Created.                                              *
 *   05/07/1996 JLB : Streamlined and sorted team list.                    *
 *=========================================================================*/
int MapEditClass::Select_Team(const char* /*unused*/) {
  /*
  **	Dialog & button dimensions
  */
  constexpr int kDialogW = 400;  // dialog width
  constexpr int kDialogH = 250;  // dialog height
  constexpr int kDialogX = 0;    // centered x-coord
  constexpr int kDialogY = 0;    // coord // of x-center
  constexpr int kMargin = 25;    // margin width/height
  constexpr int kListW = (kDialogW - (kMargin * 2)) - 20;
  constexpr int kListX = kDialogX + ((kDialogW - kListW) / 2);
  constexpr int kListY = kDialogY + 20;
  constexpr int kListH = (kDialogH - 50) - kListY;
  constexpr int kButtonW = 45;
  constexpr int kButtonH = 9;
  constexpr int kEditW = kButtonW;
  constexpr int kEditH = kButtonH;
  constexpr int kEditX = kDialogX + kDialogW - (((kEditW + 10) * 4) + 25);
  constexpr int kEditY = kDialogY + kDialogH - 20 - kEditH;
  constexpr int kNewW = kButtonW;
  constexpr int kNewH = kButtonH;
  constexpr int kNewX = kEditX + kEditW + 10;
  constexpr int kNewY = kDialogY + kDialogH - 20 - kNewH;
  constexpr int kDeleteW = kButtonW;
  constexpr int kDeleteH = kButtonH;
  constexpr int kDeleteX = kNewX + kNewW + 10;
  constexpr int kDeleteY = kDialogY + kDialogH - 20 - kDeleteH;
  constexpr int kOkW = kButtonW;
  constexpr int kOkH = kButtonH;
  constexpr int kOkX = kDeleteX + kDeleteW + 10;
  constexpr int kOkY = kDialogY + kDialogH - 20 - kOkH;

  /*
  **	Button enumerations:
  */
  constexpr int kTeamList = 100;
  constexpr int kButtonEdit = 101;
  constexpr int kButtonNew = 102;
  constexpr int kButtonDelete = 103;
  constexpr int kButtonOk = 104;

  /*
  **	Dialog variables
  */
  bool edit_team = false;                 // true = user wants to edit
  bool new_team = false;                  // true = user wants to new
  bool del_team = false;                  // true = user wants to new
  static const int tabs[] = {35, 60, 80, 100};  // list box tab stops

  /*
  **	Buttons
  */
  game::GadgetClass* commands = nullptr;  // the button list

  TListClass<TeamType*> teamlist(kTeamList, kListX, kListY, kListW, kListH,
                                 FontType::kEditor, TPF_NOSHADOW,
                                 MixArchive::RetrieveData("EBTN-UP.SHP"),
                                 MixArchive::RetrieveData("EBTN-DN.SHP"));

  TextButtonClass editbtn(kButtonEdit, "Edit", kEButtonFont, kEButtonFlags,
                          kEditX, kEditY, kEditW);
  TextButtonClass newbtn(kButtonNew, "New", kEButtonFont, kEButtonFlags, kNewX,
                         kNewY, kNewW);
  TextButtonClass deletebtn(kButtonDelete, "Delete", kEButtonFont,
                            kEButtonFlags, kDeleteX, kDeleteY, kDeleteW);
  TextButtonClass okbtn(kButtonOk, "OK", kEButtonFont, kEButtonFlags, kOkX,
                        kOkY, kOkW);

  /*
  **	Initialize
  */
  PixelView& view = TheScreen().visible_view();

  /*
  **	Fill in team names
  */
  for (int index = 0; index < TheObjectHeaps().team_type().Count(); index++) {
    teamlist.Add_Item(TheObjectHeaps().team_type().ActiveAt(index));
  }

  teamlist.Sort([](const TeamType* left, const TeamType* right) {
    return std::ranges::lexicographical_compare(
        std::string_view(left->Name()), std::string_view(right->Name()), {},
        absl::ascii_tolower, absl::ascii_tolower);
  });

  if (!CurTeam || !CurTeam->is_active()) {
    CurTeam = nullptr;
  }

  if (CurTeam) {
    teamlist.Set_Selected_Index(CurTeam);
    CurTeam = teamlist.Current_Item();
  } else {
    teamlist.Set_Selected_Index(0);
    if (TheObjectHeaps().team_type().Count()) {
      CurTeam = teamlist.Current_Item();
    }
  }

  /*
  **	Create the list
  */
  commands = &teamlist;
  editbtn.Add_Tail(*commands);
  newbtn.Add_Tail(*commands);
  deletebtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);

  /*
  **	Init tab stops for list
  */
  teamlist.Set_Tabs(tabs);

  /*
  **	Main Processing Loop
  */
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback
    */
    ServiceRealTime();

    /*
    **	Refresh display if needed
    */
    if (display) {
      game::TheMouse().Hide();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(view, TXT_TEAM_EDIT, kDialogX, kDialogY, kDialogW);
      commands->Draw_All(view);
      game::TheMouse().Show();
      display = false;
    }

    /*
    **	Get user input
    */
    const engine::window::KeyNumber input = commands->Input(view).key;

    /*
    **	Process input
    */
    switch (static_cast<int>(input)) {
      case engine::window::ButtonKey(kTeamList):
        CurTeam = teamlist.Current_Item();
        break;

      case engine::window::ButtonKey(kButtonEdit):
        if (teamlist.Count()) {
          process = false;
          edit_team = true;
        }
        break;

      case engine::window::ButtonKey(kButtonNew):
        process = false;
        new_team = true;
        break;

      case engine::window::ButtonKey(kButtonDelete):
        process = false;
        del_team = true;
        break;

      case KN_RETURN:
      case engine::window::ButtonKey(kButtonOk):
        process = false;
        break;
      default:
        break;
    }
  }

  /*
  **	Redraw the display
  */
  TheScreen().hidden_view().Clear();
  Flag_To_Redraw(true);
  Render();

  if (edit_team) {
    return 1;
  }
  if (new_team) {
    return 2;
  }
  if (del_team) {
    return 3;
  }
  return 0;
}

/***************************************************************************
 * MapEditClass::Team_Members -- user picks makeup of a team               *
 *                                                                         *
 * Team members are rendered in a 24 x 24 area; the Window coordinates     *
 * have to be set to this area when the object's 'Display()' routine is    *
 * called. Thus, the dialog's window coords have to be divisible by        *
 * 24. The height of the dialog is computed based on how many objects      *
 * there are in it.                                                        *
 *                                                                         *
 * 10 pixels are left between rows of objects, so the # of that type of    *
 * object can be displayed underneath the object.                          *
 *                                                                         *
 *  Ŀ                    *
 *                   Team Members                                        *
 *                                                                       *
 *    Ŀ                      *
 *                                                           *
 *    Ĵ                      *
 *                                                           *
 *    Ĵ                      *
 *                                                           *
 *    Ĵ                      *
 *                                                           *
 *    Ĵ                      *
 *                                                           *
 *    Ĵ                      *
 *                                                           *
 *                          *
 *                 [OK]      [Cancel]                                    *
 *                      *
 *                                                                         *
 * INPUT:                                                                  *
 *      house      house to display objects for                            *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = cancel                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *      CurTeam must NOT be NULL when this function is called.             *
 *      This routine uses HIDBUFF for data storage.                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *=========================================================================*/
// #define TEENSY_WEENSY
/*
**	Dialog & button dimensions
*/
constexpr int kDialogW = 640;
constexpr int kDialogX = 0;
constexpr int kDialogCx = kDialogX + (kDialogW / 2);

constexpr int kTxt6H = 7;
constexpr int kMargin = 7;

#ifdef TEENSY_WEENSY
constexpr int kPictureW = 32;
constexpr int kPictureH = 24;
#else
constexpr int kPictureW = 64;
constexpr int kPictureH = 48;
#endif
constexpr int kRowH = (kPictureH + 3);

constexpr int kOkW = 50;
constexpr int kOkH = 9;
constexpr int kOkX = kDialogCx - 5 - kOkW;
constexpr int kOkY = 0;

constexpr int kCancelW = 50;
constexpr int kCancelH = 9;
constexpr int kCancelX = kDialogCx + 5;
constexpr int kCancelY = 0;

int MapEditClass::Team_Members(HousesType house) {
  /*
  **	Button enumerations:
  */
  constexpr int kButtonOk = 100;
  constexpr int kButtonCancel = 101;

  /*
  **	Redraw values: in order from "top" to "bottom" layer of the dialog
  **	(highest enum is the lowest layer). Each section of the map checks
  **	the requested redraw level to see if it's supposed to draw; if it's
  **	>= its level, it redraws.
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
  bool cancel = false;  // true = user cancels
  RemapControlType* scheme = Get_Color_Scheme();

  /*
  **	Team display variables
  */
  const TechnoTypeClass* teamclass[kMaxTeamClasses] = {};  // team classes
  int teamcount[kMaxTeamClasses] = {};                     // class counts

  /*
  **	Dialog dimensions.
  */

  int curclass = -1;  // current index into 'teamclass'; can be invalid!
                      // (is based on current mouse position)

  /*
  **	Values for timing when mouse held down.
  */
  int lheld = 0;
  int rheld = 0;
  const int32_t tdelay[3] = {5, 20, 0};
  int tindex = 0;
  int64_t heldtime = 0;

  /*
  **	Buttons.
  */

  TextButtonClass okbtn(kButtonOk, TXT_OK, FontType::kEditor,
                        TPF_CENTER | TPF_NOSHADOW, kOkX, kOkY, kOkW, kOkH);
  TextButtonClass cancelbtn(kButtonCancel, TXT_CANCEL, FontType::kEditor,
                            TPF_CENTER | TPF_NOSHADOW, kCancelX, kCancelY,
                            kCancelW, kCancelH);

  /*
  **	Fill in the ObjectTypeClass array with all available object type ptrs,
  **	checking to be sure this house can own the object
  */
  int i = 0;
  for (const InfantryType i_id : magic_enum::enum_values<InfantryType>()) {
    base::At(teamclass, i) = &InfantryTypeClass::As_Reference(i_id);
    i++;
  }

  for (const AircraftType a_id : magic_enum::enum_values<AircraftType>()) {
    base::At(teamclass, i) = &AircraftTypeClass::As_Reference(a_id);
    i++;
  }

  for (const UnitType u_id : magic_enum::enum_values<UnitType>()) {
    base::At(teamclass, i) = &UnitTypeClass::As_Reference(u_id);
    i++;
  }

  for (const VesselType v_id : magic_enum::enum_values<VesselType>()) {
    base::At(teamclass, i) = &VesselTypeClass::As_Reference(v_id);
    i++;
  }

  /*
  **	Save max # classes.
  */
  const int maxclasses = i;  // max # classes available

  /*
  **	Fill in the 'count' array with data from the current team:
  **	- For every class in the current team, find that class type in the
  **	  'teamclass' array & set its count value
  */
  for (int j = 0; j < maxclasses; j++) {
    base::At(teamcount, j) = 0;
  }

  /*
  **	Loop through all classes in the team.
  */
  for (i = 0; i < CurTeam->member_count(); i++) {
    /*
    **	Find this class in our array.
    */
    for (int j = 0; j < maxclasses; j++) {
      /*
      **	Set the count; detect a match between the team's class & the
      **	'teamclass' array entry by comparing the actual pointers; typeid
      **	won't work because E1 & E2 are the same type class.
      */
      if (base::At(CurTeam->members(), i).type == base::At(teamclass, j)) {
        base::At(teamcount, j) = base::At(CurTeam->members(), i).quantity;
        break;
      }
    }
  }
  int numclasses =
      CurTeam
          ->member_count();  // current # classes in the team (limited to <=5)

  /*
  **	Set up the dialog dimensions based on number of classes we have to draw
  **
  **	Compute picture rows & cols.
  */
  const int numcols =
      (kDialogW - 16) / kPictureW;  // # units displayed horizontally

  /*
  **	Dialog's height = top margin + label + picture rows + margin + label +
  *margin + btn
  */
  const int dlg_h = 400;  // dialog height
  const int dlg_y = 0;
  const int msg_y = dlg_y + dlg_h - 26 - 15;  // y-coord for object names

  okbtn.Y = dlg_y + dlg_h - kMargin - kOkH - 15;
  cancelbtn.Y = dlg_y + dlg_h - kMargin - kCancelH - 15;

  /*
  **	Draw to visible_view.
  */
  PixelView& view = TheScreen().visible_view();

  /*
  **	Make sure 'house' is valid.
  */
  //	if (house!=HOUSE_GOOD && house!=HOUSE_BAD && house != HOUSE_MULTI1 &&
  //		house != HOUSE_MULTI2 && house != HOUSE_MULTI3 && house !=
  // HOUSE_MULTI4 ) { 		if (Scen.ScenPlayer == SCEN_PLAYER_MPLAYER) {
  // house = HOUSE_MULTI1; 		} else { 			house =
  // HOUSE_GOOD;
  //		}
  //	}

  /*
  **	Create the list.
  */
  game::ControlClass* commands = &okbtn;
  cancelbtn.Add_Tail(*commands);

  /*
  **	Main Processing Loop.
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    **	Invoke game callback.
    */
    ServiceRealTime();

    /*
    **	Refresh display if needed.
    */
    if (display != REDRAW_NONE) {
      /*
      **	Display the dialog box.
      */
      game::TheMouse().Hide();
      if (display >= REDRAW_BACKGROUND) {
        /*
        **	Display the constant background of this dialog.
        */
        Dialog_Box(kDialogX, dlg_y, kDialogW, dlg_h);
        Draw_Caption(view, TXT_TEAM_MEMBERS, kDialogX, dlg_y, kDialogW);

        /*
        **	Draw the objects.
        */
        for (i = 0; i < maxclasses; i++) {
          /*
          **	Display the object along with any count value for it.
          */
          Draw_Member(view, base::At(teamclass, i), i, base::At(teamcount, i),
                      house);
        }

        if (static_cast<unsigned>(curclass) < static_cast<unsigned>(maxclasses)) {
          Fancy_Text_Print(view, base::At(teamclass, curclass)->Full_Name(),
                           kDialogX + (kDialogW / 2), msg_y,
                           &ThePalettes().color_remaps().at(PCOLOR_BROWN),
                           kTBlack, FontType::kEditor,
                           TPF_CENTER | TPF_NOSHADOW);
        }
      }

      /*
      **	Redraw the buttons.
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All(view);
      }
      game::TheMouse().Show();
      display = REDRAW_NONE;
    }

    /*
    **	Get user input.
    */
    const engine::window::InputEvent input_event = commands->Input(view);
    const engine::window::KeyNumber input = input_event.key;

    // The mouse buttons set or clear the 'held' values that step the member
    // count while a button is down.
    using engine::window::MouseButton;
    if (input_event.IsPress(MouseButton::kLeft) ||
        input_event.IsPress(MouseButton::kRight)) {
      if (curclass >= 0 && curclass < maxclasses) {
        (input_event.button == MouseButton::kLeft ? lheld : rheld) = 1;
        tindex = 2;
        heldtime = 0;
      }
    } else if (input_event.IsRelease(MouseButton::kLeft)) {
      lheld = 0;
    } else if (input_event.IsRelease(MouseButton::kRight)) {
      rheld = 0;
    }

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
      /*
      **	OK: save values & return.
      */
      case engine::window::ButtonKey(kButtonOk):
        process = false;
        break;

      /*
      **	Cancel: abort & return.
      */
      case engine::window::ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      default:
        /*
        **	Compute new 'curclass' based on mouse position.
        */
        i = ((game::TheMouse().x() - 32 - kDialogX) / kPictureW) +
            (((game::TheMouse().y() - (dlg_y + 8 + 11)) / kRowH) * numcols);

        /*
        **	If it's changed, update class label.
        */
        if (i != curclass) {
          curclass = i;

          /*
          **	Clear out the previously printed name of the item.
          */
          game::TheMouse().Hide();
          view.FillRect(kDialogX + 32, msg_y, kDialogX + kDialogW - 64,
                        msg_y + kTxt6H, kBlack);

          if (static_cast<unsigned>(curclass) < static_cast<unsigned>(maxclasses)) {
            Fancy_Text_Print(view, base::At(teamclass, curclass)->Full_Name(),
                             kDialogX + (kDialogW / 2), msg_y, scheme, kTBlack,
                             FontType::kEditor, TPF_CENTER | TPF_NOSHADOW);
          }

          /*
          **	Force buttons to not be held.
          */
          lheld = 0;
          rheld = 0;
          game::TheMouse().Show();
        }
        break;
    }

    /*
    **	Check for a 'held' mouse button; if it's down, and the correct
    **	amount of time has gone by, increment/decrement the count for the
    **	current class.
    */
    if (lheld) {
      /*
      **	The first time in, the elapsed time will be larger than
      **	tdelay[2], so we increment the count immediately; then, we
      *decrement *	tindex to go to the next time delay, which is longer;
      *then, decr. *	again to go to the 1st time delay which is the shortest.
      */
      if (engine::platform::SystemTicks() - heldtime >
          base::At(tdelay, tindex)) {
        heldtime = engine::platform::SystemTicks();
        if (tindex) {
          tindex--;
        }

        /*
        **	Detect addition of a new class.
        */
        if (base::At(teamcount, curclass) == 0) {
          /*
          **	Don't allow more classes than we can handle.
          */
          if (numclasses == TeamType::kMaxMemberTypes) {
            continue;
          }
          numclasses++;
        }
        base::At(teamcount, curclass)++;

        /*
        **	Update number label.
        */
        Draw_Member(view, base::At(teamclass, curclass), curclass,
                    base::At(teamcount, curclass), house);
      }

    } else {
      /*
      **	The first time in, the elapsed time will be larger than
      **	tdelay[2], so we increment the count immediately; then, we
      *decrement *	tindex to go to the next time delay, which is longer;
      *then, decr. *	again to go to the 1st time delay which is the shortest.
      */
      if (rheld && (engine::platform::SystemTicks() - heldtime >
                    base::At(tdelay, tindex))) {
        if (tindex) {
          tindex--;
        }
        heldtime = engine::platform::SystemTicks();

        if (base::At(teamcount, curclass) > 0) {
          base::At(teamcount, curclass)--;

          /*
          **	Detect removal of a class.
          */
          if (base::At(teamcount, curclass) == 0) {
            numclasses--;
          }
        }

        /*
        **	Update number label.
        */
        Draw_Member(view, base::At(teamclass, curclass), curclass,
                    base::At(teamcount, curclass), house);
      }
    }
  }

  /*
  **	Copy data into team.
  */
  if (!cancel) {
    CurTeam->ClearMembers();
    for (int j = 0; j < maxclasses; j++) {
      if (base::At(teamcount, j) > 0) {
        CurTeam->AddMember({.quantity = base::At(teamcount, j),
                            .type = base::At(teamclass, j)});
      }
    }
  }

  /*
  **	Redraw the display.
  */
  TheScreen().hidden_view().Clear();
  Flag_To_Redraw(true);
  Render();

  if (cancel) {
    return (-1);
  }
  return 0;
}

/***********************************************************************************************
 * MapEditClass::Draw_Member -- Draws a member of the team dialog box. *
 *                                                                                             *
 *    This routine will display the cameo image of the potential team member. In
 *the corner,   * it will show the current quantity of this member for the
 *current team being edited.      *
 *                                                                                             *
 * INPUT:   ptr   -- Pointer to the member object type. *
 *                                                                                             *
 *          index -- The index into the team dialog box array of selectable
 *objects. This is   * used to determine the correct X and Y offsets to draw. *
 *                                                                                             *
 *          quant -- The quantity number to display in the corner of the image.
 **
 *                                                                                             *
 *          house -- The owner of this object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/02/1995 JLB : Created. *
 *=============================================================================================*/
void MapEditClass::Draw_Member(PixelView& view, const TechnoTypeClass* ptr,
                               int index, int quant, HousesType house) {
  const int numcols = (kDialogW - 64) / kPictureW;
  const int col = index % numcols;
  const int row = index / numcols;
  const int dlg_y = 0;
  const int x = kDialogX + 32 + (col * kPictureW);
  const int y = dlg_y + 8 + 13 + (row * kRowH);
  RemapControlType* scheme = Get_Color_Scheme();

  /*
  **	Change the window to this box.
  */
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowX) = x;
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowY) = y;
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowWidth) =
      kPictureW;
  base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowHeight) =
      kPictureH;

  game::TheMouse().Hide();
  Draw_Box(view, x, y, kPictureW, kPictureH, BOXSTYLE_DOWN, true);
  ptr->Display(view, Screen::kWidth / 2, Screen::kHeight / 2, WINDOW_EDITOR,
               house);
  if (quant > 0) {
    Fancy_Text_Print(view, "%d", x + 1, y + 1, scheme, kTBlack,
                     FontType::k8Point, TPF_DROPSHADOW, quant);
    //		Fancy_Text_Print("%d", x+1, y+kPictureH-8, scheme, TBLACK,
    // TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_DROPSHADOW, quant);
  }
  game::TheMouse().Show();
}

}  // namespace ra
