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

/* $Header:   F:\projects\c&c\vcs\code\mapedplc.cpv   2.16   16 Oct 1995
 * 16:51:00   JOE_BOSTIC  $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : MAPEDPLC.CPP                             *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : November 18, 1994                        *
 *                                                                         *
 *                  Last Update : July 4, 1995 [JLB]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Object-placement routines                                               *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Placement_Dialog -- adds an object to the scenario      *
 *   MapEditClass::Start_Placement -- enters placement mode                *
 *   MapEditClass::Place_Object -- attempts to place the current object    *
 *   MapEditClass::Cancel_Placement -- cancels placement mode              *
 *   MapEditClass::Place_Next -- while placing object, goes to next        *
 *   MapEditClass::Place_Prev -- while placing object, goes to previous    *
 *   MapEditClass::Place_Next_Category -- places next object category      *
 *   MapEditClass::Place_Prev_Category -- places previous object category  *
 *   MapEditClass::Place_Home -- homes the placement object                *
 *   MapEditClass::Toggle_House -- toggles current placement object's house*
 *   MapEditClass::Set_House_Buttons -- toggles house buttons for btn list *
 *   MapEditClass::Start_Trigger_Placement -- enters trigger placement mode*
 *   MapEditClass::Stop_Trigger_Placement -- exits trigger placement mode  *
 *   MapEditClass::Place_Trigger -- assigns trigger to object or cell      *
 *   MapEditClass::Start_Base_Building -- starts base-building mode        *
 *   MapEditClass::Cancel_Base_Building -- stops base-building mode        *
 *   MapEditClass::Build_Base_To -- builds the AI base to the given percent*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstdint>

#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/misc.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "td/base.h"
#include "td/building.h"
#include "td/cell.h"
#include "td/conquer.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dialog.h"
#include "td/display_constants.h"
#include "td/externs.h"
#include "td/gadget.h"
#include "td/globals.h"
#include "td/goptions.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/msgbox.h"
#include "td/object.h"
#include "td/techno.h"
#include "td/textbtn.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/vector.h"

/***************************************************************************
 * MapEditClass::Placement_Dialog -- adds an object to the scenario        *
 *                                                                         *
 * This function sets LastChoice & LastHouse to the values chosen          *
 * by the user. It's up to the caller to call Start_Placement to enter     *
 * placement mode.                                                         *
 *   This routine does not modify PendingObject or PendingHouse.           *
 *                                                                         *
 *  Ŀ                   *
 *     [GDI]  [NOD]  [Neutral]                                           *
 *                                                                       *
 *     Ŀ                                 *
 *                                     [Template]                      *
 *                                     [Overlay ]                      *
 *                                     [Smudge  ]                      *
 *                                     [Terrain ]                      *
 *           (Object picture)          [Unit    ]                      *
 *                                     [Infantry]                      *
 *                                     [Aircraft]                      *
 *                                     [Building]                      *
 *                                                                     *
 *          Ŀ                    *
 *               [<-]  [->]                  (Grid)                    *
 *                                                                     *
 *           [OK]        [Cancel]                                *
 *                     *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = cancel                                                *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   10/21/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Placement_Dialog() {
  /*........................................................................
  Dialog & button dimensions
  ........................................................................*/
  constexpr int kDialogW = 480;
  constexpr int kDialogH = 360;
  constexpr int kDialogX = ((640 - kDialogW) / 2);
  constexpr int kDialogY = ((400 - kDialogH) / 2);
  [[maybe_unused]] constexpr int kDialogCx = kDialogX + (kDialogW / 2);

  constexpr int kTxt8H = 22;
  constexpr int kMargin = 14;

  constexpr int kPictureW = 304;  // must be divisible by 8!
  constexpr int kPictureH = 210;
  constexpr int kPictureX = kDialogX + 16;  // must start on a byte boundary!
  constexpr int kPictureY = kDialogY + kMargin + kTxt8H + kMargin;
  constexpr int kPictureCx = kPictureX + (kPictureW / 2);

  constexpr int kGdiW = 90;
  constexpr int kGdiH = 18;
  constexpr int kGdiX = kDialogX + kMargin;
  constexpr int kGdiY = kDialogY + kMargin;

  constexpr int kNodW = 90;
  constexpr int kNodH = 18;
  constexpr int kNodX = kGdiX + kGdiW;
  constexpr int kNodY = kDialogY + kMargin;

  constexpr int kNeutralW = 90;
  constexpr int kNeutralH = 18;
  constexpr int kNeutralX = kNodX + kNodW;
  constexpr int kNeutralY = kDialogY + kMargin;

  constexpr int kMulti1W = 44;
  constexpr int kMulti1H = 18;
  constexpr int kMulti1X = kGdiX;
  constexpr int kMulti1Y = kGdiY;

  constexpr int kMulti2W = 44;
  constexpr int kMulti2H = 18;
  constexpr int kMulti2X = kMulti1X + kMulti1W;
  constexpr int kMulti2Y = kGdiY;

  constexpr int kMulti3W = 44;
  constexpr int kMulti3H = 18;
  constexpr int kMulti3X = kMulti2X + kMulti2W;
  constexpr int kMulti3Y = kGdiY;

  constexpr int kMulti4W = 44;
  constexpr int kMulti4H = 18;
  constexpr int kMulti4X = kMulti3X + kMulti3W;
  constexpr int kMulti4Y = kGdiY;

  constexpr int kLeftW = 90;
  constexpr int kLeftH = 18;
  constexpr int kLeftX = kPictureCx - 5 - kLeftW;
  constexpr int kLeftY = kPictureY + kPictureH + kMargin;

  constexpr int kRightW = 90;
  constexpr int kRightH = 18;
  constexpr int kRightX = kPictureCx + 5;
  constexpr int kRightY = kPictureY + kPictureH + kMargin;

  constexpr int kTemplateW = 140;
  constexpr int kTemplateH = 18;
  constexpr int kTemplateX = kDialogX + kDialogW - kMargin - kTemplateW;
  constexpr int kTemplateY = kPictureY;

  constexpr int kOverlayW = 140;
  constexpr int kOverlayH = 18;
  constexpr int kOverlayX = kDialogX + kDialogW - kMargin - kOverlayW;
  constexpr int kOverlayY = kTemplateY + kTemplateH;

  constexpr int kSmudgeW = 140;
  constexpr int kSmudgeH = 18;
  constexpr int kSmudgeX = kDialogX + kDialogW - kMargin - kSmudgeW;
  constexpr int kSmudgeY = kOverlayY + kOverlayH;

  constexpr int kTerrainW = 140;
  constexpr int kTerrainH = 18;
  constexpr int kTerrainX = kDialogX + kDialogW - kMargin - kTerrainW;
  constexpr int kTerrainY = kSmudgeY + kSmudgeH;

  constexpr int kUnitW = 140;
  constexpr int kUnitH = 18;
  constexpr int kUnitX = kDialogX + kDialogW - kMargin - kUnitW;
  constexpr int kUnitY = kTerrainY + kTerrainH;

  constexpr int kInfantryW = 140;
  constexpr int kInfantryH = 18;
  constexpr int kInfantryX = kDialogX + kDialogW - kMargin - kInfantryW;
  constexpr int kInfantryY = kUnitY + kUnitH;

  constexpr int kAircraftW = 140;
  constexpr int kAircraftH = 18;
  constexpr int kAircraftX = kDialogX + kDialogW - kMargin - kAircraftW;
  constexpr int kAircraftY = kInfantryY + kInfantryH;

  constexpr int kBuildingW = 140;
  constexpr int kBuildingH = 18;
  constexpr int kBuildingX = kDialogX + kDialogW - kMargin - kBuildingW;
  constexpr int kBuildingY = kAircraftY + kAircraftH;

  constexpr int kOkW = 90;
  constexpr int kOkH = 18;
  constexpr int kOkX = kPictureCx - kOkW - 5;
  constexpr int kOkY = kDialogY + kDialogH - kOkH - kMargin;

  constexpr int kCancelW = 90;
  constexpr int kCancelH = 18;
  constexpr int kCancelX = kPictureCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kCancelH - kMargin;

  /*........................................................................
  Grid Dimensions
  ........................................................................*/
  constexpr int kGridsize = 10;
  constexpr int kGridblockW = 6;
  constexpr int kGridblockH = 6;
  constexpr int kGridX =
      kDialogX + kDialogW - (kGridsize * kGridblockW) - kMargin;
  constexpr int kGridY =
      kDialogY + kDialogH - (kGridsize * kGridblockH) - kMargin;
  /*........................................................................
  Button enumerations:
  ........................................................................*/
  constexpr int kButtonGdi = 100;
  constexpr int kButtonNod = 101;
  constexpr int kButtonNeutral = 102;
  [[maybe_unused]] constexpr int kButtonJp = 103;  // placeholder
  constexpr int kButtonMulti1 = 104;
  constexpr int kButtonMulti2 = 105;
  constexpr int kButtonMulti3 = 106;
  constexpr int kButtonMulti4 = 107;
  constexpr int kButtonNext = 108;
  constexpr int kButtonPrev = 109;
  constexpr int kButtonOk = 110;
  constexpr int kButtonCancel = 111;
  constexpr int kButtonTemplate = 112;
  constexpr int kButtonOverlay = 113;
  constexpr int kButtonSmudge = 114;
  constexpr int kButtonTerrain = 115;
  constexpr int kButtonUnit = 116;
  constexpr int kButtonInfantry = 117;
  constexpr int kButtonAircraft = 118;
  constexpr int kButtonBuilding = 119;
  /*........................................................................
  Redraw values: in order from "top" to "bottom" layer of the dialog
  ........................................................................*/
  enum class RedrawType {
    REDRAW_NONE = 0,
    REDRAW_BUTTONS = 1,
    REDRAW_OBJECT = 2,
    REDRAW_BACKGROUND = 3,
    REDRAW_ALL = REDRAW_BACKGROUND
  };
  using enum RedrawType;
  /*........................................................................
  Dialog variables
  ........................................................................*/
  bool cancel = false;            // true = user cancels
  int x = 0;
  int y = 0;  // for drawing the grid
  int i = 0;
  int typeindex = 0;  // index of class type
  /*........................................................................
  Buttons
  ........................................................................*/

  TextButtonClass gdibtn(
      kButtonGdi, "GDI",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kGdiX, kGdiY,
      kGdiW, kGdiH);

  TextButtonClass nodbtn(
      kButtonNod, "NOD",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNodX, kNodY,
      kNodW, kNodH);

  TextButtonClass neutbtn(
      kButtonNeutral, "Neutral",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kNeutralX,
      kNeutralY, kNeutralW, kNeutralH);

  TextButtonClass multi1btn(
      kButtonMulti1, "M1",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti1X,
      kMulti1Y, kMulti1W, kMulti1H);

  TextButtonClass multi2btn(
      kButtonMulti2, "M2",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti2X,
      kMulti2Y, kMulti2W, kMulti2H);

  TextButtonClass multi3btn(
      kButtonMulti3, "M3",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti3X,
      kMulti3Y, kMulti3W, kMulti3H);

  TextButtonClass multi4btn(
      kButtonMulti4, "M4",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kMulti4X,
      kMulti4Y, kMulti4W, kMulti4H);

  TextButtonClass nextbtn(
      kButtonNext, TXT_RIGHT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kRightX,
      kRightY, kRightW, kRightH);

  TextButtonClass prevbtn(
      kButtonPrev, TXT_LEFT,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kLeftX,
      kLeftY, kLeftW, kLeftH);

  TextButtonClass okbtn(
      kButtonOk, TXT_OK,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOkX, kOkY,
      kOkW, kOkH);

  TextButtonClass cancelbtn(
      kButtonCancel, TXT_CANCEL,
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kCancelX,
      kCancelY, kCancelW, kCancelH);

  TextButtonClass templatebtn(
      kButtonTemplate, "Template",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kTemplateX,
      kTemplateY, kTemplateW, kTemplateH);

  TextButtonClass overlaybtn(
      kButtonOverlay, "Overlay",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kOverlayX,
      kOverlayY, kOverlayW, kOverlayH);

  TextButtonClass smudgebtn(
      kButtonSmudge, "Smudge",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kSmudgeX,
      kSmudgeY, kSmudgeW, kSmudgeH);

  TextButtonClass terrainbtn(
      kButtonTerrain, "Terrain",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kTerrainX,
      kTerrainY, kTerrainW, kTerrainH);

  TextButtonClass unitbtn(
      kButtonUnit, "Unit",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kUnitX,
      kUnitY, kUnitW, kUnitH);

  TextButtonClass infantrybtn(
      kButtonInfantry, "Infantry",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kInfantryX,
      kInfantryY, kInfantryW, kInfantryH);

  TextButtonClass aircraftbtn(
      kButtonAircraft, "Aircraft",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kAircraftX,
      kAircraftY, kAircraftW, kAircraftH);

  TextButtonClass buildingbtn(
      kButtonBuilding, "Building",
      TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW, kBuildingX,
      kBuildingY, kBuildingW, kBuildingH);

  /*------------------------------------------------------------------------
  Initialize addable objects list; we must do this every time in case one
  of the object pools has become exhausted; that object won't be available
  for adding.  (Skip aircraft, since they won't be used in the editor.)
  ------------------------------------------------------------------------*/
  Clear_List();
  TemplateTypeClass::Prep_For_Add();
  OverlayTypeClass::Prep_For_Add();
  SmudgeTypeClass::Prep_For_Add();
  TerrainTypeClass::Prep_For_Add();
  UnitTypeClass::Prep_For_Add();
  InfantryTypeClass::Prep_For_Add();
  BuildingTypeClass::Prep_For_Add();

  /*........................................................................
  Compute offset of each class type in the Objects array
  ........................................................................*/
  TypeOffset[0] = 0;
  for (i = 1; i < kNumEditClasses; i++) {
    TypeOffset[i] = TypeOffset[i - 1] + NumType[i - 1];
  }

  /*
  --------------------- Return if no objects to place ----------------------
  */
  if (!ObjCount) {
    return (-1);
  }

  /*
  ------------------------------- Initialize -------------------------------
  */
  Set_Logic_Page(SeenBuff);
  if (LastChoice >= ObjCount) {
    LastChoice = 0;
  }
  const ObjectTypeClass* curobj =
      Objects[LastChoice];  // Working object pointer.  // current object to
                            // choose

  ControlClass* commands = &neutbtn;
  if (ScenPlayer == SCEN_PLAYER_MPLAYER) {
    multi1btn.Add_Tail(*commands);
    multi2btn.Add_Tail(*commands);
    multi3btn.Add_Tail(*commands);
    multi4btn.Add_Tail(*commands);
  } else {
    gdibtn.Add_Tail(*commands);
    nodbtn.Add_Tail(*commands);
  }
  nextbtn.Add_Tail(*commands);
  prevbtn.Add_Tail(*commands);
  okbtn.Add_Tail(*commands);
  cancelbtn.Add_Tail(*commands);
  templatebtn.Add_Tail(*commands);
  overlaybtn.Add_Tail(*commands);
  smudgebtn.Add_Tail(*commands);
  terrainbtn.Add_Tail(*commands);
  unitbtn.Add_Tail(*commands);
  infantrybtn.Add_Tail(*commands);
  aircraftbtn.Add_Tail(*commands);
  buildingbtn.Add_Tail(*commands);

  /*........................................................................
  If the current house isn't valid for the current object type, cycle to
  the next house.
  ........................................................................*/
  if (!Verify_House(LastHouse, curobj)) {
    LastHouse = Cycle_House(LastHouse, curobj);
  }

  /*
  ..................... Set the buttons for this house .....................
  */
  Set_House_Buttons(LastHouse, commands, kButtonGdi);

  /*
  -------------------------- Main processing loop --------------------------
  */
  RedrawType display = REDRAW_ALL;  // display level
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
    ---------------------- Refresh display if needed ----------------------
    */
    if (display > REDRAW_NONE) {
      /*
      ---------------------- Display the dialog box ----------------------
      */
      Hide_Mouse();
      if (display >= REDRAW_BACKGROUND) {
        Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
        Draw_Caption(TXT_NONE, kDialogX, kDialogY, kDialogW);
      }

      /*------------------------------------------------------------------
      Display the current object:
      - save the current window dimensions
      - adjust the window size to the actual drawable area
      - draw the shape
      - reset the window dimensions
      ------------------------------------------------------------------*/
      if (display >= REDRAW_OBJECT) {
        WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowX] = kPictureX / 8;
        WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowY] = kPictureY;
        WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowWidth] =
            kPictureW / 8;
        WindowList[static_cast<int>(WINDOW_EDITOR)][kWindowHeight] = kPictureH;
        Change_Window(static_cast<int>(WINDOW_EDITOR));
        Draw_Box(kPictureX, kPictureY, kPictureW, kPictureH,
                 BOXSTYLE_GREEN_DOWN, true);
        curobj->Display(ScreenWidth * 4, ScreenHeight / 2, WINDOW_EDITOR,
                        LastHouse);

        /*
        ........................ Erase the grid .........................
        */
        LogicPage->Fill_Rect(kGridX - (kGridblockW * 2), kGridY,
                             kGridX + (kGridsize * kGridblockW),
                             kGridY + (kGridsize * kGridblockH), kBlack);

        /*
        .............. Draw a box for every cell occupied ...............
        */
        const int16_t* occupy =
            curobj->Occupy_List();  // ptr into object's OccupyList
        while ((*occupy) != REFRESH_EOL) {
          const int cell = (*occupy);  // cell index for parsing OccupyList
          occupy++;
          x = kGridX + ((cell % MAP_CELL_W) * kGridblockW);
          y = kGridY + ((cell / MAP_CELL_W) * kGridblockH);
          LogicPage->Fill_Rect(x, y, x + kGridblockW - 1, y + kGridblockH - 1,
                               kCcBrightGreen);
        }

        /*
        ..................... Draw the grid itself ......................
        */
        for (y = 0; y <= kGridsize; y++) {
          for (x = 0; x <= kGridsize; x++) {
            LogicPage->Draw_Line(
                kGridX + (x * kGridblockW), kGridY, kGridX + (x * kGridblockW),
                kGridY + (kGridsize * kGridblockH), kCcGreenShadow);
          }
          LogicPage->Draw_Line(kGridX, kGridY + (y * kGridblockH),
                               kGridX + (kGridsize * kGridblockW),
                               kGridY + (y * kGridblockH), kCcGreenShadow);
        }

        /*...............................................................
        Print the object's label from the class's Full_Name().
        Warning: Text_String returns an EMS pointer, so standard string
        functions won't work!
        ...............................................................*/
        Fancy_Text_Print(
            curobj->Full_Name(), kPictureCx, kPictureY + kMargin, kCcGreen,
            kTBlack,
            TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
      }

      /*
      -------------------------- Redraw buttons --------------------------
      */
      if (display >= REDRAW_BUTTONS) {
        /*...............................................................
        Figure out which class category we're in & highlight that button
        This updates 'typeindex', which is used below, and it also updates
        the category button states.
        ...............................................................*/
        i = 0;
        for (typeindex = 0; typeindex < kNumEditClasses; typeindex++) {
          i += NumType[typeindex];
          if (LastChoice < i) {
            break;
          }
        }
        templatebtn.Turn_Off();
        overlaybtn.Turn_Off();
        smudgebtn.Turn_Off();
        terrainbtn.Turn_Off();
        unitbtn.Turn_Off();
        infantrybtn.Turn_Off();
        aircraftbtn.Turn_Off();
        buildingbtn.Turn_Off();
        switch (typeindex + kButtonTemplate) {
          case kButtonTemplate:
            templatebtn.Turn_On();
            break;

          case kButtonOverlay:
            overlaybtn.Turn_On();
            break;

          case kButtonSmudge:
            smudgebtn.Turn_On();
            break;

          case kButtonTerrain:
            terrainbtn.Turn_On();
            break;

          case kButtonUnit:
            unitbtn.Turn_On();
            break;

          case kButtonInfantry:
            infantrybtn.Turn_On();
            break;

          case kButtonAircraft:
            aircraftbtn.Turn_On();
            break;

          case kButtonBuilding:
            buildingbtn.Turn_On();
            break;
          default:
            break;
        }
      }

      /*
      .......................... Redraw buttons ..........................
      */
      commands->Draw_All();
      Show_Mouse();
      display = REDRAW_NONE;
    }

    /*
    ........................... Get user input ............................
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    ------------------------- Process user input --------------------------
    */
    switch (static_cast<int>(input)) {
      /*
      ---------------------------- GDI House -----------------------------
      */
      case ButtonKey(kButtonGdi):
      case ButtonKey(kButtonNod):
      case ButtonKey(kButtonNeutral):
      case ButtonKey(kButtonMulti1):
      case ButtonKey(kButtonMulti2):
      case ButtonKey(kButtonMulti3):
      case ButtonKey(kButtonMulti4): {
        const auto house = static_cast<HousesType>(
            static_cast<int>(input & ~KN_BUTTON) - kButtonGdi);
        /*
        ............... ignore if invalid for this object ...............
        */
        if (!Verify_House(house, curobj)) {
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
          break;
        }

        /*
        ...................... Set flags & buttons ......................
        */
        LastHouse = house;
        Set_House_Buttons(LastHouse, commands, kButtonGdi);
        display = REDRAW_OBJECT;
        break;
      }

      /*
      --------------------------- Next in list ---------------------------
      */
      case KN_RIGHT:
      case ButtonKey(kButtonNext):
        /*
        ..................... Increment to next obj .....................
        */
        LastChoice++;
        if (LastChoice == ObjCount) {
          LastChoice = 0;
        }
        curobj = Objects[LastChoice];

        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }

        nextbtn.Turn_Off();
        display = REDRAW_OBJECT;
        break;

      /*
      ------------------------- Previous in list -------------------------
      */
      case KN_LEFT:
      case ButtonKey(kButtonPrev):
        /*
        ..................... Decrement to prev obj .....................
        */
        LastChoice--;
        if (LastChoice < 0) {
          LastChoice = ObjCount - 1;
        }
        curobj = Objects[LastChoice];

        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }

        prevbtn.Turn_Off();
        display = REDRAW_OBJECT;
        break;

      /*
      ----------------------- Select a class type ------------------------
      */
      case ButtonKey(kButtonTemplate):
      case ButtonKey(kButtonOverlay):
      case ButtonKey(kButtonSmudge):
      case ButtonKey(kButtonTerrain):
      case ButtonKey(kButtonUnit):
      case ButtonKey(kButtonInfantry):
      case ButtonKey(kButtonAircraft):
      case ButtonKey(kButtonBuilding):
        /*
        ...................... Find index of class ......................
        */
        typeindex = input - ButtonKey(kButtonTemplate);

        /*
        ............ If no objects of that type, do nothing .............
        */
        if (NumType[typeindex] == 0) {
          display = REDRAW_BUTTONS;  // force to reset button states
          break;
        }

        /*
        ...................... Set current object .......................
        */
        LastChoice = TypeOffset[typeindex];
        curobj = Objects[LastChoice];

        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }

        display = REDRAW_OBJECT;
        break;

      /*
      -------------------------- Next category ---------------------------
      */
      case KN_PGDN:
        typeindex++;
        if (typeindex == kNumEditClasses) {
          typeindex = 0;
        }

        /*
        ...................... Set current object .......................
        */
        LastChoice = TypeOffset[typeindex];
        curobj = Objects[LastChoice];

        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }

        display = REDRAW_OBJECT;
        break;

      /*
      ------------------------ Previous category -------------------------
      */
      case KN_PGUP:
        typeindex--;
        if (typeindex < 0) {
          typeindex = kNumEditClasses - 1;
        }

        /*
        ...................... Set current object .......................
        */
        LastChoice = TypeOffset[typeindex];
        curobj = Objects[LastChoice];

        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }

        display = REDRAW_OBJECT;
        break;

      /*
      ------------------------ Jump to 1st choice ------------------------
      */
      case KN_HOME:
        LastChoice = 0;
        /*
        ...................... Set current object .......................
        */
        curobj = Objects[LastChoice];
        /*
        .................... Get valid house for obj ....................
        */
        if (!Verify_House(LastHouse, curobj)) {
          LastHouse = Cycle_House(LastHouse, curobj);
          Set_House_Buttons(LastHouse, commands, kButtonGdi);
        }
        display = REDRAW_OBJECT;
        break;

      /*
      -------------------------------- OK --------------------------------
      */
      case KN_RETURN:
      case ButtonKey(kButtonOk):
        cancel = false;
        process = false;
        break;

      /*
      ------------------------------ Cancel ------------------------------
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

  /*
  --------------------------- Redraw the display ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();

  if (cancel) {
    return (-1);
  }

  return 0;
}

/***************************************************************************
 * MapEditClass::Start_Placement -- enters placement mode                  *
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
 *   11/04/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Start_Placement() {

  /*------------------------------------------------------------------------
  Initialize addable objects list; we must do this every time in case one
  of the object pools has become exhausted; that object won't be available
  for adding.
  ------------------------------------------------------------------------*/
  Clear_List();
  TemplateTypeClass::Prep_For_Add();
  OverlayTypeClass::Prep_For_Add();
  SmudgeTypeClass::Prep_For_Add();
  TerrainTypeClass::Prep_For_Add();
  UnitTypeClass::Prep_For_Add();
  InfantryTypeClass::Prep_For_Add();
  // AircraftTypeClass::Prep_For_Add();
  BuildingTypeClass::Prep_For_Add();
  /*........................................................................
  Compute offset of each class type in the Objects array
  ........................................................................*/
  TypeOffset[0] = 0;
  for (int i = 1; i < kNumEditClasses; i++) {
    TypeOffset[i] = TypeOffset[i - 1] + NumType[i - 1];
  }

  /*
  ---------------------- Create the placement object -----------------------
  */
  /*------------------------------------------------------------------------
  Create the placement object:
  - For normal placement mode, use the last-used index into Objects
    (LastChoice), and the last-used house (LastHouse).
  - For base-building mode, force the object to be a building, and use the
    House specified in the Base object
  ------------------------------------------------------------------------*/
  if (!BaseBuilding) {
    LastChoice = std::min(LastChoice, ObjCount - 1);
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(LastHouse));
  } else {
    LastChoice = std::clamp(LastChoice, TypeOffset[7], ObjCount - 1);
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse = Base.House;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(LastHouse));
  }

  /*
  ------------------- Error if no more objects available -------------------
  */
  if (!PendingObjectPtr) {
    CCMessageBox().Process("No more objects of this type available.");
    HiddenPage.Clear();
    Flag_To_Redraw(true);
    Render();
    PendingObject = nullptr;
    if (BaseBuilding) {
      Cancel_Base_Building();
    }
    return;
  }

  /*
  ------------------------ Set the placement cursor ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(PendingObject->Occupy_List());
}

/***************************************************************************
 * MapEditClass::Place_Object -- attempts to place the current object      *
 *                                                                         *
 * Placement of "real" objects is simply checked via their Unlimbo routine.*
 * Placement of templates is more complex:                                 *
 * - for every cell in the template's OccupyList, check for objects        *
 *     already in that cell by looking at the cell's OccupyList &          *
 *     OverlapList                                                         *
 * - "lift" all the objects in the cell by Mark'ing them                   *
 * - temporarily place the template in that cell                           *
 * - try to Unlimbo all the objects that were in the cell. If any          *
 *     objects fail to Unlimbo onto that template, the template cannot     *
 *     be placed here                                                      *
 *                                                                         *
 * It is assumed that the object being placed is a "new" object; the       *
 * object's strength & mission are not set during Unlimbo.                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = OK, -1 = unable to place                                       *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/04/1994 BR : Created.                                              *
 *=========================================================================*/
int MapEditClass::Place_Object() {
  CELL template_cell = 0;    // cell being checked for template
  COORDINATE obj_coord = 0;  // coord of occupier object
  BaseNodeClass node;        // for adding to an AI Base

  /*------------------------------------------------------------------------
  Placing a template:
  - first lift up any objects in the cell
  - place the template, and try to replace the objects; if they won't go
    back, the template can't go there
  ------------------------------------------------------------------------*/
  // ScenarioInit++;
  if (PendingObject->What_Am_I() == RTTI_TEMPLATETYPE) {
    /*
    .......... Loop through all cells this template will occupy ...........
    */
    bool okflag = true;  // OK to place a template?
    const int16_t* occupy =
        PendingObject->Occupy_List();  // ptr into template's OccupyList
    while ((*occupy) != REFRESH_EOL) {
      /*
      ................. Check this cell for an occupier ..................
      */
      template_cell = static_cast<CELL>((ZoneCell + ZoneOffset) + (*occupy));
      if ((*this)[template_cell].Cell_Occupier()) {
        ObjectClass* occupier =
            (*this)[template_cell].Cell_Occupier();  // occupying object

        /*
        .................. Save object's coordinates ....................
        */
        obj_coord = occupier->Coord;

        /*
        ................... Place the object in limbo ...................
        */
        occupier->Mark(MARK_UP);

        /*
        ................ Set the cell's template values .................
        */
        const TemplateType save_ttype =
            (*this)[template_cell].TType;  // for saving cell's TType
        const unsigned char save_ticon =
            (*this)[template_cell].TIcon;  // for saving cell's TIcon
        (*this)[template_cell].TType =
            dynamic_cast<const TemplateTypeClass*>(PendingObject)->Type;
        (*this)[template_cell].TIcon = static_cast<unsigned char>(
            Cell_X(*occupy) +
            (Cell_Y(*occupy) *
             dynamic_cast<const TemplateTypeClass*>(PendingObject)->Width));
        (*this)[template_cell].Recalc_Attributes();
        /*
        ................ Try to put the object back down ................
        */
        if (occupier->Can_Enter_Cell(Coord_Cell(obj_coord)) != MOVE_OK) {
          okflag = false;
        }

        /*
        .............. Put everything back the way it was ...............
        */
        (*this)[template_cell].TType = save_ttype;
        (*this)[template_cell].TIcon = save_ticon;
        (*this)[template_cell].Recalc_Attributes();

        /*
        .......... Major error if can't replace the object now ..........
        */
        occupier->Mark(MARK_DOWN);
      }
      occupy++;
    }

    /*
    ......... If it's still OK after ALL THAT, place the template .........
    */
    if (okflag) {
      if (PendingObjectPtr->Unlimbo(Cell_Coord(static_cast<CELL>(ZoneCell + ZoneOffset)))) {
        /*...............................................................
        Loop through all cells occupied by this template, and clear the
        smudge & overlay.
        ...............................................................*/
        occupy = PendingObject->Occupy_List();
        while ((*occupy) != REFRESH_EOL) {
          /*
          ............... Get cell for this occupy item ................
          */
          template_cell = static_cast<CELL>((ZoneCell + ZoneOffset) + (*occupy));

          /*
          ................... Clear smudge & overlay ...................
          */
          (*this)[template_cell].Overlay = OVERLAY_NONE;
          (*this)[template_cell].OverlayData = 0;
          (*this)[template_cell].Smudge = SMUDGE_NONE;

          /*
          ............ make adjacent cells recalc attrib's .............
          */
          (*this)[template_cell].Recalc_Attributes();
          (*this)[template_cell].Wall_Update();
          (*this)[template_cell].Concrete_Calc();

          occupy++;
        }

        /*
        ......................... Set flags etc .........................
        */
        PendingObjectPtr = nullptr;
        PendingObject = nullptr;
        PendingHouse = HOUSE_NONE;
        Set_Cursor_Shape(nullptr);
        // ScenarioInit--;
        TotalValue = Overpass();
        Flag_To_Redraw(false);
        return 0;
      }

      /*
      **	Failure to deploy results in a returned failure code.
      */
      // ScenarioInit--;
      return (-1);
    }

    /*
    ........................ Not OK; return error .........................
    */
    // ScenarioInit--;
    return (-1);
  }

  /*------------------------------------------------------------------------
  Placing infantry: Infantry can go into cell sub-positions, so find the
  sub-position closest to the mouse & put him there
  ------------------------------------------------------------------------*/
  if (PendingObject->What_Am_I() == RTTI_INFANTRYTYPE) {
    /*
    ....................... Find cell sub-position ........................
    */
    if (Is_Spot_Free(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()))) {
      obj_coord =
          Closest_Free_Spot(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
    } else {
      obj_coord = 0;
    }

    /*
    ................ No free spots; don't place the object ................
    */
    if (obj_coord == 0) {
      // ScenarioInit--;
      return (-1);
    }

    /*
    ......................... Unlimbo the object ..........................
    */
    if (PendingObjectPtr->Unlimbo(obj_coord)) {
      dynamic_cast<InfantryClass*>(PendingObjectPtr)->Set_Occupy_Bit(obj_coord);
      //			Map[Coord_Cell(obj_coord)].Flag.Composite |=
      //				(1 << CellClass::Spot_Index(obj_coord));
      PendingObjectPtr = nullptr;
      PendingObject = nullptr;
      PendingHouse = HOUSE_NONE;
      Set_Cursor_Shape(nullptr);
      // ScenarioInit--;
      return 0;
    }

    // ScenarioInit--;
    return (-1);
  }

  /*------------------------------------------------------------------------
  Placing an object
  ------------------------------------------------------------------------*/
  if (PendingObjectPtr->Unlimbo(Cell_Coord(static_cast<CELL>(ZoneCell + ZoneOffset)))) {
    /*
    ** Update the Tiberium computation if we're placing an overlay
    */
    if (PendingObject->What_Am_I() == RTTI_OVERLAYTYPE &&
        dynamic_cast<const OverlayTypeClass*>(PendingObject)->IsTiberium) {
      TotalValue = Overpass();
      Flag_To_Redraw(false);
    }

    /*
    ** If we're building a base, add this building to the base's Node list.
    */
    if (BaseBuilding && PendingObject->What_Am_I() == RTTI_BUILDINGTYPE) {
      node.Type = dynamic_cast<const BuildingTypeClass*>(PendingObject)->Type;
      node.Coord = PendingObjectPtr->Coord;
      Base.Nodes.Add(node);
    }

    PendingObjectPtr = nullptr;
    PendingObject = nullptr;
    PendingHouse = HOUSE_NONE;
    Set_Cursor_Shape(nullptr);
    // ScenarioInit--;
    return 0;
  }

  return (-1);
}

/***************************************************************************
 * MapEditClass::Cancel_Placement -- cancels placement mode                *
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
 *   11/04/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Cancel_Placement() {
  /*
  ---------------------- Delete the placement object -----------------------
  */
  delete PendingObjectPtr;
  PendingObject = nullptr;
  PendingObjectPtr = nullptr;
  PendingHouse = HOUSE_NONE;

  /*
  -------------------------- Restore cursor shape --------------------------
  */
  Set_Cursor_Shape(nullptr);

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Place_Next -- while placing object, goes to next          *
 *                                                                         *
 * - Deletes the current 'PendingObjectPtr'                                *
 * - Increments LastChoice                                                 *
 * - Tries to create a new 'PendingObjectPtr'; if fails, keeps             *
 *   incrementing until it gets it                                         *
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
 *   11/03/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Next() {
  delete PendingObjectPtr;
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;

  /*
  ------------------ Loop until we create a valid object -------------------
  */
  while (!PendingObjectPtr) {
    /*
    ................. Go to next object in Objects list ...................
    */
    LastChoice++;
    if (LastChoice == ObjCount) {
      /*
      ** If we're in normal placement mode, wrap to the 1st object;
      ** if we're in base-building mode, wrap to the 1st building
      */
      if (!BaseBuilding) {
        LastChoice = 0;
      } else {
        LastChoice = TypeOffset[7];
      }
    }

    /*
    ................... Get house for this object type ....................
    */
    if (!Verify_House(LastHouse, Objects[LastChoice])) {
      /*
      ** If we're in normal placement mode, change the current
      ** placement house to the one that can own this object.
      ** If we're building a base, skip ahead to the next object if the
      ** base's house can't own this one.
      */
      if (!BaseBuilding) {
        LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
      } else {
        continue;
      }
    }

    /*
    ....................... Create placement object .......................
    */
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
    }
  }

  /*
  ------------------------ Set the new cursor shape ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(nullptr);
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Place_Prev -- while placing object, goes to previous      *
 *                                                                         *
 * - Deletes the current 'PendingObjectPtr'                                *
 * - Decrements LastChoice                                                 *
 * - Tries to create a new 'PendingObjectPtr'; if fails, keeps             *
 *   decrementing until it gets it                                         *
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
 *   11/03/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Prev() {
  delete PendingObjectPtr;
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;

  /*
  ------------------ Loop until we create a valid object -------------------
  */
  while (!PendingObjectPtr) {
    /*
    ................. Go to prev object in Objects list ..................
    */
    LastChoice--;
    /*
    ** If we're in normal placement mode, wrap at the 1st object.
    ** If we're building a base, wrap at the 1st building.
    */
    if (!BaseBuilding) {
      if (LastChoice < 0) {
        LastChoice = ObjCount - 1;
      }
    } else {
      if (LastChoice < TypeOffset[7]) {
        LastChoice = ObjCount - 1;
      }
    }

    /*
    ................... Get house for this object type ....................
    */
    if (!Verify_House(LastHouse, Objects[LastChoice])) {
      /*
      ** If we're in normal placement mode, change the current
      ** placement house to the one that can own this object.
      ** If we're building a base, skip ahead to the next object if the
      ** base's house can't own this one.
      */
      if (!BaseBuilding) {
        LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
      } else {
        continue;
      }
    }

    /*
    ....................... Create placement object .......................
    */
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
    }
  }

  /*
  ------------------------ Set the new cursor shape ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(nullptr);
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Place_Next_Category -- places next category of object     *
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
 *   11/03/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Next_Category() {

  /*
  ** Don't allow this command if we're building a base; the only valid
  ** category for base-building is buildings.
  */
  if (BaseBuilding) {
    return;
  }

  delete PendingObjectPtr;
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;

  /*
  ------------------ Go to next category in Objects list -------------------
  */
  int i = LastChoice;
  while (Objects[i]->What_Am_I() == Objects[LastChoice]->What_Am_I()) {
    i++;
    if (i == ObjCount) {
      i = 0;
    }
  }
  LastChoice = i;

  /*
  ------------------ Loop until we create a valid object -------------------
  */
  while (!PendingObjectPtr) {
    /*
    ................... Get house for this object type ....................
    */
    if (!Verify_House(LastHouse, Objects[LastChoice])) {
      LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
    }

    /*
    ....................... Create placement object .......................
    */
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    .................. If this one failed, try the next ...................
    */
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
      LastChoice++;
      if (LastChoice == ObjCount) {
        LastChoice = 0;
      }
    }
  }

  /*
  ------------------------ Set the new cursor shape ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(nullptr);
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Place_Prev_Category -- places previous category of object *
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
 *   11/03/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Prev_Category() {

  /*
  ** Don't allow this command if we're building a base; the only valid
  ** category for base-building is buildings.
  */
  if (BaseBuilding) {
    return;
  }

  delete PendingObjectPtr;
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;

  /*
  ------------------ Go to prev category in Objects list -------------------
  */
  int i = LastChoice;
  /*
  ..................... Scan for the previous category .....................
  */
  while (Objects[i]->What_Am_I() == Objects[LastChoice]->What_Am_I()) {
    i--;
    if (i < 0) {
      i = ObjCount - 1;
    }
  }
  /*
  .................... Scan for start of this category .....................
  */
  LastChoice = i;
  while (Objects[i]->What_Am_I() == Objects[LastChoice]->What_Am_I()) {
    LastChoice = i;
    i--;
    if (i < 0) {
      i = ObjCount - 1;
    }
  }

  /*
  ------------------ Loop until we create a valid object -------------------
  */
  while (!PendingObjectPtr) {
    /*
    ................... Get house for this object type ....................
    */
    if (!Verify_House(LastHouse, Objects[LastChoice])) {
      LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
    }

    /*
    ....................... Create placement object .......................
    */
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    .................. If this one failed, try the next ...................
    */
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
      LastChoice--;
      if (LastChoice < 0) {
        LastChoice = ObjCount - 1;
      }
    }
  }

  /*
  ------------------------ Set the new cursor shape ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(nullptr);
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Place_Home -- homes the placement object                  *
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
 *   11/03/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Home() {
  delete PendingObjectPtr;
  PendingObjectPtr = nullptr;
  PendingObject = nullptr;

  /*
  ** Don't allow this command if we're building a base; the only valid
  ** category for base-building is buildings.
  */
  if (BaseBuilding) {
    return;
  }

  /*
  ------------------ Loop until we create a valid object -------------------
  */
  LastChoice = 0;
  while (!PendingObjectPtr) {
    /*
    ................... Get house for this object type ....................
    */
    if (!Verify_House(LastHouse, Objects[LastChoice])) {
      LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
    }

    /*
    ....................... Create placement object .......................
    */
    PendingObject = Objects[LastChoice];
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    .................. If this one failed, try the next ...................
    */
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
      LastChoice++;
      if (LastChoice == ObjCount) {
        LastChoice = 0;
      }
    }
  }

  /*
  ------------------------ Set the new cursor shape ------------------------
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape(nullptr);
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  ----------------- Redraw the map to erase old leftovers ------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
  Render();
}

/***************************************************************************
 * MapEditClass::Toggle_House -- toggles current placement object's house  *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/04/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Toggle_House() {

  /*
  ** Don't allow this command if we're building a base; the only valid
  ** house for base-building is the one assigned to the base.
  */
  if (BaseBuilding) {
    return;
  }

  /*------------------------------------------------------------------------
  Only techno objects can be owned by a house; return if not a techno
  ------------------------------------------------------------------------*/
  if (!PendingObjectPtr->Is_Techno()) {
    return;
  }

  /*------------------------------------------------------------------------
  Select the house that will own this object
  ------------------------------------------------------------------------*/
  LastHouse = Cycle_House(PendingObjectPtr->Owner(), PendingObject);

  /*------------------------------------------------------------------------
  Change the house
  ------------------------------------------------------------------------*/
  auto* tp = dynamic_cast<TechnoClass*>(PendingObjectPtr);
  tp->House = HouseClass::As_Pointer(LastHouse);

  /*------------------------------------------------------------------------
  Set house variables to new house
  ------------------------------------------------------------------------*/
  PendingHouse = LastHouse;
}

/***************************************************************************
 * MapEditClass::Set_House_Buttons -- toggles house buttons for btn list   *
 *                                                                         *
 * Looks in the given button list for the given GDI, NOD & Neutral button  *
 * id's. Sets the On/Off state of the buttons based on the given house,    *
 * only if that button is found in the list.                               *
 *                                                                         *
 * INPUT:                                                                  *
 *      house            house to set buttons to                           *
 *      btnlist         ptr to button list to search                       *
 *      base_id         button ID for GDI; assumes other id's are sequential*
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/23/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Set_House_Buttons(HousesType house, GadgetClass* btnlist,
                                     int base_id) {

  /*
  **	Loop through all houses, searching the button list for each one.
  */
  for (HousesType h = HOUSE_FIRST; h < HOUSE_COUNT; h++) {
    /*
    **	Compute the desired button ID; get a pointer to the button
    */
    const int id = static_cast<int>(h) + base_id;
    auto* btn = dynamic_cast<TextButtonClass*>(
        btnlist->Extract_Gadget(static_cast<unsigned>(id)));
    if (btn) {
      /*
      **	If this house value is the desired one, turn the button on;
      **	otherwise, turn it off.
      */
      if (h == house) {
        btn->Turn_On();
      } else {
        btn->Turn_Off();
      }
    }
  }
}

/***************************************************************************
 * MapEditClass::Start_Trigger_Placement -- enters trigger placement mode  *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Start_Trigger_Placement() {
  Set_Default_Mouse(MOUSE_CAN_MOVE);
  Override_Mouse_Shape(MOUSE_CAN_MOVE);
}

/***************************************************************************
 * MapEditClass::Stop_Trigger_Placement -- exits trigger placement mode    *
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
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Stop_Trigger_Placement() {
  CurTrigger = nullptr;
  Set_Default_Mouse(MOUSE_NORMAL);
  Override_Mouse_Shape(MOUSE_NORMAL);
}

/***************************************************************************
 * MapEditClass::Place_Trigger -- assigns trigger to object or cell        *
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
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Place_Trigger() {
  ObjectClass* object = nullptr;  // Generic object clicked on.

  /*
  -------------------- See if an object was clicked on ---------------------
  */
  int x = ActiveKeyboard->MouseQX;
  int y = ActiveKeyboard->MouseQY;

  /*
  ............................ Get cell for x,y ............................
  */
  CELL const cell = Click_Cell_Calc(x, y);  // Cell that was selected.

  /*
  ............... Convert x,y to offset from cell upper-left ...............
  */
  x = (x - TacPixelX) % ICON_PIXEL_W;
  y = (y - TacPixelY) % ICON_PIXEL_H;

  /*
  ......................... Get object at that x,y .........................
  */
  object = Cell_Object(cell, x, y);

  /*
  ---------------------- Assign trigger to an object -----------------------
  */
  if (object && TriggerClass::Event_Need_Object(CurTrigger->Event)) {
    object->Trigger = CurTrigger;
  } else {
    /*
    ------------------------ Assign trigger to a cell ------------------------
    */
    if (CurTrigger->Event <= EVENT_OBJECTFIRST) {
      Map[cell].IsTrigger = true;
      CellTriggers[cell] = CurTrigger;
    }
  }

  /*
  -------------------------- Force map to redraw ---------------------------
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
}

/***************************************************************************
 * MapEditClass::Start_Base_Building -- starts base-building mode          *
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
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Start_Base_Building() {
  /*
  ** Fully build the base so the user can edit it
  */
  Build_Base_To(100);

  /*
  ** Start placement mode
  */
  BaseBuilding = true;
  Start_Placement();

  /*
  ** Force map to redraw
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
}

/***************************************************************************
 * MapEditClass::Cancel_Base_Building -- stops base-building mode          *
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
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Cancel_Base_Building() {
  /*
  ** Build the base to the proper amount
  */
  Build_Base_To(BasePercent);

  /*
  ** Cancel placement mode
  */
  Cancel_Placement();
  BaseBuilding = false;

  /*
  ** Force map to redraw
  */
  HiddenPage.Clear();
  Flag_To_Redraw(true);
}

/***************************************************************************
 * MapEditClass::Build_Base_To -- builds the AI base to the given percent  *
 *                                                                         *
 * INPUT:                                                                  *
 *      percent      percentage to build base to                           *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/01/1994 BR : Created.                                              *
 *=========================================================================*/
void MapEditClass::Build_Base_To(int percent) {
  BuildingClass* obj = nullptr;

  // ScenarioInit++;

  /*
  ** Completely dismantle the base, so we start at a known point
  */
  for (int i = 0; i < Base.Nodes.Count(); i++) {
    if (Base.Is_Built(i)) {
      obj = Base.Get_Building(i);
      delete obj;
    }
  }

  /*
  ** Compute number of buildings to build
  */
  const int num_buildings =
      (static_cast<int>(Base.Nodes.Count()) * percent) / 100;

  /*
  ** Build the base to the desired amount
  */
  for (int i = 0; i < num_buildings; i++) {
    /*
    ** Get a ptr to the type of building to build, create one, and unlimbo it.
    */
    const BuildingTypeClass* objtype =
        &BuildingTypeClass::As_Reference(Base.Nodes[i].Type);
    obj = dynamic_cast<BuildingClass*>(
        objtype->Create_One_Of(HouseClass::As_Pointer(Base.House)));
    /*
    ** If unlimbo fails, error out
    */
    if (!obj->Unlimbo(Base.Nodes[i].Coord)) {
      delete obj;
      CCMessageBox().Process("Unable to build base!");
      return;
    }
  }

  // ScenarioInit--;
}
