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

/* $Header:   F:\projects\c&c\vcs\code\infantry.h_v   2.18   16 Oct 1995
 * 16:48:08   JOE_BOSTIC  $ */
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

#ifndef INFANTRY_H
#define INFANTRY_H

#include <cstddef>

#include "td/bullet.h"
#include "td/cell.h"
#include "td/defines.h"
#include "td/foot.h"
#include "td/ftimer.h"
#include "td/inline.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/techno.h"
#include "td/type.h"
#include "tech/noinit.h"
#include "tech/wwfile.h"

/**********************************************************************
**	Infantry can be afraid. These defines are for the various infantry
**	fear levels. When infantry be come scared enough they take cover and
**	even run away in panic.
*/
#define FEAR_ANXIOUS 10   // Something makes them scared.
#define FEAR_SCARED 100   // Scared enough to take cover.
#define FEAR_PANIC 200    // Run away! Run away!
#define FEAR_MAXIMUM 255  // Scared to death.

class InfantryClass : public FootClass {
 public:
  InfantryTypeClass const* const Class;
  operator InfantryType() const { return Class->Type; }

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
  *an *	amount of time has expired subsiquent to an significant event. This is
  *the *	timer the counts down.
  */
  TCountDownTimerClass Comment;

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
  ** This flag is set when the infantryman is engaged in hand-to-hand
  ** combat.  By setting this flag, it'll play the put-down-the-gun
  ** sequence only once, and it'll know to pick up the gun when the
  ** fight is over.
  */
  unsigned IsBoxing : 1;

  /*
  **	The fear rating of this infantry unit. The more afraid the infantry, the
  *more *	likely it is to panic and seek cover.
  */
  unsigned char Fear;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(size_t size) throw();
  void* operator new(size_t, void* ptr) throw() { return ptr; }
  void operator delete(void* ptr);
  InfantryClass();
  InfantryClass(InfantryType classid, HousesType house);
  InfantryClass(NoInitClass const& x)
      : FootClass(x), Class(Class), Comment(x) {}
  ~InfantryClass() override;
  RTTIType What_Am_I() const override;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  static void Init();

  void Assign_Destination(TARGET) override;

  /*
  **	Query functions.
  */
  bool Is_Infantry() const override;
  ObjectTypeClass const& Class_Of() const override;
  int Full_Name() const override;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  COORDINATE Fire_Coord(int which) const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(COORDINATE coord, DirType facing) override;
  bool Limbo() override;
  void Detach(TARGET target, bool all) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  short const* Overlap_List() const override;
  void Draw_It(int x, int y, WindowNumberType window) override;
  void Look(bool incremental = false) override;

  /*
  **	User I/O.
  */
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;

  /*
  **	Combat related.
  */
  virtual int Made_A_Kill();
  ActionType What_Action(ObjectClass* object) override;
  ActionType What_Action(CELL cell) const override;
  void Assign_Mission(MissionType order) override;
  BulletClass* Fire_At(TARGET target, int which) override;
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source = nullptr) override;
  TARGET As_Target() const override;
  FireErrorType Can_Fire(TARGET target, int which) const override;
  void Assign_Target(TARGET) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;
  int Rearm_Delay(bool second) const override;
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
  TARGET Greatest_Threat(ThreatType threat) const override;
  int Mission_Attack() override;

/*
**	Scenario and debug support.
*/
#ifdef CHEAT_KEYS
  virtual void Debug_Dump(MonoClass* mono) const;
#endif

  /*
  **	File I/O.
  */
  static void Read_INI(char* buffer);
  static void Write_INI(char* buffer);
  static char const* INI_Name() { return "INFANTRY"; }
  bool Load(FileClass& file);
  bool Save(FileClass& file);
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	Movement and animation.
  */
  virtual bool Do_Action(DoType todo, bool force = false);
  void Random_Animate() override;
  MoveType Can_Enter_Cell(CELL, FacingType = FACING_NONE) const override;
  void Per_Cell_Process(bool center) override;
  void Enter_Idle_Mode(bool initial = false) override;
  void Scatter(COORDINATE threat, bool forced = false) override;

  /*
  **	Dee-buggin' support.
  */
  int Validate() const;

  /*
  **	Translation table to convert facing into infantry shape number. This
  *special *	table is needed since several facing stages are reused and
  *flipped about the Y *	axis.
  */
  static int const HumanShape[32];

 private:
  static DoStruct const MasterDoControls[DO_COUNT];

  /*
  ** This contains the value of the Virtual Function Table Pointer
  */
  static void* VTable;
};

#endif
