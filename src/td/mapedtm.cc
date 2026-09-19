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

/* $Header:   F:\projects\c&c\vcs\code\mapedtm.cpv   2.18   16 Oct 1995 16:52:16
 * JOE_BOSTIC  $ */
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
 *                  Last Update : April 9, 1996 [BRR]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Handle_Teams -- main team-dialog-handling function      *
 *   MapEditClass::Select_Team -- user selects a team from a list          *
 *   MapEditClass::Edit_Team -- user edits a team's options                *
 *   MapEditClass::Team_Members -- user picks makeup of a team             *
 *   MapEditClass::Build_Mission_list -- fills in mission list box         *
 *   MapEditClass::Draw_Member -- Draws a member of the team dialog box.   *
 *   MapEditClass::Team_Members -- Team members dialog                     *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

#include "absl/strings/str_format.h"
#include "base/array.h"
#include "base/numeric.h"
#include "port/safe_string.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/timer.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/edit.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/heap.h"
#include "td/jshell.h"
#include "td/list.h"
#include "td/mapedit.h"
#include "td/msgbox.h"
#include "td/teamtype.h"
#include "td/textbtn.h"
#include "td/type.h"
#include "tech/number_parse.h"

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

  /*------------------------------------------------------------------------
  Team dialog processing loop:
  - Invoke the team selection dialog. If a team's selected, break
    & return
  - If user wants to edit the current team, do so
  - If user wants to create new team, new a TeamTypeClass & edit it
  - If user wants to delete team, delete the current team
  - Keep looping until 'OK'
  ------------------------------------------------------------------------*/
  for (;;) {
    /*
    ............................. Select team .............................
    */
    const int rc = Select_Team(caption);

    /*
    ............................. 'OK'; break .............................
    */
    if (rc == 0) {
      break;
    }
    /*
          ............................... 'Edit'
       ................................
          */
    if (rc == 1 && CurTeam) {
      if (Edit_Team() == 0) {
        Changed = true;
      }
    } else {
      /*
      ................................ 'New' ................................
      */
      if (rc == 2) {
        /*
        ........................ Create a new team .........................
        */
        CurTeam = new TeamTypeClass();
        if (CurTeam) {
          /*
          ................... delete it if user cancels ...................
          */
          if (Edit_Team() == -1) {
            delete CurTeam;
            CurTeam = nullptr;
          } else {
            Changed = true;
          }
        } else {
          /*
          ................. Unable to create; issue warning ..................
          */
          CCMessageBox().Process("No more teams available.");
          HiddenPage.Clear();
          Flag_To_Redraw(true);
          Render();
        }
      } else {
        /*
        .............................. 'Delete'
        ...............................
        */
        if ((rc == 3) && CurTeam) {
          CurTeam->Remove();
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
 *=========================================================================*/
int MapEditClass::Select_Team(const char* caption) {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 528;                         // dialog width
  constexpr int kDialogH = 290;                         // dialog height
  constexpr int kDialogX = ((640 - kDialogW) / 2);      // centered x-coord
  constexpr int kDialogY = ((400 - kDialogH) / 2);      // centered y-coord
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);  // coord of x-center

  constexpr int kTxt8H = 22;   // ht of 8-pt text
  constexpr int kMargin = 14;  // margin width/height

  constexpr int kListW = 500;
  constexpr int kListH = 208;
  constexpr int kListX = kDialogX + kMargin;
  constexpr int kListY = kDialogY + kMargin + kTxt8H;

  constexpr int kEditW = 90;
  constexpr int kEditH = 18;
  constexpr int kEditX = kDialogX + (kDialogW / 8) - (kEditW / 2);
  constexpr int kEditY = kDialogY + kDialogH - kMargin - kEditH;

  constexpr int kNewW = 90;
  constexpr int kNewH = 18;
  constexpr int kNewX = kDialogX + ((kDialogW / 8) * 3) - (kNewW / 2);
  constexpr int kNewY = kDialogY + kDialogH - kMargin - kNewH;

  constexpr int kDeleteW = 90;
  constexpr int kDeleteH = 18;
  constexpr int kDeleteX = kDialogX + ((kDialogW / 8) * 5) - (kDeleteW / 2);
  constexpr int kDeleteY = kDialogY + kDialogH - kMargin - kDeleteH;

  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogX + ((kDialogW / 8) * 7) - (kOkW / 2);
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;

  constexpr int kTeamtxtLen = 43;  // max length of a team entry

  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kTeamList = 100;
  constexpr int kButtonEdit = 101;
  constexpr int kButtonNew = 102;
  constexpr int kButtonDelete = 103;
  constexpr int kButtonOk = 104;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables
  ........................................................................*/
  char* teamtext[kTeamTypeMax + 1] = {};  // text for defined teams
  bool edit_team = false;            // true = user wants to edit
  bool new_team = false;             // true = user wants to new
  bool del_team = false;             // true = user wants to new
  static const int tabs[] = {120, 180};  // list box tab stops
  char txt[10];
  //	int housetxt;

  /*........................................................................
  Buttons
  ........................................................................*/
  GadgetClass* commands = nullptr;  // the button list

  ListClass teamlist(kTeamList, kListX, kListY, kListW, kListH,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                     Hires_Retrieve("BTN-UP.SHP"),
                     Hires_Retrieve("BTN-DN.SHP"));

  TextButtonClass editbtn(
      kButtonEdit, "Edit",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kEditX,
      kEditY, kEditW, kEditH);

  TextButtonClass newbtn(
      kButtonNew, "New",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNewX, kNewY,
      kNewW, kNewH);

  TextButtonClass deletebtn(
      kButtonDelete, "Delete",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kDeleteX,
      kDeleteY, kDeleteW, kDeleteH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*
  ........................... Fill in team names ...........................
  */
  int def_idx = 0;  // default list index
  for (int i = 0; i < TeamTypes.Count(); i++) {
    /*
    ................... Generate string for this team .....................
    */
    // teamtext[i] = (char *)HidPage.Get_Graphic_Buffer()->Get_Buffer() +
    // TEAMTXT_LEN * i;
    constexpr int kTeamNameSize = 255;
    base::At(teamtext, i) = new char[kTeamNameSize];
    // This entry was allocated immediately above with exactly kTeamNameSize
    // characters.
    // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
    const std::span<char> team_name(base::At(teamtext, i), kTeamNameSize);

    /*
    ........................ Fill in name & house .........................
    */
    port::SafeCopy(team_name, TeamTypes.Ptr(i)->IniName);
    port::SafeAppend(team_name, "\t");
    port::SafeAppend(
        team_name,
        HouseTypeClass::As_Reference(TeamTypes.Ptr(i)->House).Suffix);
    port::SafeAppend(team_name, "\t");

    /*
    ................ Fill in class & count for all classes ................
    */
    for (int j = 0; std::cmp_less(j, TeamTypes.Ptr(i)->ClassCount); j++) {
      absl::SNPrintF(txt, sizeof(txt), "%s:%d",
                     base::At(TeamTypes.Ptr(i)->Class, j)->IniName,
                     base::At(TeamTypes.Ptr(i)->DesiredNum, j));

      /*..................................................................
      Add entry if there's room; break otherwise
      (+ 3 for the ", " and the NULL; +3 again for the "..." for the next
      entry)
      ..................................................................*/
      if (std::string_view(txt).size() +
              std::string_view(base::At(teamtext, i)).size() + 6 <
          kTeamtxtLen) {
        if (j > 0) {
          port::SafeAppend(team_name, ", ");
        }
        port::SafeAppend(team_name, txt);
      } else {
        port::SafeAppend(team_name, "...");
        break;
      }
    }

    /*
    .................. Set def_idx if this is CurTeam .....................
    */
    if (TeamTypes.Ptr(i) == CurTeam) {
      def_idx = i;
    }

    /*
    ........................... Add to list box ...........................
    */
    teamlist.Add_Item(base::At(teamtext, i));
  }

  /*
  ....................... Set CurTeam if it isn't ..........................
  */
  if (TeamTypes.Count() == 0) {
    CurTeam = nullptr;
  } else {
    if (!CurTeam) {
      CurTeam = TeamTypes.Ptr(def_idx);
    }
  }

  /*
  ............................ Create the list .............................
  */
  commands = &teamlist;
  editbtn.Add_Tail(*commands);
  newbtn.Add_Tail(*commands);
  deletebtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);

  /*
  ------------------------ Init tab stops for list -------------------------
  */
  teamlist.Set_Tabs(tabs);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);

        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            caption, kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kTeamList):
        def_idx = teamlist.Current_Index();
        if (def_idx < TeamTypes.Count()) {
          CurTeam = TeamTypes.Ptr(def_idx);
        }
        break;

      case ButtonKey(kButtonEdit):
        if (CurTeam) {  // only allow if there's one selected
          process = false;
          edit_team = true;
        }
        break;

      case ButtonKey(kButtonNew):
        process = false;
        new_team = true;
        break;

      case ButtonKey(kButtonDelete):
        process = false;
        del_team = true;
        break;

      case KN_RETURN:
      case ButtonKey(kButtonOk):
        process = false;
        break;
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  for (int i = 0; i < TeamTypes.Count(); i++) {
    delete[] base::At(teamtext, i);
  }
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
 * MapEditClass::Edit_Team -- user edits a team's options                  *
 *                                                                         *
 *  Ŀ      *
 *                           Team Editor                                 *
 *                                                                       *
 *              Name ______               [Roundabout]                   *
 *          Priority ______  [   GDI   ]  [Learning  ]                   *
 *           Max Num ______  [   NOD   ]  [Suicide   ]                   *
 *          Init Num ______               [Autocreate]                   *
 *              Fear ______               [Mercenary ]                   *
 *                                        [Prebuild  ]                   *
 *                                        [Reinforce ]                   *
 *                                                                       *
 *    Ŀ             Ŀ        *
 *                       ^                                ^        *
 *                       Ĵ  [Add >>]                      Ĵ        *
 *                          [Insert]                               *
 *                          [Delete]                               *
 *                       Ĵ    ____                        Ĵ        *
 *                       v                                v        *
 *                         *
 *                                                                       *
 *          [Members]          [Cancel]           [OK]                   *
 *                                                                       *
 *        *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = cancel                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *      CurTeam must NOT be NULL when this function is called.             *
 *      This routine invokes the Members dialog, which uses HIDBUFF.       *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/08/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Edit_Team() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 516;
  constexpr int kDialogH = 376;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  constexpr int kDialogCx = kDialogX + (kDialogW / 2);

  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;

  constexpr int kNameW = 120;
  constexpr int kNameH = 18;
  constexpr int kNameX = kDialogX + kMargin + 100;
  constexpr int kNameY = kDialogY + kMargin + kTxt8H;

  constexpr int kPriorityW = 120;
  constexpr int kPriorityH = 18;
  constexpr int kPriorityX = kDialogX + kMargin + 100;
  constexpr int kPriorityY = kNameY + kNameH;

  constexpr int kMaxnumW = 120;
  constexpr int kMaxnumH = 18;
  constexpr int kMaxnumX = kDialogX + kMargin + 100;
  constexpr int kMaxnumY = kPriorityY + kPriorityH;

  constexpr int kInitnumW = 120;
  constexpr int kInitnumH = 18;
  constexpr int kInitnumX = kDialogX + kMargin + 100;
  constexpr int kInitnumY = kMaxnumY + kMaxnumH;

  constexpr int kFearW = 120;
  constexpr int kFearH = 18;
  constexpr int kFearX = kDialogX + kMargin + 100;
  constexpr int kFearY = kInitnumY + kInitnumH;

  constexpr int kGdiW = 100;
  constexpr int kGdiH = 18;
  constexpr int kGdiX = kNameX + kNameW + kMargin;
  constexpr int kGdiY = kNameY + kNameH + (kNameH / 2);

  constexpr int kNodW = 100;
  constexpr int kNodH = 18;
  constexpr int kNodX = kNameX + kNameW + kMargin;
  constexpr int kNodY = kGdiY + kGdiH;

  constexpr int kNeuW = 100;
  constexpr int kNeuH = 18;
  constexpr int kNeuX = kNameX + kNameW + kMargin;
  constexpr int kNeuY = kNodY + kNodH;

  constexpr int kMulti1W = 50;
  constexpr int kMulti1H = 18;
  constexpr int kMulti1X = kGdiX;
  constexpr int kMulti1Y = kGdiY;

  constexpr int kMulti2W = 50;
  constexpr int kMulti2H = 18;
  constexpr int kMulti2X = kGdiX + kMulti2W;
  constexpr int kMulti2Y = kGdiY;

  constexpr int kMulti3W = 50;
  constexpr int kMulti3H = 18;
  constexpr int kMulti3X = kNodX;
  constexpr int kMulti3Y = kNodY;

  constexpr int kMulti4W = 50;
  constexpr int kMulti4H = 18;
  constexpr int kMulti4X = kNodX + kMulti4W;
  constexpr int kMulti4Y = kNodY;

  constexpr int kRoundaboutW = 130;
  constexpr int kRoundaboutH = 18;
  constexpr int kRoundaboutX = kDialogX + kDialogW - kMargin - kRoundaboutW;
  constexpr int kRoundaboutY = kDialogY + kMargin + kTxt8H - 10;

  constexpr int kLearningW = kRoundaboutW;
  constexpr int kLearningH = 18;
  constexpr int kLearningX = kRoundaboutX;
  constexpr int kLearningY = kRoundaboutY + kRoundaboutH;

  constexpr int kSuicideW = kRoundaboutW;
  constexpr int kSuicideH = 18;
  constexpr int kSuicideX = kRoundaboutX;
  constexpr int kSuicideY = kLearningY + kLearningH;

  constexpr int kAutocreateW = kRoundaboutW;
  constexpr int kAutocreateH = 18;
  constexpr int kAutocreateX = kRoundaboutX;
  constexpr int kAutocreateY = kSuicideY + kSuicideH;

  constexpr int kMercenaryW = kRoundaboutW;
  constexpr int kMercenaryH = 18;
  constexpr int kMercenaryX = kRoundaboutX;
  constexpr int kMercenaryY = kAutocreateY + kAutocreateH;

  constexpr int kPrebuiltW = kRoundaboutW;
  constexpr int kPrebuiltH = 18;
  constexpr int kPrebuiltX = kRoundaboutX;
  constexpr int kPrebuiltY = kMercenaryY + kMercenaryH;

  constexpr int kReinforceW = kRoundaboutW;
  constexpr int kReinforceH = 18;
  constexpr int kReinforceX = kRoundaboutX;
  constexpr int kReinforceY = kPrebuiltY + kPrebuiltH;

  constexpr int kMission1W = 180;
  constexpr int kMission1H = 128;
  constexpr int kMission1X = kDialogX + kMargin;
  constexpr int kMission1Y = kReinforceY + kReinforceH + kMargin;

  constexpr int kMission2W = 180;
  constexpr int kMission2H = 128;
  constexpr int kMission2X = kDialogX + kDialogW - kMargin - kMission2W;
  constexpr int kMission2Y = kMission1Y;

  constexpr int kAddW = 100;
  constexpr int kAddH = 18;
  constexpr int kAddX = kMission1X + kMission1W + kMargin;
  constexpr int kAddY = kMission1Y + kAddH;

  constexpr int kInsertW = 100;
  constexpr int kInsertH = 18;
  constexpr int kInsertX = kMission1X + kMission1W + kMargin;
  constexpr int kInsertY = kAddY + kAddH;

  constexpr int kDelW = 100;
  constexpr int kDelH = 18;
  constexpr int kDelX = kMission1X + kMission1W + kMargin;
  constexpr int kDelY = kInsertY + kInsertH;

  constexpr int kArgW = 100;
  constexpr int kArgH = 18;
  constexpr int kArgX = kMission1X + kMission1W + kMargin;
  constexpr int kArgY = kDelY + kDelH;

  constexpr int kMembersW = 100;
  constexpr int kMembersH = 18;
  constexpr int kMembersX = kDialogX + (kDialogW / 6) - (kMembersW / 2);
  constexpr int kMembersY = kDialogY + kDialogH - kMargin - kMembersH;

  constexpr int kCancelW = 100;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kDialogX + ((kDialogW / 6) * 3) - (kCancelW / 2);
  constexpr int kCancelY = kDialogY + kDialogH - kMargin - kCancelH;

  constexpr int kOkW = 100;
  constexpr int kOkH = 18;
  constexpr int kOkX = kDialogX + ((kDialogW / 6) * 5) - (kOkW / 2);
  constexpr int kOkY = kDialogY + kDialogH - kMargin - kOkH;

  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kButtonName = 100;
  constexpr int kButtonRecruit = 101;
  constexpr int kButtonMaxnum = 102;
  constexpr int kButtonInitnum = 103;
  constexpr int kButtonFear = 104;
  constexpr int kButtonGdi = 105;
  constexpr int kButtonNod = 106;
  constexpr int kButtonNeu = 107;
  [[maybe_unused]] constexpr int kButtonJp = 108;  // placeholder
  constexpr int kButtonMulti1 = 109;
  constexpr int kButtonMulti2 = 110;
  constexpr int kButtonMulti3 = 111;
  constexpr int kButtonMulti4 = 112;
  [[maybe_unused]] constexpr int kButtonMulti5 = 113;
  [[maybe_unused]] constexpr int kButtonMulti6 = 114;
  constexpr int kButtonRoundabout = 115;
  constexpr int kButtonLearning = 116;
  constexpr int kButtonSuicide = 117;
  constexpr int kButtonAuto = 118;
  constexpr int kButtonMercenary = 119;
  constexpr int kButtonPrebuilt = 120;
  constexpr int kButtonReinforce = 121;
  constexpr int kButtonMission1 = 122;
  constexpr int kButtonMission2 = 123;
  constexpr int kButtonAdd = 124;
  constexpr int kButtonInsert = 125;
  constexpr int kButtonDel = 126;
  constexpr int kButtonArg = 127;
  constexpr int kButtonMembers = 128;
  constexpr int kButtonOk = 129;
  constexpr int kButtonCancel = 130;

  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_BACKGROUND = 2,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;

  /*........................................................................
  Dialog variables:
  ........................................................................*/
  bool cancel = false;  // true = user cancels
  char name_buf[12];
  char recr_buf[4];
  char maxnum_buf[4];
  char initnum_buf[4];
  char fear_buf[4];
  TeamMissionStruct missions[TeamTypeClass::kMaxTeamMissions];
  char missionbuf[TeamTypeClass::kMaxTeamMissions][20];

  char arg_buf[4] = {0};
  static const int tabs[] = {130, 180};  // list box tab stops
  int i = 0;

  /*........................................................................
  Buttons:
  ........................................................................*/
  EditClass name_edt(kButtonName, name_buf, 8,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNameX,
                     kNameY, kNameW, kNameH, EditClass::ALPHANUMERIC);

  EditClass recr_edt(kButtonRecruit, recr_buf, 3,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kPriorityX,
                     kPriorityY, kPriorityW, kPriorityH, EditClass::NUMERIC);

  EditClass maxnum_edt(kButtonMaxnum, maxnum_buf, 3,
                       TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMaxnumX,
                       kMaxnumY, kMaxnumW, kMaxnumH, EditClass::NUMERIC);

  EditClass initnum_edt(kButtonInitnum, initnum_buf, 3,
                        TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
                        kInitnumX, kInitnumY, kInitnumW, kInitnumH,
                        EditClass::NUMERIC);

  EditClass fear_edt(kButtonFear, fear_buf, 3,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kFearX,
                     kFearY, kFearW, kFearH, EditClass::NUMERIC);

  TextButtonClass gdibtn(
      kButtonGdi, "GDI",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdiX, kGdiY,
      kGdiW, kGdiH);

  TextButtonClass nodbtn(
      kButtonNod, "NOD",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodX, kNodY,
      kNodW, kNodH);

  TextButtonClass neubtn(
      kButtonNeu, "NEUTRAL",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNeuX, kNeuY,
      kNeuW, kNeuH);

  const TextButtonClass multi1btn(
      kButtonMulti1, "M1",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti1X,
      kMulti1Y, kMulti1W, kMulti1H);

  const TextButtonClass multi2btn(
      kButtonMulti2, "M2",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti2X,
      kMulti2Y, kMulti2W, kMulti2H);

  const TextButtonClass multi3btn(
      kButtonMulti3, "M3",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti3X,
      kMulti3Y, kMulti3W, kMulti3H);

  const TextButtonClass multi4btn(
      kButtonMulti4, "M4",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti4X,
      kMulti4Y, kMulti4W, kMulti4H);

  TextButtonClass roundbtn(
      kButtonRoundabout, "Roundabout",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kRoundaboutX,
      kRoundaboutY, kRoundaboutW, kRoundaboutH);

  TextButtonClass learnbtn(
      kButtonLearning, "Learning",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kLearningX,
      kLearningY, kLearningW, kLearningH);

  TextButtonClass suicidebtn(
      kButtonSuicide, "Suicide",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kSuicideX,
      kSuicideY, kSuicideW, kSuicideH);

  TextButtonClass autocreatebtn(
      kButtonAuto, "Autocreate",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kAutocreateX,
      kAutocreateY, kAutocreateW, kAutocreateH);

  TextButtonClass mercbtn(
      kButtonMercenary, "Mercenary",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMercenaryX,
      kMercenaryY, kMercenaryW, kMercenaryH);

  TextButtonClass prebuiltbtn(
      kButtonPrebuilt, "Prebuild",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kPrebuiltX,
      kPrebuiltY, kPrebuiltW, kPrebuiltH);

  TextButtonClass reinforcebtn(
      kButtonReinforce, "Reinforce",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kReinforceX,
      kReinforceY, kReinforceW, kReinforceH);

  ListClass missionlist1(
      kButtonMission1, kMission1X, kMission1Y, kMission1W, kMission1H,
      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      Hires_Retrieve("BTN-UP.SHP"), Hires_Retrieve("BTN-DN.SHP"));

  ListClass missionlist2(
      kButtonMission2, kMission2X, kMission2Y, kMission2W, kMission2H,
      TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
      Hires_Retrieve("BTN-UP.SHP"), Hires_Retrieve("BTN-DN.SHP"));

  TextButtonClass addbtn(
      kButtonAdd, "Add >>",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kAddX, kAddY,
      kAddW, kAddH);

  TextButtonClass insertbtn(
      kButtonInsert, "Insert",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kInsertX,
      kInsertY, kInsertW, kInsertH);

  TextButtonClass delbtn(
      kButtonDel, "Delete",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kDelX, kDelY,
      kDelW, kDelH);

  EditClass arg_edt(kButtonArg, arg_buf, 4,
                    TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kArgX,
                    kArgY, kArgW, kArgH, EditClass::ALPHANUMERIC);

  TextButtonClass membersbtn(
      kButtonMembers, "Members",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMembersX,
      kMembersY, kMembersW, kMembersH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);

  /*
  ........................... Copy team's state ............................
  */
  port::SafeCopy(name_buf, CurTeam->IniName);
  absl::SNPrintF(recr_buf, sizeof(recr_buf), "%d", CurTeam->RecruitPriority);
  absl::SNPrintF(maxnum_buf, sizeof(maxnum_buf), "%d", CurTeam->MaxAllowed);
  absl::SNPrintF(initnum_buf, sizeof(initnum_buf), "%d", CurTeam->InitNum);
  absl::SNPrintF(fear_buf, sizeof(fear_buf), "%d", CurTeam->Fear);
  int roundabout = CurTeam->IsRoundAbout;
  int learning = CurTeam->IsLearning;
  int suicide = CurTeam->IsSuicide;
  HousesType house = CurTeam->House;
  int autocreate = CurTeam->IsAutocreate;
  int mercenary = CurTeam->IsMercenary;
  int prebuilt = CurTeam->IsPrebuilt;
  int reinforce = CurTeam->IsReinforcable;

  /*
  ......................... Fill in mission lists ..........................
  */
  for (i = 0; i < static_cast<int>(TMISSION_COUNT); i++) {
    missionlist1.Add_Item(
        TeamTypeClass::Name_From_Mission(static_cast<TeamMissionType>(i)));
  }

  int missioncount = CurTeam->MissionCount;
  for (i = 0; i < missioncount; i++) {
    base::At(missions, i) = base::At(CurTeam->MissionList, i);
  }
  Build_Mission_List(missioncount, missions, missionbuf, &missionlist2);

  int curmission = 0;  // currently-selected mission index
  if (missioncount) {
    if (base::At(missions, curmission).Mission == TMISSION_MOVE ||
        base::At(missions, curmission).Mission == TMISSION_UNLOAD) {
      absl::SNPrintF(arg_buf, sizeof(arg_buf), "%c",
                     base::At(missions, curmission).Argument + 'A');
    } else {
      absl::SNPrintF(arg_buf, sizeof(arg_buf), "%d",
                     base::At(missions, curmission).Argument);
    }
  }
  missionlist2.Set_Tabs(tabs);

  /*
  ......................... Init the button states .........................
  */
  name_edt.Set_Text(name_buf, 8);
  recr_edt.Set_Text(recr_buf, 3);
  maxnum_edt.Set_Text(maxnum_buf, 3);
  initnum_edt.Set_Text(initnum_buf, 3);
  fear_edt.Set_Text(fear_buf, 3);
  arg_edt.Set_Text(arg_buf, 3);

  if (roundabout) {
    roundbtn.Turn_On();
  }
  if (learning) {
    learnbtn.Turn_On();
  }
  if (suicide) {
    suicidebtn.Turn_On();
  }
  if (autocreate) {
    autocreatebtn.Turn_On();
  }
  if (mercenary) {
    mercbtn.Turn_On();
  }
  if (reinforce) {
    reinforcebtn.Turn_On();
  }
  if (prebuilt) {
    prebuiltbtn.Turn_On();
  }

  /*
  ............................ Create the list .............................
  */
  ControlClass* commands = &okbtn;
  cancelbtn.Add_Tail(*commands);
  membersbtn.Add_Tail(*commands);

  name_edt.Add_Tail(*commands);
  recr_edt.Add_Tail(*commands);
  maxnum_edt.Add_Tail(*commands);
  initnum_edt.Add_Tail(*commands);
  fear_edt.Add_Tail(*commands);

  gdibtn.Add_Tail(*commands);
  nodbtn.Add_Tail(*commands);
  neubtn.Add_Tail(*commands);

  roundbtn.Add_Tail(*commands);
  learnbtn.Add_Tail(*commands);
  suicidebtn.Add_Tail(*commands);
  autocreatebtn.Add_Tail(*commands);
  mercbtn.Add_Tail(*commands);
  prebuiltbtn.Add_Tail(*commands);
  reinforcebtn.Add_Tail(*commands);

  missionlist1.Add_Tail(*commands);
  missionlist2.Add_Tail(*commands);
  addbtn.Add_Tail(*commands);
  insertbtn.Add_Tail(*commands);
  delbtn.Add_Tail(*commands);
  arg_edt.Add_Tail(*commands);

  Set_House_Buttons(house, commands, kButtonGdi);

  /*
  -------------------------- Main Processing Loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    ........................ Invoke game callback .........................
    */
    Call_Back();

    /*
    ...................... Refresh display if needed ......................
    */
    if (display != REDRAW_NONE) {
      /*
      ...................... Display the dialog box ......................
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);

        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);
        /*
        ....................... Draw the captions .......................
        */
        Fancy_Text_Print(
            "Team Edit", kDialogCx, kDialogY + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Name", kNameX - 5, kNameY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Priority", kPriorityX - 5, kPriorityY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Max Num", kMaxnumX - 5, kMaxnumY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Init Num", kInitnumX - 5, kInitnumY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        Fancy_Text_Print(
            "Fear", kFearX - 5, kFearY, kCcGreen, kTBlack,
            TPF_RIGHT | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }
      /*
      ........................ Redraw the buttons ........................
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();

    /*
    ............................ Process input ............................
    */
    switch (static_cast<int>(input)) {
      case ButtonKey(kButtonName):
      case ButtonKey(kButtonRecruit):
      case ButtonKey(kButtonMaxnum):
      case ButtonKey(kButtonInitnum):
      case ButtonKey(kButtonFear):
        break;

      /*..................................................................
      Toggle RoundAbout
      ..................................................................*/
      case ButtonKey(kButtonRoundabout):
        if (roundabout) {
          roundabout = 0;
          roundbtn.Turn_Off();
        } else {
          roundabout = 1;
          roundbtn.Turn_On();
        }
        break;

      /*..................................................................
      Toggle Learning
      ..................................................................*/
      case ButtonKey(kButtonLearning):
        if (learning) {
          learning = 0;
          learnbtn.Turn_Off();
        } else {
          learning = 1;
          learnbtn.Turn_On();
        }
        break;

      /*..................................................................
      Toggle Suicide
      ..................................................................*/
      case ButtonKey(kButtonSuicide):
        if (suicide) {
          suicide = 0;
          suicidebtn.Turn_Off();
        } else {
          suicide = 1;
          suicidebtn.Turn_On();
        }
        break;

      /*..................................................................
      Toggle Spy
      ..................................................................*/
      case ButtonKey(kButtonAuto):
        if (autocreate) {
          autocreate = 0;
          autocreatebtn.Turn_Off();
        } else {
          autocreate = 1;
          autocreatebtn.Turn_On();
        }
        break;

      /*..................................................................
      Toggle Mercenary
      ..................................................................*/
      case ButtonKey(kButtonMercenary):
        if (mercenary) {
          mercenary = 0;
          mercbtn.Turn_Off();
        } else {
          mercenary = 1;
          mercbtn.Turn_On();
        }
        break;

      case ButtonKey(kButtonPrebuilt):
        if (prebuilt) {
          prebuilt = 0;
          prebuiltbtn.Turn_Off();
        } else {
          prebuilt = 1;
          prebuiltbtn.Turn_On();
        }
        break;

      case ButtonKey(kButtonReinforce):
        if (reinforce) {
          reinforce = 0;
          reinforcebtn.Turn_Off();
        } else {
          reinforce = 1;
          reinforcebtn.Turn_On();
        }
        break;

      /*..................................................................
      Select a Mission on the left-hand mission list
      ..................................................................*/
      case ButtonKey(kButtonMission1):
        break;

      /*..................................................................
      Select a Mission on the right-hand mission list; update the Argument
      field to reflect the current value
      ..................................................................*/
      case ButtonKey(kButtonMission2):
        if (missionlist2.Count() > 0 &&
            missionlist2.Current_Index() != curmission) {
          curmission = missionlist2.Current_Index();
          if (base::At(missions, curmission).Mission == TMISSION_MOVE ||
              base::At(missions, curmission).Mission == TMISSION_UNLOAD) {
            absl::SNPrintF(arg_buf, sizeof(arg_buf), "%c",
                           base::At(missions, curmission).Argument + 'A');
          } else {
            absl::SNPrintF(arg_buf, sizeof(arg_buf), "%d",
                           base::At(missions, curmission).Argument);
          }
          arg_edt.Set_Text(arg_buf, 3);
        }
        break;

      /*..................................................................
      Copy mission from left list box to right list box
      ..................................................................*/
      case ButtonKey(kButtonAdd):
      case ButtonKey(kButtonInsert):
        if (missioncount < TeamTypeClass::kMaxTeamMissions) {
          /*
          ** Set 'i' to the position we're going to add into; this will
          ** be just AFTER the current item if we're adding, and it will
          ** be the current item if we're inserting.
          */
          if (input == ButtonKey(kButtonAdd)) {
            i = missionlist2.Current_Index() + 1;
            i = std::max(i, 0);
            i = std::min(i, missioncount);
          } else {
            i = missionlist2.Current_Index();
            i = std::max(i, 0);
            if (i >= missioncount && missioncount > 0) {
              i = missioncount - 1;
            }
          }

          /*
          ** Move all other missions forward in the array
          */
          for (int j = missioncount; j > i; j--) {
            base::At(missions, j) = base::At(missions, j - 1);
          }

          /*
          ** Set the Mission value based on 1st list box's index
          */
          base::At(missions, i).Mission = static_cast<TeamMissionType>(
              static_cast<int>(TMISSION_ATTACKBASE) +
              missionlist1.Current_Index());

          /*
          ** Set the missions argument field
          */
          if (base::At(missions, i).Mission == TMISSION_MOVE ||
              base::At(missions, i).Mission == TMISSION_UNLOAD) {
            base::At(missions, i).Argument =
                toupper(base::At(arg_buf, 0)) - 'A';
          } else {
            base::At(missions, i).Argument =
                tech::ParseIntegerOr<int>(arg_buf, 0);
          }
          missioncount++;

          /*
          ** Rebuild the list box from scratch
          */
          Build_Mission_List(missioncount, missions, missionbuf, &missionlist2);

          /*
          ** Update the list's current item index
          */
          missionlist2.Set_Selected_Index(i);
        }
        break;

      /*..................................................................
      Delete mission from right-hand list box
      ..................................................................*/
      case ButtonKey(kButtonDel):
        if (missioncount > 0) {
          i = missionlist2.Current_Index();
          if (i < 0 || i >= missioncount) {
            break;
          }

          /*
          ** Move all missions back in the array
          */
          for (int j = i; j < missioncount - 1; j++) {
            base::At(missions, j) = base::At(missions, j + 1);
          }
          missioncount--;

          /*
          ** Rebuild the list box from scratch
          */
          Build_Mission_List(missioncount, missions, missionbuf, &missionlist2);

          /*
          ** Update the list's current item index
          */
          if (i >= missioncount) {
            i--;
            i = std::max(i, 0);
            missionlist2.Set_Selected_Index(i);
          }
        }
        break;

      /*..................................................................
      Set house
      ..................................................................*/
      case ButtonKey(kButtonGdi):
      case ButtonKey(kButtonNod):
      case ButtonKey(kButtonNeu):
      case ButtonKey(kButtonMulti1):
      case ButtonKey(kButtonMulti2):
      case ButtonKey(kButtonMulti3):
      case ButtonKey(kButtonMulti4):
        house = static_cast<HousesType>(static_cast<int>(input & ~KN_BUTTON) -
                                        kButtonGdi);
        Set_House_Buttons(house, commands, kButtonGdi);
        break;

      /*..................................................................
      Invoke the members dialog
      ..................................................................*/
      case ButtonKey(kButtonMembers):
        /*
        .................... Take editor focus away .....................
        */
        membersbtn.Turn_Off();

        /*
        ....................... Invoke the dialog .......................
        */
        Team_Members(house);

        /*
        ............................ Redraw .............................
        */
        display = REDRAW_ALL;
        break;

      /*..................................................................
      OK: return
      ..................................................................*/
      case ButtonKey(kButtonOk):
        cancel = false;
        process = false;
        break;

      /*..................................................................
      Cancel: return
      ..................................................................*/
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      /*..................................................................
      Pass all other events to the currently-active text editor
      ..................................................................*/
      default:
        break;
    }
  }

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  /*
  ------------------------- If cancel, just return -------------------------
  */
  if (cancel) {
    return (-1);
  }

  /*
  ------------------------ Save selections & return ------------------------
  */
  CurTeam->Set_Name(name_buf);
  CurTeam->RecruitPriority = tech::ParseIntegerOr<int>(recr_buf, 0);
  CurTeam->MaxAllowed =
      static_cast<unsigned char>(tech::ParseIntegerOr<int>(maxnum_buf, 0));
  CurTeam->InitNum =
      static_cast<unsigned char>(tech::ParseIntegerOr<int>(initnum_buf, 0));
  CurTeam->IsRoundAbout = roundabout != 0;
  CurTeam->IsLearning = learning != 0;
  CurTeam->IsSuicide = suicide != 0;
  CurTeam->IsAutocreate = autocreate != 0;
  CurTeam->IsPrebuilt = prebuilt != 0;
  CurTeam->IsReinforcable = reinforce != 0;
  CurTeam->IsMercenary = mercenary != 0;
  CurTeam->House = house;
  CurTeam->MissionCount = missioncount;
  for (i = 0; i < missioncount; i++) {
    base::At(CurTeam->MissionList, i) = base::At(missions, i);
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
#define TEENSY_WEENSY
/*
**	Dialog & button dimensions
*/
constexpr int kDialogW = 608;
constexpr int kDialogX = ((640 - kDialogW) / 2);
constexpr int kDialogCx = kDialogX + (kDialogW / 2);

constexpr int kTxt6H = 14;
constexpr int kMargin = 14;

#ifdef TEENSY_WEENSY
  // D_PICTURE_W = 32,
  // D_PICTURE_H = 24,
constexpr int kPictureW = 64;  // 9 pictures / row, 16 pixel margin on each side
constexpr int kPictureH = 48;
#else
  // D_PICTURE_W = 32,
  // D_PICTURE_H = 30,
constexpr int kPictureW = 64;
constexpr int kPictureH = 60;
#endif
constexpr int kRowH = (kPictureH + 6);

constexpr int kOkW = 100;
constexpr int kOkH = 18;
constexpr int kOkX = kDialogCx - 10 - kOkW;
constexpr int kOkY = 0;

constexpr int kCancelW = 100;
constexpr int kCancelH = 18;
constexpr int kCancelX = kDialogCx + 10;
constexpr int kCancelY = 0;

/***************************************************************************
 * MapEditClass::Team_Members -- Team members dialog                       *
 *                                                                         *
 * INPUT:                                                                  *
 *		house			house to show members for
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *		0 = OK, -1 = cancel
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   04/09/1996 BRR : Created.                                             *
 *=========================================================================*/
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
  ............................ Dialog variables ............................
  */
  bool cancel = false;  // true = user cancels

  /*
  ......................... Team display variables .........................
  */
                                      //	int col;
  //// horizontal picture index 	int row;
  //// vertical picture index 	int x,y;

  /*
  **	Dialog dimensions.
  */

  /*
  **	Values for parsing the classes.
  */
  int curclass = -1;  // current index into 'teamclass'; can be invalid!
                      // (is based on current mouse position)

  /*
  **	Values for timing when mouse held down.
  */
  int lheld = 0;
  int rheld = 0;
  const int64_t tdelay[3] = {5, 20, 0};
  int tindex = 0;
  int64_t heldtime = 0;

  /*
  **	Buttons.
  */

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  /*
  **	Set up the team data arrays (ObjectTypeClass pointers & count)
  */
  std::array<const TechnoTypeClass*, kMaxTeamClasses>
      teamclass{};                               // array of team classes
  std::array<int, kMaxTeamClasses> teamcount{};  // array of class counts

  /*
  **	Fill in the ObjectTypeClass array with all available object type ptrs,
  **	checking to be sure this house can own the object
  */
  int i = 0;
  for (InfantryType i_id = INFANTRY_E1; i_id < INFANTRY_COUNT; i_id++) {
    if (Verify_House(house, &InfantryTypeClass::As_Reference(i_id))) {
      teamclass.at(base::ToSize(i)) = &InfantryTypeClass::As_Reference(i_id);
      i++;
    }
  }

  for (AircraftType a_id = AIRCRAFT_TRANSPORT; a_id < AIRCRAFT_COUNT; a_id++) {
    if (Verify_House(house, &AircraftTypeClass::As_Reference(a_id))) {
      teamclass.at(base::ToSize(i)) = &AircraftTypeClass::As_Reference(a_id);
      i++;
    }
  }

  for (UnitType u_id = UNIT_HTANK; u_id < UNIT_COUNT; u_id++) {
    if (Verify_House(house, &UnitTypeClass::As_Reference(u_id))) {
      teamclass.at(base::ToSize(i)) = &UnitTypeClass::As_Reference(u_id);
      i++;
    }
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
    teamcount.at(base::ToSize(j)) = 0;
  }

  /*
  **	Loop through all classes in the team.
  */
  for (i = 0; std::cmp_less(i, CurTeam->ClassCount); i++) {
    /*
    **	Find this class in our array.
    */
    for (int j = 0; j < maxclasses; j++) {
      /*
      **	Set the count; detect a match between the team's class & the
      **	'teamclass' array entry by comparing the actual pointers; typeid
      **	won't work because E1 & E2 are the same type class.
      */
      if (base::At(CurTeam->Class, i) == teamclass.at(base::ToSize(j))) {
        teamcount.at(base::ToSize(j)) = base::At(CurTeam->DesiredNum, i);
        break;
      }
    }
  }
  int numclasses =
      CurTeam->ClassCount;  // current # classes in the team (limited to <=5)

  /*
  **	Set up the dialog dimensions based on number of classes we have to draw
  **
  **	Compute picture rows & cols.
  */
  const int numcols =
      (kDialogW - 16) / kPictureW;  // # units displayed horizontally
  const int numrows =
      (maxclasses + numcols - 1) / numcols;  // # units displayed vertically

  //
  //	Dialog's height = top margin + label + picture rows +
  // margin + label + margin + btn
  //
  int dlg_h = (kMargin + kTxt6H + kMargin + (numrows * kRowH) + kMargin +
               kTxt6H + kMargin + kOkH + kMargin);  // dialog height
  dlg_h = std::min(dlg_h, 400);
  const int dlg_y = (400 - dlg_h) / 2;
  const int dlg_picture_top =
      dlg_y + kMargin + kTxt6H + kMargin;  // coord of top of pictures
  const int msg_y = dlg_y + kMargin + kTxt6H + kMargin + (numrows * kRowH) +
                    kMargin;  // y-coord for object names

  okbtn.Y = dlg_y + dlg_h - kMargin - kOkH;
  cancelbtn.Y = dlg_y + dlg_h - kMargin - kCancelH;

  /*
  **	Draw to SeenBuff.
  */
  Set_Logic_Page(SeenBuff);

  /*
  **	Make sure 'house' is valid.
  */
  if (house != HOUSE_GOOD && house != HOUSE_BAD && house != HOUSE_MULTI1 &&
      house != HOUSE_MULTI2 && house != HOUSE_MULTI3 && house != HOUSE_MULTI4) {
    if (ScenPlayer == SCEN_PLAYER_MPLAYER) {
      house = HOUSE_MULTI1;
    } else {
      house = HOUSE_GOOD;
    }
  }

  /*
  **	Create the list.
  */
  ControlClass* commands = &okbtn;
  cancelbtn.Add_Tail(*commands);

  /*
  **	Main Processing Loop.
  */
  RedrawType display = REDRAW_ALL;  // requested redraw level
  bool process = true;              // loop while true
  while (process) {
    /*
    ** If we have just received input focus again after running in the
    *background then
    ** we need to redraw.
    */
    if (AllSurfaces.SurfacesRestored) {
      AllSurfaces.SurfacesRestored = false;
      display = REDRAW_ALL;
    }

    /*
    **	Invoke game callback.
    */
    Call_Back();

    /*
    **	Refresh display if needed.
    */
    if (display != REDRAW_NONE) {
      /*
      **	Display the dialog box.
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        /*
        **	Display the constant background of this dialog.
        */
        Dialog_Box(kDialogX, dlg_y, kDialogW, dlg_h);
        Draw_Caption(TXT_NONE, kDialogX, dlg_y, kDialogW);
        Fancy_Text_Print(
            "Team Members", kDialogCx, dlg_y + kMargin, kCcGreen, kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);

        //
        //	Draw the objects.
        //
        for (i = 0; i < maxclasses; i++) {
          //
          //	Display the object along with any count value for it.
          //
          Draw_Member(teamclass.at(base::ToSize(i)), i,
                      teamcount.at(base::ToSize(i)), house, kDialogX + 16,
                      dlg_picture_top);
        }

        if (static_cast<unsigned>(curclass) < static_cast<unsigned>(maxclasses)) {
          Fancy_Text_Print(
              teamclass.at(base::ToSize(curclass))->Full_Name(),
              kDialogX + (kDialogW / 2), msg_y, kCcTan, kTBlack,
              TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
        }
      }

      /*
      **	Redraw the buttons.
      */
      if (display >= REDRAW_BUTTONS) {
        commands->Draw_All();
      }
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    **	Get user input.
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    **	Process input.
    */
    switch (static_cast<int>(input)) {
      /*
      **	Mouse buttons set or clear 'held' values
      */
      case KN_LMOUSE:
        if (curclass >= 0 && curclass < maxclasses) {
          lheld = 1;
          tindex = 2;
          heldtime = 0;
        }
        break;

      case KN_RMOUSE:
        if (curclass >= 0 && curclass < maxclasses) {
          rheld = 1;
          tindex = 2;
          heldtime = 0;
        }
        break;

      case (KN_LMOUSE | KN_RLSE_BIT):
        lheld = 0;
        break;

      case (KN_RMOUSE | KN_RLSE_BIT):
        rheld = 0;
        break;

      /*
      **	OK: save values & return.
      */
      case ButtonKey(kButtonOk):
        process = false;
        break;

      /*
      **	Cancel: abort & return.
      */
      case ButtonKey(kButtonCancel):
        cancel = true;
        process = false;
        break;

      default:
        /*
        **	Compute new 'curclass' based on mouse position.
        */
        i = ((Get_Mouse_X() - 16 - kDialogX) / kPictureW) +
            (((Get_Mouse_Y() - dlg_picture_top) / kRowH) * numcols);

        /*
        **	If it's changed, update class label.
        */
        if (i != curclass) {
          curclass = i;

          /*
          **	Clear out the previously printed name of the item.
          */
          Hide_Mouse();
          LogicPage->Fill_Rect(kDialogX + 8, msg_y, kDialogX + kDialogW - 9,
                               msg_y + kTxt6H, kBlack);

          if (static_cast<unsigned>(curclass) < static_cast<unsigned>(maxclasses)) {
            Fancy_Text_Print(
                teamclass.at(base::ToSize(curclass))->Full_Name(),
                kDialogX + (kDialogW / 2), msg_y, kCcGreen, kTBlack,
                TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
          }

          /*
          **	Force buttons to not be held.
          */
          lheld = 0;
          rheld = 0;
          Show_Mouse();
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
      **	The first time in, TickCount - heldtime will be larger than
      **	tdelay[2], so we increment the count immediately; then, we
      *decrement *	tindex to go to the next time delay, which is longer;
      *then, decr. *	again to go to the 1st time delay which is the shortest.
      */
      if (TickCount.Time() - heldtime > base::At(tdelay, tindex)) {
        heldtime = TickCount.Time();
        if (tindex) {
          tindex--;
        }

        /*
        **	Detect addition of a new class.
        */
        if (teamcount.at(base::ToSize(curclass)) == 0) {
          /*
          **	Don't allow more classes than we can handle.
          */
          if (numclasses == TeamTypeClass::kMaxTeamClasscount) {
            continue;
          }
          numclasses++;
        }
        teamcount.at(base::ToSize(curclass))++;

        /*
        **	Update number label.
        */
        Draw_Member(teamclass.at(base::ToSize(curclass)), curclass,
                    teamcount.at(base::ToSize(curclass)), house, kDialogX + 16,
                    dlg_picture_top);
      }

    } else {
      /*
      **	The first time in, TickCount - heldtime will be larger than
      **	tdelay[2], so we increment the count immediately; then, we
      *decrement *	tindex to go to the next time delay, which is longer;
      *then, decr. *	again to go to the 1st time delay which is the shortest.
      */
      if (rheld && (TickCount.Time() - heldtime > base::At(tdelay, tindex))) {
        if (tindex) {
          tindex--;
        }
        heldtime = TickCount.Time();

        if (teamcount.at(base::ToSize(curclass)) > 0) {
          teamcount.at(base::ToSize(curclass))--;

          /*
          **	Detect removal of a class.
          */
          if (teamcount.at(base::ToSize(curclass)) == 0) {
            numclasses--;
          }
        }

        /*
        **	Update number label.
        */
        Draw_Member(teamclass.at(base::ToSize(curclass)), curclass,
                    teamcount.at(base::ToSize(curclass)), house, kDialogX + 16,
                    dlg_picture_top);
      }
    }
  }

  /*
  **	Copy data into team.
  */
  if (!cancel) {
    CurTeam->ClassCount = static_cast<unsigned char>(numclasses);
    i = 0;  // current team class index
    for (int j = 0; j < maxclasses; j++) {
      if (teamcount.at(base::ToSize(j)) > 0) {
        base::At(CurTeam->DesiredNum, i) =
            static_cast<unsigned char>(teamcount.at(base::ToSize(j)));
        base::At(CurTeam->Class, i) = teamclass.at(base::ToSize(j));
        i++;
      }
    }
  }

  /*
  **	Redraw the display.
  */
  HiddenPage.Clear();
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
 *          house -- The owner of this object. * pic_x, pic_y -- x,y coords of
 *upper-left corner to start drawing at
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/02/1995 JLB : Created. *
 *=============================================================================================*/
void MapEditClass::Draw_Member(const TechnoTypeClass* ptr, int index, int quant,
                               HousesType house, int pic_x, int pic_y) {
  const int numcols = (kDialogW - 32) / kPictureW;
  const int col = index % numcols;
  const int row = index / numcols;
  const int x = pic_x + (col * kPictureW);
  const int y = pic_y + (row * kRowH);

  base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)), kWindowX) = 0;
  base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)), kWindowY) = 0;
  base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
           kWindowWidth) = 640 / 8;
  base::At(base::At(WindowList, static_cast<int>(WINDOW_EDITOR)),
           kWindowHeight) = 400;
  Change_Window(static_cast<int>(WINDOW_EDITOR));

  Hide_Mouse();
  Draw_Box(x, y, kPictureW, kPictureH, BOXSTYLE_GREEN_DOWN, true);

  ptr->Display(x + (kPictureW / 2), y + (kPictureH / 2), WINDOW_EDITOR, house);

  if (quant > 0) {
    Fancy_Text_Print("%d", x + 1, y + kPictureH - 16, kCcGreen, kTBlack,
                     TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_DROPSHADOW, quant);
  }

  Show_Mouse();
}

/***************************************************************************
 * MapEditClass::Build_Mission_list -- fills in mission list box           *
 *                                                                         *
 * INPUT:                                                                  *
 *      missioncount      # of missions to add to the list                 *
 *      missions            array of TeamMissionStruct's                   *
 *      missionbuf         character arrays to store strings in            *
 *      list               list box to add strings to                      *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = cancel                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/07/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Build_Mission_List(
    int missioncount,
    const TeamMissionStruct (&missions)[TeamTypeClass::kMaxTeamMissions],
    char (&missionbuf)[TeamTypeClass::kMaxTeamMissions][20], ListClass* list) {
  /*
  ** Start with an empty list
  */
  while (list->Count()) {
    list->Remove_Item(list->Get_Item(0));
  }

  for (int i = 0; i < missioncount; i++) {
    /*
    ** generate the string for a MOVE mission; the argument is the
    ** letter-designation of the cell to move to.
    */
    if (base::At(missions, i).Mission == TMISSION_MOVE ||
        base::At(missions, i).Mission == TMISSION_UNLOAD) {
      absl::SNPrintF(
          base::At(missionbuf, i), sizeof(base::At(missionbuf, i)), "%s\t%c",
          TeamTypeClass::Name_From_Mission(base::At(missions, i).Mission),
          base::At(missions, i).Argument + 'A');
    } else {
      /*
      ** All other missions take a numeric argument.
      */
      absl::SNPrintF(
          base::At(missionbuf, i), sizeof(base::At(missionbuf, i)), "%s\t%d",
          TeamTypeClass::Name_From_Mission(base::At(missions, i).Mission),
          base::At(missions, i).Argument);
    }

    /*
    ** Add the string to the list box
    */
    list->Add_Item(base::At(missionbuf, i));
  }
}
