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

/* $Header: /CounterStrike/INFANTRY.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : INFANTRY.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : August 15, 1994 *
 *                                                                                             *
 *                  Last Update : August 15, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_INFANTRY_H_
#define CNC_RED_ALERT_RA_INFANTRY_H_

#include <cstddef>

#include "ra/bullet.h"
#include "ra/ccini.h"
#include "ra/ccptr.h"
#include "ra/cell.h"
#include "ra/coord.h"
#include "ra/defines.h"
#include "ra/face.h"
#include "ra/foot.h"
#include "ra/jshell.h"
#include "ra/monoc.h"
#include "ra/object.h"
#include "ra/techno.h"
#include "ra/type.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"
#include "tech/pipe.h"
#include "tech/straw.h"

class InfantryClass : public FootClass {
 public:
  CCPtr<InfantryTypeClass> Class;

  /*
  **	If the infantry is undergoing some choreographed animation sequence,
  *then *	this holds the particular sequence number. The frame of
  *animation is kept *	track of by the regular frame tracking system. When
  *performing an animation *	sequence, the infantry cannot perform anything
  *else (even move).
  */
  DoType Doing;

  /*
  **	Certain infantry will either perform some comment or say something after
  *an *	amount of time has expired subsequent to an significant event. This is
  *the *	timer the counts down.
  */
  CDTimerClass<FrameTimerClass> Comment;

  /*
  **	If this civilian is actually a technician, then this flag will be true.
  **	It should only be set for the civilian type infantry. Typically, the
  **	technician appears after a building is destroyed.
  */
  unsigned IsTechnician : 1;

  /*
  **	If the infantry just performed some feat, then it may respond with an
  *action. *	This flag will be true if an action is to be performed when the
  *Comment timer *	has expired.
  */
  unsigned IsStoked : 1;

  /*
  **	This flag indicates if the infantry unit is prone. Prone infantry become
  *that way *	when they are fired upon. Infantry in the prone position are
  *less vulnerable to *	combat.
  */
  unsigned IsProne : 1;

  /*
  **	If the infantry is allowed to move one cell from one zone to another,
  *then this *	flag will be true. It exists only so that when a bridge is
  *destroyed, the bomb *	placer is allowed to run from the destroyed
  *bridge cell back onto a real cell.
  */
  unsigned IsZoneCheat : 1;

  /*
  ** This flag is set for the dogs, when they launch into bullet mode.
  ** it's to remember if the unit was selected, and if it was, then
  ** when the dog is re-enabled, he'll reselect himself.
  */
  unsigned WasSelected : 1;

  /*
  **	The fear rating of this infantry unit. The more afraid the infantry, the
  *more *	likely it is to panic and seek cover.
  */
  FearType Fear;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) noexcept;
  void* operator new(size_t, void* ptr) noexcept { return ptr; }
  void operator delete(void* ptr);
  InfantryClass(InfantryType classid, HousesType house);
  InfantryClass(const NoInitClass& x) : FootClass(x), Class(x), Comment(x) {}
  ~InfantryClass() override;
  operator InfantryType() const { return Class->Type; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  void Assign_Destination(TARGET) override;

  /*
  **	Query functions.
  */
  bool Is_Ready_To_Random_Animate() const override;
  const void* Get_Image_Data() const override;
  int Shape_Number() const;
  const ObjectTypeClass& Class_Of() const override;
  int Full_Name() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE coord, DirType facing) override;
  bool Paradrop(COORDINATE coord) override;
  bool Limbo() override;
  void Detach(TARGET target, bool all) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  const short* Overlap_List(bool redraw = false) const override;
  void Draw_It(int x, int y, WindowNumberType window) const override;

  /*
  **	User I/O.
  */
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, CELL cell) override {
    FootClass::Active_Click_With(action, cell);
  }

  /*
  **	Combat related.
  */
  ActionType What_Action(const ObjectClass* object) const override;
  ActionType What_Action(CELL cell) const override;
  BulletClass* Fire_At(TARGET target, int which) override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr,
                         bool forced = false) override;
  FireErrorType Can_Fire(TARGET target, int which) const override;
  void Assign_Target(TARGET) override;
  void Set_Occupy_Bit(COORDINATE coord) {
    Set_Occupy_Bit(Coord_Cell(coord), CellClass::Spot_Index(coord));
  }
  void Set_Occupy_Bit(CELL cell, int spot_index);
  void Clear_Occupy_Bit(COORDINATE coord) {
    Clear_Occupy_Bit(Coord_Cell(coord), CellClass::Spot_Index(coord));
  }
  void Clear_Occupy_Bit(CELL cell, int spot_index);

  /*
  **	Driver control support functions. These are used to control cell
  **	occupation flags and driver instructions.
  */
  bool Stop_Driver() override;
  bool Start_Driver(COORDINATE& coord) override;

  /*
  **	AI.
  */
  void AI() override;
  void Fear_AI();
  TARGET Greatest_Threat(ThreatType threat) override;  // const;
  int Mission_Attack() override;
  bool Edge_Of_World_AI();
  void Firing_AI();
  void Doing_AI();
  void Movement_AI();

  /*
  **	Scenario and debug support.
  */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static void Write_INI(CCINIClass& ini);
  static const char* INI_Name() { return "INFANTRY"; }
  bool Load(Straw& file);
  bool Save(Pipe& file) const;

  /*
  **	Movement and animation.
  */
  virtual bool Do_Action(DoType todo, bool force = false);
  bool Random_Animate() override;
  MoveType Can_Enter_Cell(CELL, FacingType = FACING_NONE) const override;
  void Per_Cell_Process(PCPType why) override;
  void Enter_Idle_Mode(bool initial = false) override;
  void Scatter(COORDINATE threat, bool forced = false,
               bool nokidding = false) override;

  /*
  **	Translation table to convert facing into infantry shape number. This
  *special *	table is needed since several facing stages are reused and
  *flipped about the Y *	axis.
  */
  static const int HumanShape[32];

 private:
  static const DoStruct MasterDoControls[DO_COUNT];
};

#endif  // CNC_RED_ALERT_RA_INFANTRY_H_
