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

/* $Header:   F:\projects\c&c\vcs\code\mapedit.h_v   2.19   16 Oct 1995 16:46:36
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_MAPEDIT_H_
#define CNC_RED_ALERT_TD_MAPEDIT_H_

#include <array>
#include <span>

#include "sdllib/keyboard.h"
#include "td/control.h"
#include "td/defines.h"
#include "td/dial8.h"
#include "td/gadget.h"
#include "td/gauge.h"
#include "td/list.h"
#include "td/mouse.h"
#include "td/object.h"
#include "td/teamtype.h"
#include "td/textbtn.h"
#include "td/trigger.h"
#include "td/txtlabel.h"
#include "td/type.h"

/*
********************************** Defines **********************************
*/
/*...........................................................................
This is the maximum # of ObjectTypeClasses the editor has to deal with.
...........................................................................*/
inline constexpr int kMaxEditObjects =
    static_cast<int>(TEMPLATE_COUNT) + static_cast<int>(OVERLAY_COUNT) +
    static_cast<int>(SMUDGE_COUNT) + static_cast<int>(TERRAIN_COUNT) +
    static_cast<int>(UNIT_COUNT) + static_cast<int>(INFANTRY_COUNT) +
    static_cast<int>(AIRCRAFT_COUNT) +
    static_cast<int>(STRUCT_COUNT);  // max # of ObjectTypeClasses allowed

inline constexpr int kMaxTeamClasses =
    static_cast<int>(UNIT_COUNT) + static_cast<int>(INFANTRY_COUNT) +
    static_cast<int>(AIRCRAFT_COUNT);  // max # ObjectTypeClasses for a team

//	NUM_EDIT_MISSIONS = 6,			// # missions that can be
// assigned an object

inline constexpr int kNumEditClasses =
    8;  // # different classes (templates, terrain, etc)

inline constexpr int kMaxMainMenuNum = 8;
inline constexpr int kMaxMainMenuLen = 20;

inline constexpr int kMaxAiMenuNum = 6;
inline constexpr int kMaxAiMenuLen = 20;

inline constexpr int kPopupGdiW = 100;
inline constexpr int kPopupGdiH = 18;
inline constexpr int kPopupGdiX = 20;
inline constexpr int kPopupGdiY = 320;

inline constexpr int kPopupNodW = 100;
inline constexpr int kPopupNodH = 18;
inline constexpr int kPopupNodX = 20;
inline constexpr int kPopupNodY = 338;

inline constexpr int kPopupNeutralW = 100;
inline constexpr int kPopupNeutralH = 18;
inline constexpr int kPopupNeutralX = 20;
inline constexpr int kPopupNeutralY = 356;

inline constexpr int kPopupMulti1W = 50;
inline constexpr int kPopupMulti1H = 18;
inline constexpr int kPopupMulti1X = 20;
inline constexpr int kPopupMulti1Y = 320;

inline constexpr int kPopupMulti2W = 50;
inline constexpr int kPopupMulti2H = 18;
inline constexpr int kPopupMulti2X = 70;
inline constexpr int kPopupMulti2Y = 320;

inline constexpr int kPopupMulti3W = 50;
inline constexpr int kPopupMulti3H = 18;
inline constexpr int kPopupMulti3X = 20;
inline constexpr int kPopupMulti3Y = 330;

inline constexpr int kPopupMulti4W = 50;
inline constexpr int kPopupMulti4H = 18;
inline constexpr int kPopupMulti4X = 70;
inline constexpr int kPopupMulti4Y = 338;

inline constexpr int kPopupMissionW = 160;
inline constexpr int kPopupMissionH = 80;
inline constexpr int kPopupMissionX = 140;
inline constexpr int kPopupMissionY = 300;

inline constexpr int kPopupFaceboxW = 60;
inline constexpr int kPopupFaceboxH = 60;
inline constexpr int kPopupFaceboxX = 320;
inline constexpr int kPopupFaceboxY = 320;

inline constexpr int kPopupHealthW = 100;
inline constexpr int kPopupHealthH = 20;
inline constexpr int kPopupHealthX = 400;
inline constexpr int kPopupHealthY = 340;

inline constexpr int kPopupBaseW = 100;
inline constexpr int kPopupBaseH = 16;
inline constexpr int kPopupBaseX = 600 - kPopupBaseW;
inline constexpr int kPopupBaseY = 0;

/*...........................................................................
These are the button ID's for the pop-up object-editing gizmos.
The house button ID's must be sequential, with a 1-to-1 correspondence to
the HousesType values.
...........................................................................*/
inline constexpr int kPopupGdi = 500;          // GDI house button
inline constexpr int kPopupNod = 501;          // NOD house button
inline constexpr int kPopupNeutral = 502;      // Neutral house button
inline constexpr int kPopupHouseJp = 503;      // not used
inline constexpr int kPopupMulti1 = 504;       // Multiplayer 1 house button
inline constexpr int kPopupMulti2 = 505;       // Multiplayer 2 house button
inline constexpr int kPopupMulti3 = 506;       // Multiplayer 3 house button
inline constexpr int kPopupMulti4 = 507;       // Multiplayer 4 house button
inline constexpr int kPopupMulti5 = 508;       // Multiplayer 4 house button
inline constexpr int kPopupMulti6 = 509;       // Multiplayer 4 house button
inline constexpr int kPopupMissionlist = 510;  // list box for missions
inline constexpr int kPopupHealthgauge = 511;  // health of object
inline constexpr int kPopupFacingdial = 512;   // object's facing
inline constexpr int kPopupBasepercent = 513;  // Base's percent-built slider
inline constexpr int kMapArea = 514;           // map as a click-able thingy
inline constexpr int kButtonFlag = 0x8000;

/*
***************************** Class Declaration *****************************
*/
class MapEditClass : public MouseClass {
  /*
  ---------------------------- Public Interface ----------------------------
  */
 public:
  /*
  ............................. mapedit.cpp .............................
  */
  MapEditClass();
  void One_Time() override;  // One-time init
  void Init_IO() override;   // Inits button list
  void AI(KeyNumType& input, int x, int y) override;
  void Draw_It(bool forced = true) override;
  bool Scroll_Map(DirType facing, int& distance, bool really = true) override;
  //		virtual void Flag_To_Redraw(bool complete);
  void Read_INI(char* buffer) override;
  void Write_INI(std::span<char> buffer) override;
  void Detach(ObjectClass* object) override;
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
  ............................ mapeddlg.cpp .............................
  */
  int New_Scenario();
  int Load_Scenario();
  int Save_Scenario();
  int Pick_Scenario(const char* caption, int* scen_nump,
                    ScenarioPlayerType* playerp, ScenarioDirType* dirp,
                    ScenarioVarType* varp, int multi);
  int Size_Map(int x, int y, int w, int h);
  int Scenario_Dialog();
  void Handle_Triggers();
  int Select_Trigger();
  int Edit_Trigger();
  int Import_Triggers();
  int Import_Teams();
  /*
  ............................ mapedplc.cpp .............................
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
  static void Set_House_Buttons(HousesType house, GadgetClass* btnlist,
                                int base_id);
  void Start_Trigger_Placement();
  void Stop_Trigger_Placement();
  void Place_Trigger();
  void Start_Base_Building();
  void Cancel_Base_Building();
  static void Build_Base_To(int percent);

  /*
  ............................ mapedsel.cpp .............................
  */
  int Select_Object();
  void Select_Next();
  void Popup_Controls();
  void Grab_Object();
  int Move_Grabbed_Object();
  static bool Change_House(HousesType newhouse);

  /*
  ............................. mapedtm.cpp .............................
  */
  static void Draw_Member(const TechnoTypeClass* ptr, int index, int quant,
                          HousesType house, int pic_x, int pic_y);
  void Handle_Teams(const char* caption);
  int Select_Team(const char* caption);
  int Edit_Team();
  int Team_Members(HousesType house);
  static void Build_Mission_List(
      int missioncount,
      const TeamMissionStruct (&missions)[TeamTypeClass::kMaxTeamMissions],
      char (&missionbuf)[TeamTypeClass::kMaxTeamMissions][20], ListClass* list);

  /*
  --------------------------- Private Interface ----------------------------
  */
 private:
  /*.....................................................................
  This is the last-requested variation of a loaded/saved/new scenario.
  .....................................................................*/
  ScenarioVarType ScenVar{SCEN_VAR_A};

  /*.....................................................................
  Array of all TypeClasses the user can add to the map; cleared by
  Clear_List(), added to by Add_To_List()
  .....................................................................*/
  const ObjectTypeClass* Objects[kMaxEditObjects]{};
  int ObjCount{0};  // # of objects in the Objects array

  /*.....................................................................
  Last-selected object to place, and last-selected house of object
  .....................................................................*/
  int LastChoice{0};                 // index of item user picked last
  HousesType LastHouse{HOUSE_GOOD};  // house of last item picked

  /*.....................................................................
  Variables for grabbing/moving objects
  .....................................................................*/
  ObjectClass* GrabbedObject{nullptr};  // object "grabbed" with mouse
  CELL GrabOffset = 0;          // offset to grabbed obj's upper-left
  int64_t LastClickTime = 0;  // time of last LMOUSE click

  /*.....................................................................
  Number of each type of object in Objects, so we can switch categories
  .....................................................................*/
  int NumType[kNumEditClasses]{};  // # of each type of class:
                                   // 0 = Template
                                   // 1 = Overlay
                                   // 2 = Smudge
                                   // 3 = Terrain
                                   // 4 = Unit
                                   // 5 = Infantry
                                   // 6 = Aircraft
                                   // 7 = Building

  /*.....................................................................
  The offset of each type of object within the Objects[] array
  .....................................................................*/
  int TypeOffset[kNumEditClasses]{};  // offsets within Objects[]

  /*.....................................................................
  The "current" trigger for point-and-click trigger setting
  .....................................................................*/
  TriggerClass* CurTrigger;  // current trigger

  /*.....................................................................
  The "current" team type for editing & associating with a trigger
  .....................................................................*/
  TeamTypeClass* CurTeam = nullptr;  // current team

  /*.....................................................................
  Bitfields for flags & such
  .....................................................................*/
  bool Changed : 1;       // 1 = changes are unsaved
  bool LMouseDown : 1;    // 1 = left mouse is held down
  bool BaseBuilding : 1;  // 1 = we're in base-building mode

  /*.....................................................................
  Variables for pre-building a base
  .....................................................................*/
  int BasePercent;  // Percentage the base will be built

  /*.....................................................................
  Variables for supporting the object-editing controls at screen bottom
  .....................................................................*/
  TextButtonClass* GDIButton = nullptr;
  TextButtonClass* NODButton = nullptr;
  TextButtonClass* NeutralButton = nullptr;
  TextButtonClass* Multi1Button = nullptr;
  TextButtonClass* Multi2Button = nullptr;
  TextButtonClass* Multi3Button = nullptr;
  TextButtonClass* Multi4Button = nullptr;
  ListClass* MissionList = nullptr;
  TriColorGaugeClass* HealthGauge = nullptr;
  Dial8Class* FacingDial = nullptr;
  ControlClass* MapArea = nullptr;
  TextLabelClass* HealthText = nullptr;
  static char HealthBuf[20];
  GaugeClass* BaseGauge = nullptr;
  TextLabelClass* BaseLabel = nullptr;
  static constexpr std::array MapEditMissions = {
      MISSION_GUARD,  MISSION_STICKY, MISSION_HARVEST, MISSION_GUARD_AREA,
      MISSION_RETURN, MISSION_AMBUSH, MISSION_HUNT,    MISSION_SLEEP,
  };
};

#endif  // CNC_RED_ALERT_TD_MAPEDIT_H_
