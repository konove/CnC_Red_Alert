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

/* $Header: /CounterStrike/MAPEDPLC.CPP 1     3/03/97 10:25a Joe_bostic $ */
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
 *                  Last Update : May 12, 1996 [JLB]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Object-placement routines                                               *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   MapEditClass::Build_Base_To -- builds the AI base to the given percent*
 *   MapEditClass::Cancel_Base_Building -- stops base-building mode        *
 *   MapEditClass::Cancel_Placement -- cancels placement mode              *
 *   MapEditClass::Place_Home -- homes the placement object                *
 *   MapEditClass::Place_Next -- while placing object, goes to next        *
 *   MapEditClass::Place_Next_Category -- places next object category      *
 *   MapEditClass::Place_Object -- attempts to place the current object    *
 *   MapEditClass::Place_Prev -- while placing object, goes to previous    *
 *   MapEditClass::Place_Prev_Category -- places previous object category  *
 *   MapEditClass::Place_Trigger -- assigns trigger to object or cell      *
 *   MapEditClass::Placement_Dialog -- adds an object to the scenario      *
 *   MapEditClass::Set_House_Buttons -- toggles house buttons for btn list *
 *   MapEditClass::Start_Base_Building -- starts base-building mode        *
 *   MapEditClass::Start_Placement -- enters placement mode                *
 *   MapEditClass::Start_Trigger_Placement -- enters trigger placement mode*
 *   MapEditClass::Stop_Trigger_Placement -- exits trigger placement mode  *
 *   MapEditClass::Toggle_House -- toggles current placement object's house*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <algorithm>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/conquer.h"
#include "ra/control.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/dialog.h"
#include "ra/display_constants.h"
#include "ra/externs.h"
#include "ra/gadget.h"
#include "ra/globals.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/inline.h"
#include "ra/jshell.h"
#include "ra/list.h"
#include "ra/mapedit.h"
#include "ra/msgbox.h"
#include "ra/object.h"
#include "ra/scenario.h"
#include "ra/techno.h"
#include "ra/tevent.h"
#include "ra/text_ids.h"
#include "ra/textbtn.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/vector_dynamic.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/keyboard.h"
#include "sdllib/ww_mouse.h"
#include "sdllib/ww_win.h"
#include "sdllib/wwstd.h"
#include "tech/mix_archive.h"

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
 *   12/13/1995 JLB : Fixed house buttons to handle expanded house list.   *
 *   05/12/1996 JLB : Handles hi-res.                                      *
 *=========================================================================*/
int MapEditClass::Placement_Dialog() {
  HousesType house = HOUSE_NONE;
  RemapControlType* scheme = GadgetClass::Get_Color_Scheme();

  /*
  **	Dialog & button dimensions
  */
  constexpr int kDialogW = 400;
  constexpr int kDialogH = 180;
  constexpr int kDialogX = 0;
  constexpr int kDialogY = 0;
  constexpr int kTxt8H = 11;
  constexpr int kMargin = 7;
  constexpr int kPictureW = 152;  // must be divisible by 8!
  constexpr int kPictureH = 105;
  constexpr int kPictureX = kDialogX + 35;  // must start on a byte boundary!
  constexpr int kPictureY = kDialogY + kMargin + kTxt8H + kMargin;
  constexpr int kPictureCx = kPictureX + (kPictureW / 2);
  constexpr int kGdiX = kPictureX + kPictureW + 5;
  constexpr int kGdiY = kPictureY;
  constexpr int kLeftW = 45;
  constexpr int kLeftH = 9;
  constexpr int kLeftX = kPictureCx - 5 - kLeftW;
  constexpr int kLeftY = kPictureY + kPictureH + kMargin;
  constexpr int kRightW = 45;
  constexpr int kRightH = 9;
  constexpr int kRightX = kPictureCx + 5;
  constexpr int kRightY = kPictureY + kPictureH + kMargin;
  constexpr int kTemplateW = 70;
  constexpr int kTemplateH = 9;
  constexpr int kTemplateX = kDialogX + kDialogW - kMargin - kTemplateW - 30;
  constexpr int kTemplateY = kPictureY;
  constexpr int kOverlayW = 70;
  constexpr int kOverlayH = 9;
  constexpr int kOverlayX = kDialogX + kDialogW - kMargin - kOverlayW - 30;
  constexpr int kOverlayY = kTemplateY + kTemplateH;
  constexpr int kSmudgeW = 70;
  constexpr int kSmudgeH = 9;
  constexpr int kSmudgeX = kDialogX + kDialogW - kMargin - kSmudgeW - 30;
  constexpr int kSmudgeY = kOverlayY + kOverlayH;
  constexpr int kTerrainW = 70;
  constexpr int kTerrainH = 9;
  constexpr int kTerrainX = kDialogX + kDialogW - kMargin - kTerrainW - 30;
  constexpr int kTerrainY = kSmudgeY + kSmudgeH;
  constexpr int kUnitW = 70;
  constexpr int kUnitH = 9;
  constexpr int kUnitX = kDialogX + kDialogW - kMargin - kUnitW - 30;
  constexpr int kUnitY = kTerrainY + kTerrainH;
  constexpr int kInfantryW = 70;
  constexpr int kInfantryH = 9;
  constexpr int kInfantryX = kDialogX + kDialogW - kMargin - kInfantryW - 30;
  constexpr int kInfantryY = kUnitY + kUnitH;
  constexpr int kAircraftW = 70;
  constexpr int kAircraftH = 9;
  constexpr int kAircraftX = kDialogX + kDialogW - kMargin - kAircraftW - 30;
  constexpr int kAircraftY = kInfantryY + kInfantryH;
  constexpr int kBuildingW = 70;
  constexpr int kBuildingH = 9;
  constexpr int kBuildingX = kDialogX + kDialogW - kMargin - kBuildingW - 30;
  constexpr int kBuildingY = kAircraftY + kAircraftH;
  constexpr int kAirW = 70;
  constexpr int kAirH = 9;
  constexpr int kAirX = kDialogX + kDialogW - kMargin - kAirW - 30;
  constexpr int kAirY = kBuildingY + kBuildingH;
  constexpr int kOkW = 45;
  constexpr int kOkH = 9;
  constexpr int kOkX = kPictureCx - kOkW - 5;
  constexpr int kOkY = kDialogY + kDialogH - kOkH - kMargin - 15;
  constexpr int kCancelW = 45;
  constexpr int kCancelH = 9;
  constexpr int kCancelX = kPictureCx + 5;
  constexpr int kCancelY = kDialogY + kDialogH - kCancelH - kMargin - 15;
  constexpr int kGridsize = 10;
  constexpr int kGridblockW = 3;
  constexpr int kGridblockH = 3;
  constexpr int kGridX =
      kDialogX + kDialogW - (kGridsize * kGridblockW) - kMargin - 35;
  constexpr int kGridY =
      kDialogY + kDialogH - (kGridsize * kGridblockH) - kMargin - 35;

  /*
  **	Button enumerations:
  */
  constexpr int kButtonHouse = 101;
  constexpr int kButtonNext = 102;
  constexpr int kButtonPrev = 103;
  constexpr int kButtonOk = 104;
  constexpr int kButtonCancel = 105;
  constexpr int kButtonTemplate = 106;
  constexpr int kButtonOverlay = 107;
  constexpr int kButtonSmudge = 108;
  constexpr int kButtonTerrain = 109;
  constexpr int kButtonUnit = 110;
  constexpr int kButtonInfantry = 111;
  constexpr int kButtonAircraft = 112;
  constexpr int kButtonBuilding = 113;
  constexpr int kButtonAir = 114;

  /*
  **	Dialog variables
  */
  bool cancel = false;            // true = user cancels
  int x = 0;
  int y = 0;  // for drawing the grid
  int i = 0;
  int typeindex = 0;  // index of class type

  /*
  **	Buttons
  */

  ListClass housebtn(kButtonHouse, kGdiX, kGdiY, 60, 8 * 16,
                     TPF_EFNT | TPF_NOSHADOW,
                     MixArchive::RetrieveData("EBTN-UP.SHP"),
                     MixArchive::RetrieveData("EBTN-DN.SHP"));
  for (const HousesType each_house : magic_enum::enum_values<HousesType>()) {
    housebtn.Add_Item(HouseTypeClass::As_Reference(each_house).IniName);
  }

  TextButtonClass nextbtn(kButtonNext, TXT_RIGHT, kTpfEButton, kRightX, kRightY,
                          kRightW, kRightH);
  TextButtonClass prevbtn(kButtonPrev, TXT_LEFT, kTpfEButton, kLeftX, kLeftY,
                          kLeftW, kLeftH);
  TextButtonClass okbtn(kButtonOk, "OK", kTpfEButton, kOkX, kOkY, kOkW, kOkH);
  TextButtonClass cancelbtn(kButtonCancel, "Cancel", kTpfEButton, kCancelX,
                            kCancelY, kCancelW, kCancelH);
  TextButtonClass templatebtn(kButtonTemplate, "Template", kTpfEButton,
                              kTemplateX, kTemplateY, kTemplateW, kTemplateH);
  TextButtonClass overlaybtn(kButtonOverlay, "Overlay", kTpfEButton, kOverlayX,
                             kOverlayY, kOverlayW, kOverlayH);
  TextButtonClass smudgebtn(kButtonSmudge, "Smudge", kTpfEButton, kSmudgeX,
                            kSmudgeY, kSmudgeW, kSmudgeH);
  TextButtonClass terrainbtn(kButtonTerrain, "Terrain", kTpfEButton, kTerrainX,
                             kTerrainY, kTerrainW, kTerrainH);
  TextButtonClass unitbtn(kButtonUnit, "Unit", kTpfEButton, kUnitX, kUnitY,
                          kUnitW, kUnitH);
  TextButtonClass infantrybtn(kButtonInfantry, "Infantry", kTpfEButton,
                              kInfantryX, kInfantryY, kInfantryW, kInfantryH);
  TextButtonClass aircraftbtn(kButtonAircraft, "Ships", kTpfEButton, kAircraftX,
                              kAircraftY, kAircraftW, kAircraftH);
  TextButtonClass buildingbtn(kButtonBuilding, "Building", kTpfEButton,
                              kBuildingX, kBuildingY, kBuildingW, kBuildingH);
  TextButtonClass airbtn(kButtonAir, "Aircraft", kTpfEButton, kAirX, kAirY,
                         kAirW, kAirH);

  /*
  **	Initialize addable objects list; we must do this every time in case one
  **	of the object pools has become exhausted; that object won't be available
  **	for adding.  (Skip aircraft, since they won't be used in the editor.)
  */
  Clear_List();
  TemplateTypeClass::Prep_For_Add();
  OverlayTypeClass::Prep_For_Add();
  SmudgeTypeClass::Prep_For_Add();
  TerrainTypeClass::Prep_For_Add();
  UnitTypeClass::Prep_For_Add();
  InfantryTypeClass::Prep_For_Add();
  VesselTypeClass::Prep_For_Add();
  BuildingTypeClass::Prep_For_Add();
  AircraftTypeClass::Prep_For_Add();

  /*
  **	Compute offset of each class type in the Objects array
  */
  TypeOffset[0] = 0;
  for (i = 1; i < kNumEditClasses; i++) {
    base::At(TypeOffset, i) =
        base::At(TypeOffset, i - 1) + base::At(NumType, i - 1);
  }

  /*
  **	Return if no objects to place
  */
  if (!ObjCount) {
    return (-1);
  }

  /*
  **	Initialize
  */
  Set_Logic_Page(SeenBuff);
  if (LastChoice >= ObjCount) {
    LastChoice = 0;
  }
  const ObjectTypeClass* curobj =
      base::At(Objects, LastChoice);  // Working object pointer.  // current
                                      // object to choose

  ControlClass* commands = &nextbtn;
  housebtn.Add_Tail(*commands);
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
  airbtn.Add_Tail(*commands);

  /*
  **	Make sure the recorded house selection matches the house list
  **	box selection.
  */
  LastHouse = HousesType(housebtn.Current_Index());

  /*
  **	Main processing loop
  */
  bool display = true;
  bool process = true;
  while (process) {
    /*
    **	Invoke game callback
    */
    Call_Back();

    /*
    **	Refresh display if needed
    */
    if (display) {
      /*
      **	Display the dialog box
      */
      Hide_Mouse();
      Dialog_Box(kDialogX, kDialogY, kDialogW, kDialogH);
      Draw_Caption(TXT_PLACE_OBJECT, kDialogX, kDialogY, kDialogW);

      /*
      **	Display the current object:
      **	- save the current window dimensions
      **	- adjust the window size to the actual drawable area
      **	- draw the shape
      **	- reset the window dimensions
      */
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowX) =
          kPictureX;
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowY) =
          kPictureY;
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowWidth) =
          kPictureW;
      base::At(WindowList[static_cast<int>(WINDOW_EDITOR)], kWindowHeight) =
          kPictureH;
      Change_Window(static_cast<int>(WINDOW_EDITOR));
      Draw_Box(kPictureX, kPictureY, kPictureW, kPictureH, BOXSTYLE_DOWN,
               false);
      curobj->Display(ScreenWidth / 2, ScreenHeight / 2, WINDOW_EDITOR,
                      LastHouse);
      //			curobj->Display(WinW<<2, WinH>>1, WINDOW_EDITOR,
      // LastHouse);

      /*
      **	Erase the grid
      */
      LogicPage->Fill_Rect(kGridX - (kGridblockW * 2), kGridY,
                           kGridX + (kGridsize * kGridblockW),
                           kGridY + (kGridsize * kGridblockH), kBlack);

      /*
      **	Draw a box for every cell occupied
      */
      std::span<const int16_t> occupy =
          curobj->Occupy_List();  // ptr into object's OccupyList
      while (occupy.front() != kRefreshEol) {
        const int cell = occupy.front();  // cell index for parsing OccupyList
        occupy = occupy.subspan(1);
        x = kGridX + ((cell % MAP_CELL_W) * kGridblockW);
        y = kGridY + ((cell / MAP_CELL_W) * kGridblockH);
        LogicPage->Fill_Rect(x, y, x + kGridblockW - 1, y + kGridblockH - 1,
                             scheme->Bright);
      }

      /*
      **	Draw the grid itself
      */
      for (y = 0; y <= kGridsize; y++) {
        for (x = 0; x <= kGridsize; x++) {
          LogicPage->Draw_Line(
              kGridX + (x * kGridblockW), kGridY, kGridX + (x * kGridblockW),
              kGridY + (kGridsize * kGridblockH), scheme->Shadow);
        }
        LogicPage->Draw_Line(kGridX, kGridY + (y * kGridblockH),
                             kGridX + (kGridsize * kGridblockW),
                             kGridY + (y * kGridblockH), scheme->Shadow);
      }

      /*
      **	Print the object's label from the class's Full_Name().
      **	Warning: Text_String returns an EMS pointer, so standard string
      **	functions won't work!
      */
      Fancy_Text_Print(curobj->Full_Name(), kPictureCx, kPictureY + kMargin,
                       scheme, kTBlack, TPF_CENTER | TPF_EFNT | TPF_NOSHADOW);

      /*
      **	Redraw buttons
      **	Figure out which class category we're in & highlight that button
      **	This updates 'typeindex', which is used below, and it also
      *updates *	the category button states.
      */
      i = 0;
      for (typeindex = 0; typeindex < kNumEditClasses; typeindex++) {
        i += base::At(NumType, typeindex);
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
      airbtn.Turn_Off();
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

        case kButtonAir:
          airbtn.Turn_On();
          break;

        case kButtonBuilding:
          buildingbtn.Turn_On();
          break;
        default:
          break;
      }

      /*
      **	Redraw buttons
      */
      commands->Draw_All();
      Show_Mouse();
      display = false;
    }

    /*
    **	Get user input
    */
    const KeyNumType input = commands->Input();  // user input

    /*
    **	Process user input
    */
    switch (static_cast<int>(input)) {
      /*
      **	GDI House
      */
      case ButtonKey(kButtonHouse):
        house = HousesType(housebtn.Current_Index());

        /*
        **	Set flags & buttons
        */
        LastHouse = house;
        display = true;
        break;

      /*
      **	Next in list
      */
      case KN_RIGHT:
      case ButtonKey(kButtonNext):
        /*
        **	Increment to next obj
        */
        LastChoice++;
        if (LastChoice == ObjCount) {
          LastChoice = 0;
        }
        curobj = base::At(Objects, LastChoice);

        nextbtn.Turn_Off();
        display = true;
        break;

      /*
      **	Previous in list
      */
      case KN_LEFT:
      case ButtonKey(kButtonPrev):

        /*
        **	Decrement to prev obj
        */
        LastChoice--;
        if (LastChoice < 0) {
          LastChoice = ObjCount - 1;
        }
        curobj = base::At(Objects, LastChoice);
        prevbtn.Turn_Off();
        display = true;
        break;

      /*
      **	Select a class type
      */
      case ButtonKey(kButtonTemplate):
      case ButtonKey(kButtonOverlay):
      case ButtonKey(kButtonSmudge):
      case ButtonKey(kButtonTerrain):
      case ButtonKey(kButtonUnit):
      case ButtonKey(kButtonInfantry):
      case ButtonKey(kButtonAircraft):
      case ButtonKey(kButtonBuilding):
      case ButtonKey(kButtonAir):

        /*
        **	Find index of class
        */
        typeindex = input - ButtonKey(kButtonTemplate);

        /*
        **	If no objects of that type, do nothing
        */
        if (base::At(NumType, typeindex) == 0) {
          display = true;
          break;
        }

        /*
        **	Set current object
        */
        LastChoice = base::At(TypeOffset, typeindex);
        curobj = base::At(Objects, LastChoice);
        display = true;
        break;

      /*
      **	Next category
      */
      case KN_PGDN:
        typeindex++;
        if (typeindex == kNumEditClasses) {
          typeindex = 0;
        }

        /*
        **	Set current object
        */
        LastChoice = base::At(TypeOffset, typeindex);
        curobj = base::At(Objects, LastChoice);
        display = true;
        break;

      /*
      **	Previous category
      */
      case KN_PGUP:
        typeindex--;
        if (typeindex < 0) {
          typeindex = kNumEditClasses - 1;
        }

        /*
        **	Set current object
        */
        LastChoice = base::At(TypeOffset, typeindex);
        curobj = base::At(Objects, LastChoice);
        display = true;
        break;

      /*
      **	Jump to 1st choice
      */
      case KN_HOME:
        LastChoice = 0;

        /*
        **	Set current object
        */
        curobj = base::At(Objects, LastChoice);
        display = true;
        break;

      /*
      **	OK
      */
      case KN_RETURN:
      case ButtonKey(kButtonOk):
        cancel = false;
        process = false;
        break;

      /*
      **	Cancel
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
  **	Redraw the display
  */
  HidPage.Clear();
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
  /*
  **	Initialize addable objects list; we must do this every time in case one
  **	of the object pools has become exhausted; that object won't be available
  **	for adding. These must be added in the same order expected by the
  **	object selection dialog (same as button order).
  */
  Clear_List();
  TemplateTypeClass::Prep_For_Add();
  OverlayTypeClass::Prep_For_Add();
  SmudgeTypeClass::Prep_For_Add();
  TerrainTypeClass::Prep_For_Add();
  UnitTypeClass::Prep_For_Add();
  InfantryTypeClass::Prep_For_Add();
  VesselTypeClass::Prep_For_Add();
  BuildingTypeClass::Prep_For_Add();
  AircraftTypeClass::Prep_For_Add();

  /*
  **	Compute offset of each class type in the Objects array
  */
  TypeOffset[0] = 0;
  for (int i = 1; i < kNumEditClasses; i++) {
    base::At(TypeOffset, i) =
        base::At(TypeOffset, i - 1) + base::At(NumType, i - 1);
  }

  /*
  **	Create the placement object:
  **	- For normal placement mode, use the last-used index into Objects
  **	  (LastChoice), and the last-used house (LastHouse).
  **	- For base-building mode, force the object to be a building, and use the
  **	  House specified in the Base object
  */
  if (!BaseBuilding) {
    if (LastChoice >= ObjCount) {
      LastChoice = ObjCount - 1;
    }
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(LastHouse));
  } else {
    LastChoice = std::clamp(LastChoice, TypeOffset[7], ObjCount - 1);
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse = Base.House;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(LastHouse));
  }

  /*
  **	Error if no more objects available
  */
  if (!PendingObjectPtr) {
    WWMessageBox().Process("No more objects of this type available.");
    HidPage.Clear();
    Flag_To_Redraw(true);
    Render();
    PendingObject = nullptr;
    if (BaseBuilding) {
      Cancel_Base_Building();
    }
    return;
  }

  /*
  **	Set the placement cursor
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
                             //	BaseNodeClass node;
                             //// for adding to an AI Base

  /*
  **	Placing a template:
  **	- first lift up any objects in the cell
  **	- place the template, and try to replace the objects; if they won't go
  **	  back, the template can't go there
  */
  // ScenarioInit++;
  if (PendingObject->What_Am_I() == RTTI_TEMPLATETYPE) {
    /*
    **	Loop through all cells this template will occupy
    */
    bool okflag = true;  // OK to place a template?
    std::span<const int16_t> occupy =
        PendingObject->Occupy_List();  // ptr into template's OccupyList
    while (occupy.front() != kRefreshEol) {
      /*
      **	Check this cell for an occupier
      */
      template_cell =
          static_cast<CELL>((ZoneCell + ZoneOffset) + occupy.front());
      if ((*this).at(template_cell).Cell_Occupier()) {
        ObjectClass* occupier =
            (*this).at(template_cell).Cell_Occupier();  // occupying object

        /*
        **	Save object's coordinates
        */
        obj_coord = occupier->Coord;

        /*
        **	Place the object in limbo
        */
        occupier->Mark(MARK_UP);

        /*
        **	Set the cell's template values
        */
        const TemplateType save_ttype =
            (*this).at(template_cell).TType;  // for saving cell's TType
        const unsigned char save_ticon =
            (*this).at(template_cell).TIcon;  // for saving cell's TIcon
        (*this).at(template_cell).TType =
            dynamic_cast<const TemplateTypeClass*>(PendingObject)->Type;
        (*this).at(template_cell).TIcon = static_cast<unsigned char>(
            Cell_X(occupy.front()) +
            (Cell_Y(occupy.front()) *
             dynamic_cast<const TemplateTypeClass*>(PendingObject)->Width));
        (*this).at(template_cell).Recalc_Attributes();

        /*
        **	Try to put the object back down
        */
        if (occupier->Can_Enter_Cell(Coord_Cell(obj_coord)) != MOVE_OK) {
          okflag = false;
        }

        /*
        **	Put everything back the way it was
        */
        (*this).at(template_cell).TType = save_ttype;
        (*this).at(template_cell).TIcon = save_ticon;
        (*this).at(template_cell).Recalc_Attributes();

        /*
        **	Major error if can't replace the object now
        */
        occupier->Mark(MARK_DOWN);
      }
      occupy = occupy.subspan(1);
    }

    /*
    **	If it's still OK after ALL THAT, place the template
    */
    if (okflag) {
      if (PendingObjectPtr->Unlimbo(Cell_Coord(static_cast<CELL>(ZoneCell + ZoneOffset)))) {
        /*
        **	Loop through all cells occupied by this template, and clear the
        **	smudge & overlay.
        */
        occupy = PendingObject->Occupy_List();
        while (occupy.front() != kRefreshEol) {
          /*
          **	Get cell for this occupy item
          */
          template_cell =
              static_cast<CELL>((ZoneCell + ZoneOffset) + occupy.front());

          /*
          **	Clear smudge & overlay
          */
          (*this).at(template_cell).Overlay = OVERLAY_NONE;
          (*this).at(template_cell).OverlayData = 0;
          (*this).at(template_cell).Smudge = SMUDGE_NONE;

          /*
          **	make adjacent cells recalc attrib's
          */
          (*this).at(template_cell).Recalc_Attributes();
          (*this).at(template_cell).Wall_Update();
          (*this).at(template_cell).Concrete_Calc();

          occupy = occupy.subspan(1);
        }

        /*
        **	Set flags etc
        */
        PendingObjectPtr = nullptr;
        PendingObject = nullptr;
        PendingHouse = HOUSE_NONE;
        Set_Cursor_Shape({});
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
    **	Not OK; return error
    */
    // ScenarioInit--;
    return (-1);
  }

  /*
  **	Placing infantry: Infantry can go into cell sub-positions, so find the
  **	sub-position closest to the mouse & put him there
  */
  if (PendingObject->What_Am_I() == RTTI_INFANTRYTYPE) {
    /*
    **	Find cell sub-position
    */
    if (Is_Spot_Free(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()))) {
      obj_coord =
          Closest_Free_Spot(Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
    } else {
      obj_coord = 0;
    }

    /*
    **	No free spots; don't place the object
    */
    if (obj_coord == 0) {
      // ScenarioInit--;
      return (-1);
    }

    /*
    **	Unlimbo the object
    */
    if (PendingObjectPtr->Unlimbo(obj_coord)) {
      dynamic_cast<InfantryClass*>(PendingObjectPtr)->Set_Occupy_Bit(obj_coord);
      //			Map[obj_coord].Flag.Composite |=
      //				(1 << CellClass::Spot_Index(obj_coord));
      PendingObjectPtr = nullptr;
      PendingObject = nullptr;
      PendingHouse = HOUSE_NONE;
      Set_Cursor_Shape({});
      // ScenarioInit--;
      return 0;
    }

    // ScenarioInit--;
    return (-1);
  }

  /*
  **	Placing an object
  */
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
      //			node.Type = ((BuildingTypeClass
      //*)PendingObject)->Type; 			node.Cell =
      // Coord_Cell(PendingObjectPtr->Coord);
      Base.Nodes.Add(BaseNodeClass(
          dynamic_cast<const BuildingTypeClass*>(PendingObject)->Type,
          Coord_Cell(PendingObjectPtr->Coord)));
    }

    PendingObjectPtr = nullptr;
    PendingObject = nullptr;
    PendingHouse = HOUSE_NONE;
    Set_Cursor_Shape({});
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
  **	Delete the placement object
  */
  delete PendingObjectPtr;
  PendingObject = nullptr;
  PendingObjectPtr = nullptr;
  PendingHouse = HOUSE_NONE;

  /*
  **	Restore cursor shape
  */
  Set_Cursor_Shape({});

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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
  **	Loop until we create a valid object
  */
  while (!PendingObjectPtr) {
    /*
    **	Go to next object in Objects list
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
    **	Create placement object
    */
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
    }
  }

  /*
  **	Set the new cursor shape
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape({});
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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
  **	Loop until we create a valid object
  */
  while (!PendingObjectPtr) {
    /*
    **	Go to prev object in Objects list
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
    **	Create placement object
    */
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));
    if (!PendingObjectPtr) {
      PendingObject = nullptr;
    }
  }

  /*
  **	Set the new cursor shape
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape({});
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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
  **	Go to next category in Objects list
  */
  int i = LastChoice;
  while (base::At(Objects, i)->What_Am_I() ==
         base::At(Objects, LastChoice)->What_Am_I()) {
    i++;
    if (i == ObjCount) {
      i = 0;
    }
  }
  LastChoice = i;

  /*
  **	Loop until we create a valid object
  */
  while (!PendingObjectPtr) {
    /*
    **	Get house for this object type
    */
    //		if (!Verify_House(LastHouse, Objects[LastChoice])) {
    //			LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
    //		}

    /*
    **	Create placement object
    */
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    **	If this one failed, try the next
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
  **	Set the new cursor shape
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape({});
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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
  **	Go to prev category in Objects list
  */
  int i = LastChoice;

  /*
  **	Scan for start of this category
  */
  while (base::At(Objects, i)->What_Am_I() ==
         base::At(Objects, LastChoice)->What_Am_I()) {
    i--;
    if (i < 0) {
      i = ObjCount - 1;
    }
  }

  i--;
  if (i < 0) {
    i = ObjCount - 1;
  }
  LastChoice = i;

  /*
  **	Scan for the previous category
  */
  while (base::At(Objects, i)->What_Am_I() ==
         base::At(Objects, LastChoice)->What_Am_I()) {
    i--;
    if (i < 0) {
      i = ObjCount - 1;
    }
  }

  i++;
  if (i >= ObjCount) {
    i = 0;
  }
  LastChoice = i;

  /*
  **	Loop until we create a valid object
  */
  while (!PendingObjectPtr) {
    /*
    **	Get house for this object type
    */
    //		if (!Verify_House(LastHouse, Objects[LastChoice])) {
    //			LastHouse = Cycle_House(LastHouse, Objects[LastChoice]);
    //		}

    /*
    **	Create placement object
    */
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    **	If this one failed, try the next
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
  **	Set the new cursor shape
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape({});
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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
  **	Loop until we create a valid object
  */
  LastChoice = 0;
  while (!PendingObjectPtr) {
    /*
    **	Get house for this object type
    */
    if (!Verify_House(LastHouse, base::At(Objects, LastChoice))) {
      LastHouse = Cycle_House(LastHouse, base::At(Objects, LastChoice));
    }

    /*
    **	Create placement object
    */
    PendingObject = base::At(Objects, LastChoice);
    PendingHouse = LastHouse;
    PendingObjectPtr =
        PendingObject->Create_One_Of(HouseClass::As_Pointer(PendingHouse));

    /*
    **	If this one failed, try the next
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
  **	Set the new cursor shape
  */
  Set_Cursor_Pos();
  Set_Cursor_Shape({});
  Set_Cursor_Shape(PendingObject->Occupy_List());

  /*
  **	Redraw the map to erase old leftovers
  */
  HidPage.Clear();
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

  /*
  **	Only techno objects can be owned by a house; return if not a techno
  */
  if (!PendingObjectPtr->Is_Techno()) {
    return;
  }

  /*
  **	Select the house that will own this object
  */
  LastHouse = Cycle_House(PendingObjectPtr->Owner(), PendingObject);

  /*
  **	Change the house
  */
  auto* tp = dynamic_cast<TechnoClass*>(PendingObjectPtr);
  tp->House = HouseClass::As_Pointer(LastHouse);

  /*
  **	Set house variables to new house
  */
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
 *   01/26/1996 JLB : Uses new house selection list method.                *
 *=========================================================================*/
void MapEditClass::Set_House_Buttons(HousesType house, GadgetClass* /*unused*/,
                                     int /*unused*/)
// void MapEditClass::Set_House_Buttons(HousesType house, GadgetClass * btnlist,
// int base_id)
{
  HouseList->Set_Selected_Index(static_cast<int>(house));
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
  **	See if an object was clicked on
  */
  int x = Keyboard->MouseQX;
  int y = Keyboard->MouseQY;

  /*
  **	Get cell for x,y
  */
  CELL const cell = Click_Cell_Calc(x, y);  // Cell that was selected.

  /*
  **	Convert x,y to offset from cell upper-left
  */
  x = (x - TacPixelX) % ICON_PIXEL_W;
  y = (y - TacPixelY) % ICON_PIXEL_H;

  /*
  **	Get object at that x,y
  */
  object = Cell_Object(cell, x, y);

  /*
  **	Assign trigger to an object
  */
  const AttachType a1 = CurTrigger->Attaches_To();
  if (object && base::Any(a1 & ATTACH_OBJECT)) {
    if (CurTrigger) {
      TriggerClass* tt = Find_Or_Make(CurTrigger);
      if (tt) {
        object->Trigger = tt;
      }
    }
  } else {
    /*
    **	Assign trigger to a cell
    */
    if (base::Any(a1 & ATTACH_CELL) && CurTrigger) {
      TriggerClass* tt = Find_Or_Make(CurTrigger);
      Map.at(cell).Trigger = tt;
    }
    //			CellTriggers[cell] = CurTrigger;
  }

  /*
  **	Force map to redraw
  */
  HidPage.Clear();
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
  HidPage.Clear();
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
  Build_Base_To(Scen.Percent);

  /*
  ** Cancel placement mode
  */
  Cancel_Placement();
  BaseBuilding = false;

  /*
  ** Force map to redraw
  */
  HidPage.Clear();
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
        &BuildingTypeClass::As_Reference(Base.Nodes.at(i).Type);
    obj = dynamic_cast<BuildingClass*>(
        objtype->Create_One_Of(HouseClass::As_Pointer(Base.House)));

    /*
    ** If unlimbo fails, error out
    */
    ScenarioInit++;
    if (!obj->Unlimbo(Cell_Coord(Base.Nodes.at(i).Cell))) {
      delete obj;
      WWMessageBox().Process("Unable to build base!");
      ScenarioInit--;
      return;
    }
    ScenarioInit--;
  }

  // ScenarioInit--;
}
