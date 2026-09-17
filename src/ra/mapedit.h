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

/* $Header: /CounterStrike/MAPEDIT.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MAPEDIT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : May 14, 1994 *
 *                                                                                             *
 *                  Last Update : May 14, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *	This class is derived from the normal display map class. It exists
 ** only to allow editing and adding items to the map.
 **
 *---------------------------------------------------------------------------------------------*
 * House-setting functions: The editor contains several house maintenance
 *routines:				  * Verify_House: tells if the given
 *ObjectType can be owned by the given HousesType * Cycle_House: Finds the next
 *valid house for the given ObjectType; used when a new object	  * can't be
 *owned by the current editor HousesType.
 ** Change_House: attempts to change the owner of the currently-selected object
 ** Toggle_House: cycles the HousesType of a pending placement object
 ** Set_House_Buttons: sets house buttons in accordance with the given
 *HousesType					  *
 *---------------------------------------------------------------------------------------------*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_MAPEDIT_H_
#define CNC_RED_ALERT_RA_MAPEDIT_H_

#include <array>

#include "ra/ccini.h"
#include "ra/control.h"
#include "ra/defines.h"
#include "ra/dial8.h"
#include "ra/face.h"
#include "ra/gadget.h"
#include "ra/gauge.h"
#include "ra/list.h"
#include "ra/mouse.h"
#include "ra/object.h"
#include "ra/teamtype.h"
#include "ra/textbtn.h"
#include "ra/txtlabel.h"
#include "ra/type.h"
#include "sdllib/keyboard.h"

/*
**	This is the maximum # of ObjectTypeClasses the editor has to deal with.
*/
inline constexpr int kMaxEditObjects =
    static_cast<int>(magic_enum::enum_count<TemplateType>()) +
    static_cast<int>(magic_enum::enum_count<OverlayType>()) +
    static_cast<int>(magic_enum::enum_count<SmudgeType>()) +
    static_cast<int>(magic_enum::enum_count<TerrainType>()) +
    static_cast<int>(magic_enum::enum_count<UnitType>()) +
    static_cast<int>(magic_enum::enum_count<InfantryType>()) +
    static_cast<int>(magic_enum::enum_count<VesselType>()) +
    static_cast<int>(magic_enum::enum_count<StructType>());
inline constexpr int kMaxTeamClasses =
    static_cast<int>(magic_enum::enum_count<UnitType>()) +
    static_cast<int>(magic_enum::enum_count<InfantryType>()) +
    static_cast<int>(magic_enum::enum_count<AircraftType>()) +
    static_cast<int>(magic_enum::enum_count<VesselType>());
//	NUM_EDIT_MISSIONS = 6,			// # missions that can be
// assigned an object
inline constexpr int kNumEditClasses =
    9;  // # different classes (templates, terrain, etc)
inline constexpr int kMaxMainMenuNum = 8;
inline constexpr int kMaxMainMenuLen = 20;
inline constexpr int kMaxAiMenuNum = 6;
inline constexpr int kMaxAiMenuLen = 20;
inline constexpr int kPopupHouseX = 10;
inline constexpr int kPopupHouseY = 100;
inline constexpr int kPopupHouseW = 60;
inline constexpr int kPopupHouseH = 190 - 100;
//	POPUP_GDI_W = 50,
//	POPUP_GDI_H = 9,
//	POPUP_GDI_X = 10,
//	POPUP_GDI_Y = 160,
//	POPUP_NOD_W = 50,
//	POPUP_NOD_H = 9,
//	POPUP_NOD_X = 10,
//	POPUP_NOD_Y = 169,
//	POPUP_NEUTRAL_W = 50,
//	POPUP_NEUTRAL_H = 9,
//	POPUP_NEUTRAL_X = 10,
//	POPUP_NEUTRAL_Y = 178,
//	POPUP_MULTI1_W = 25,
//	POPUP_MULTI1_H = 9,
//	POPUP_MULTI1_X = 10,
//	POPUP_MULTI1_Y = 160,
//	POPUP_MULTI2_W = 25,
//	POPUP_MULTI2_H = 9,
//	POPUP_MULTI2_X = 35,
//	POPUP_MULTI2_Y = 160,
//	POPUP_MULTI3_W = 25,
//	POPUP_MULTI3_H = 9,
//	POPUP_MULTI3_X = 10,
//	POPUP_MULTI3_Y = 169,
//	POPUP_MULTI4_W = 25,
//	POPUP_MULTI4_H = 9,
//	POPUP_MULTI4_X = 35,
//	POPUP_MULTI4_Y = 169,
inline constexpr int kPopupMissionW = 80;
inline constexpr int kPopupMissionH = 40;
inline constexpr int kPopupMissionX = 70;
inline constexpr int kPopupMissionY = 150;
inline constexpr int kPopupFaceboxW = 30;
inline constexpr int kPopupFaceboxH = 30;
inline constexpr int kPopupFaceboxX = 160;
inline constexpr int kPopupFaceboxY = 160;
inline constexpr int kPopupHealthW = 50;
inline constexpr int kPopupHealthH = 10;
inline constexpr int kPopupHealthX = 200;
inline constexpr int kPopupHealthY = 170;
inline constexpr int kPopupBaseW = 50;
inline constexpr int kPopupBaseH = 8;
inline constexpr int kPopupBaseX = 300 - 50;
inline constexpr int kPopupBaseY = 0;

/*
**	These are the button ID's for the pop-up object-editing gizmos.
**	The house button ID's must be sequential, with a 1-to-1 correspondence
*to *	the HousesType values.
*/
inline constexpr int kPopupSpain = 500;
inline constexpr int kPopupFirst = kPopupSpain;
inline constexpr int kPopupGreece = 501;
inline constexpr int kPopupUssr = 502;
inline constexpr int kPopupEngland = 503;
inline constexpr int kPopupItaly = 504;
inline constexpr int kPopupGermany = 505;
inline constexpr int kPopupFrance = 506;
inline constexpr int kPopupTurkey = 507;
inline constexpr int kPopupHouselist = 508;    // House selection list.
inline constexpr int kPopupSellable = 509;     // Allowed to sell.
inline constexpr int kPopupRebuildable = 510;  // Allowed to rebuild.
inline constexpr int kPopupMissionlist = 511;  // list box for missions
inline constexpr int kPopupHealthgauge = 512;  // health of object
inline constexpr int kPopupFacingdial = 513;   // object's facing
inline constexpr int kPopupBasepercent = 514;  // Base's percent-built slider
inline constexpr int kMapArea = 515;           // map as a click-able thingy
inline constexpr unsigned kButtonFlag = 0x8000;

class MapEditClass : public MouseClass {
  /*
  **	Public Interface
  */
 public:
  /*
  **	mapedit.cpp
  */
  MapEditClass();
  static bool Get_Waypoint_Name(std::span<char> wayptname);
  void Update_Waypoint(int waypt_idx);

  void One_Time() override;  // One-time init
  void Init_IO() override;   // Inits button list
  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool forced = true) override;
  bool Scroll_Map(DirType facing, int& distance, bool really = true) override;
  void Read_INI(CCINIClass& ini) override;
  void Detach(ObjectClass* object) override;
  using MouseClass::Detach;
  void Clear_List();
  bool Add_To_List(const ObjectTypeClass* object);
  void Main_Menu();
  void AI_Menu();
  bool Mouse_Moved();
  static bool Verify_House(HousesType house, const ObjectTypeClass* objtype);
  static HousesType Cycle_House(HousesType curhouse,
                                const ObjectTypeClass* objtype);
  //		int Trigger_Needs_Team(TriggerClass *trigger);
  [[noreturn]] static void Fatal(int txt);

  /*
  **	mapeddlg.cpp
  */
  int New_Scenario();
  int Load_Scenario();
  int Save_Scenario();
  int Pick_Scenario(const char* caption, int& scen_nump,
                    ScenarioPlayerType& playerp, ScenarioDirType& dirp,
                    ScenarioVarType& varp);
  int Size_Map(int x, int y, int w, int h);
  int Scenario_Dialog();
  void Handle_Triggers();
  int Select_Trigger();

  /*
  **	mapedplc.cpp
  */
  int Placement_Dialog();
  void Start_Placement();
  int Place_Object();
  void Cancel_Placement();
  void Place_Next();
  void Place_Prev();
  void Place_Next_Category();
  void Place_Prev_Category();
  void Place_Home();
  void Toggle_House();
  void Set_House_Buttons(HousesType house, GadgetClass* btnlist, int base_id);
  void Start_Trigger_Placement();
  void Stop_Trigger_Placement();
  void Place_Trigger();
  void Start_Base_Building();
  void Cancel_Base_Building();
  static void Build_Base_To(int percent);

  /*
  **	mapedsel.cpp
  */
  int Select_Object();
  void Select_Next();
  void Popup_Controls();
  void Grab_Object();
  int Move_Grabbed_Object();
  static bool Change_House(HousesType newhouse);

  /*
  **	mapedtm.cpp
  */
  static void Draw_Member(const TechnoTypeClass* ptr, int index, int quant,
                          HousesType house);
  void Handle_Teams(const char* caption);
  int Select_Team(const char* caption);
  int Team_Members(HousesType house);

  /*
  **	Private Interface
  */
 private:
  /*
  **	This is the last-requested variation of a loaded/saved/new scenario.
  */
  //		ScenarioVarType ScenVar;

  /*
  **	Array of all TypeClasses the user can add to the map; cleared by
  **	Clear_List(), added to by Add_To_List()
  */
  const ObjectTypeClass* Objects[kMaxEditObjects]{};
  int ObjCount{0};  // # of objects in the Objects array

  /*
  **	Last-selected object to place, and last-selected house of object
  */
  int LastChoice{0};                 // index of item user picked last
  HousesType LastHouse{HOUSE_GOOD};  // house of last item picked

  /*
  **	Variables for grabbing/moving objects
  */
  ObjectClass* GrabbedObject{nullptr};  // object "grabbed" with mouse
  CELL GrabOffset = 0;          // offset to grabbed obj's upper-left
  int64_t LastClickTime = 0;  // time of last LMOUSE click

  /*
  **	Number of each type of object in Objects, so we can switch categories
  */
  int NumType[kNumEditClasses]{};  // # of each type of class:
                                   // 0 = Template
                                   // 1 = Overlay
                                   // 2 = Smudge
                                   // 3 = Terrain
                                   // 4 = Unit
                                   // 5 = Infantry
                                   // 6 = Vessels
                                   // 7 = Building
                                   // 8 = Aircraft

  /*
  **	The offset of each type of object within the Objects[] array
  */
  int TypeOffset[kNumEditClasses]{};  // offsets within Objects[]

  /*
  **	The "current" trigger for point-and-click trigger setting
  */
  TriggerTypeClass* CurTrigger;  // current trigger

  /*
  **	The "current" team type for editing & associating with a trigger
  */
  TeamTypeClass* CurTeam;  // current team

  /*
  **	Bitfields for flags & such
  */
  bool Changed : 1;       // 1 = changes are unsaved
  bool LMouseDown : 1;    // 1 = left mouse is held down
  bool BaseBuilding : 1;  // 1 = we're in base-building mode

  /*
  **	Variables for pre-building a base
  */
  //		int BasePercent;
  //// Percentage the base will be built

  /*
  **	Variables for supporting the object-editing controls at screen bottom
  */
  ListClass* HouseList = nullptr;
  ListClass* MissionList = nullptr;
  TriColorGaugeClass* HealthGauge = nullptr;
  Dial8Class* FacingDial = nullptr;
  ControlClass* MapArea = nullptr;
  TextLabelClass* HealthText = nullptr;
  TextButtonClass* Sellable = nullptr;
  TextButtonClass* Rebuildable = nullptr;
  static char HealthBuf[20];
  GaugeClass* BaseGauge = nullptr;
  TextLabelClass* BaseLabel = nullptr;
  static constexpr std::array MapEditMissions = {
      MISSION_GUARD,   MISSION_STICKY,     MISSION_HARMLESS,
      MISSION_HARVEST, MISSION_GUARD_AREA, MISSION_RETURN,
      MISSION_AMBUSH,  MISSION_HUNT,       MISSION_SLEEP,
  };
};

// Switches between scenario-editor mode and normal game mode. Both modes need
// a different button layout and a full redraw, so this cannot just set a flag.
void Go_Editor(bool flag);

#endif  // CNC_RED_ALERT_RA_MAPEDIT_H_
