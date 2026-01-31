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

/* $Header:   F:\projects\c&c\vcs\code\techno.h_v   2.17   16 Oct 1995 16:46:58
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TECHNO.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 14, 1994 *
 *                                                                                             *
 *                  Last Update : April 14, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef TECHNO_H
#define TECHNO_H

#include "td/bullet.h"
#include "td/cargo.h"
#include "td/crew.h"
#include "td/defines.h"
#include "td/door.h"
#include "td/facing.h"
#include "td/flasher.h"
#include "td/house.h"
#include "td/monoc.h"
#include "td/object.h"
#include "td/radio.h"
#include "td/stage.h"
#include "td/type.h"
#include "tech/noinit.h"

/****************************************************************************
**	This is the common data between building and units.
*/
class TechnoClass : public RadioClass,
                    public FlasherClass,
                    public StageClass,
                    public CargoClass,
                    public DoorClass,
                    public CrewClass {
 public:
  /*
  **	This flag will be true if the object has been damaged with malace.
  **	Damage received due to friendly fire or wear and tear does not count.
  **	The computer is not allowed to sell a building unless it has been
  **	damaged with malace.
  */
  unsigned IsTickedOff : 1;

  /*
  **	If this object has inherited the ability to cloak, then this bit will
  **	be set to true.
  */
  unsigned IsCloakable : 1;

  /*
  **	If this object is designated as special then this flag will be true. For
  **	buildings, this means that it is the primary factory. For units, it
  *means *	that the unit is the team leader.
  */
  unsigned IsLeader : 1;

  /*
  **	Certain units are flagged as "loaners".  These units are typically
  *transports that *	are created solely for the purpose of delivering
  *reinforcements.  Such "loaner" *	units are not owned by the player and
  *thus cannot be directly controlled.  These *	units will leave the game as
  *soon as they have fulfilled their purpose.
  */
  unsigned IsALoaner : 1;

  /*
  **	Once a unit enters the map, then this flag is set. This flag is used to
  *make *	sure that a unit doesn't leave the map once it enters the map.
  */
  unsigned IsLocked : 1;

  /*
  **	Buildings and units with turrets usually have a recoil animation when
  *they *	fire. If this flag is true, then the next rendering of the
  *object will be *	in the "recoil state". The flag will then be cleared
  *pending the next *	firing event.
  */
  unsigned IsInRecoilState : 1;

  /*
  **	If this unit is "loosely attached" to another unit it is given special
  **	processing. A unit is in such a condition when it is in the process of
  **	unloading from a transport type object. During the unloading process
  **	the transport object must stay still until the unit is free and clear.
  **	At that time it radios the transport object and the "tether" is broken -
  **	freeing both the unit and the transport object.
  */
  unsigned IsTethered : 1;

  /*
  **	Is this object owned by the player?  If not, then it is owned by the
  *computer *	or remote opponent. This flag facilitates the many logic
  *differences when dealing *	with player's or computer's units or buildings.
  */
  unsigned IsOwnedByPlayer : 1;

  /*
  **	The more sophisticated game objects must keep track of whether they are
  *discovered *	or not. This is because the state of discovery can often control
  *how the object *	behaves. In addition, this fact is used in radar and
  *user I/O processing.
  */
  unsigned IsDiscoveredByPlayer : 1;

  /*
  **	This is used to control the computer recognizing this object.
  */
  unsigned IsDiscoveredByComputer : 1;

  /*
  **	Some game objects can be of the "lemon" variety. This means that they
  *take damage *	even when everything is ok. This adds a little variety
  *to the game.
  */
  unsigned IsALemon : 1;

  /*
  **	This flag is used to control second shot processing for those units or
  *buildings *	that fire two shots in quick succession. When this flag is true,
  *it indicates that *	the second shot is ready to fire. After this shot is
  *fired, regular rearm timing *	is used rather than the short rearm
  *time.
  */
  unsigned IsSecondShot : 1;

  /*
  **	This is the house that the unit belongs to.
  */
  HouseClass* House;

  /*
  **	This records the current cloak state for this vehicle.
  */
  CloakType Cloak;
  StageClass CloakingDevice;

  /* (Targeting Computer)
  **	This is the target value for the item that this vehicle should ATTACK.
  *If this *	is a vehicle with a turret, then it may differ from its movement
  *destination.
  */
  TARGET TarCom;
  TARGET SuspendedTarCom;

  /*
  **	This is the visible facing for the unit or building.
  */
  FacingClass PrimaryFacing;

  /*
  **	This is the arming countdown. It represents the time necessary
  **	to reload the weapon.
  */
  unsigned char Arm;

  /*
  **	The number of shot this object can fire before running out of ammo. If
  *this *	value is zero, then firing is not allowed. If -1, then there is
  *no ammunition *	limit.
  */
  int Ammo;

  /*
  **	This is the amount of money spent to produce this object. This value
  *really *	only comes into play for the case of buildings that have special
  *"free" *	objects available when purchased at the more expensive rate.
  */
  int PurchasePrice;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  TechnoClass();
  TechnoClass(HousesType house);
  TechnoClass(const NoInitClass& x)
      : RadioClass(x),
        FlasherClass(x),
        StageClass(x),
        CargoClass(x),
        DoorClass(x),
        CloakingDevice(x),
        PrimaryFacing(x) {}
  ~TechnoClass() override {}

  /*
  **	Query functions.
  */
  virtual int Refund_Amount() const;
  virtual CELL Find_Exit_Cell(const TechnoClass* techno) const;
  virtual BuildingClass* Find_Docking_Bay(StructType b, bool friendly) const;
  virtual int Threat_Range(int control) const;
  virtual InfantryType Crew_Type() const;
  const TechnoTypeClass* Techno_Type_Class() const {
    return dynamic_cast<const TechnoTypeClass*>(&Class_Of());
  }
  CELL Nearby_Location(const TechnoClass* from = nullptr) const;
  unsigned char Get_Ownable() const override;
  bool Can_Player_Fire() const override;
  bool Can_Player_Move() const override;
  virtual bool Is_Weapon_Equipped() const;
  bool Can_Repair() const override;
  bool Is_Techno() const override;
  HousesType Owner() const override;
  virtual int Risk() const;
  int Value() const override;
  virtual int Rearm_Delay(bool second = true) const;
  ActionType What_Action(ObjectClass* target) override;
  ActionType What_Action(CELL cell) const override;
  virtual int Tiberium_Load() const;
  virtual DirType Desired_Load_Dir(ObjectClass*, CELL& moveto) const;
  virtual int Pip_Count() const;
  virtual DirType Fire_Direction() const;

  /*
  **	User I/O.
  */
  void Clicked_As_Target(int count = 7) override;
  bool Select() override;
  virtual void Response_Select();
  virtual void Response_Move();
  virtual void Response_Attack();
  virtual void Player_Assign_Mission(MissionType order,
                                     TARGET target = kTargetNone,
                                     TARGET destination = kTargetNone);

  /*
  **	Combat related.
  */
  void Base_Is_Attacked(const TechnoClass* enemy);
  void Kill_Cargo(TechnoClass* source);
  void Record_The_Kill(TechnoClass* source) override;
  virtual bool Target_Something_Nearby(ThreatType threat = THREAT_NORMAL);
  virtual void Stun();
  bool In_Range(COORDINATE coord, int which = 0) const override;
  virtual bool In_Range(TARGET target, int which = 0) const;
  virtual bool In_Range(const ObjectClass* target, int which = 0) const;
  virtual void Death_Announcement(
      const TechnoClass* source = nullptr) const = 0;
  virtual FireErrorType Can_Fire(TARGET target, int which = 0) const;
  virtual TARGET Greatest_Threat(ThreatType threat) const;
  virtual void Assign_Target(TARGET target);
  void Override_Mission(MissionType mission, TARGET tarcom,
                        TARGET navcom) override;
  bool Restore_Mission() override;
  virtual BulletClass* Fire_At(TARGET target, int which = 0);
  int Weapon_Range(int which) const override;
  virtual bool Captured(HouseClass* newowner);
  ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                         TechnoClass* source) override;
  bool Evaluate_Cell(ThreatType method, int mask, CELL cell, int range,
                     const TechnoClass** object, int& value) const;
  bool Evaluate_Object(ThreatType method, int mask, int range,
                       const TechnoClass* object, int& value) const;

  /*
  **	AI.
  */
  void AI() override;
  bool Revealed(HouseClass* house) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   long& param) override;

  /*
   **	Scenario and debug support.
   */
  void Debug_Dump(MonoClass* mono) const override;

  /*
  **	File I/O.
  */
  void Code_Pointers() override;
  void Decode_Pointers() override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  virtual const void* Remap_Table();
  VisualType Visual_Character(bool raw = false);
  void Techno_Draw_Object(const void* shapefile, int shapenum, int x, int y,
                          WindowNumberType window);
  void Draw_It(int x, int y, WindowNumberType window) override;
  virtual void Draw_Pips(int x, int y, WindowNumberType window);
  void Hidden() override;
  bool Mark(MarkType mark) override;
  int Exit_Object(TechnoClass*) override;
  virtual void Do_Uncloak();
  virtual void Do_Cloak();
  void Do_Shimmer() override;

  /*
  **	Movement and animation.
  */
  virtual void Random_Animate();
  virtual void Assign_Destination(TARGET target);
  void Scatter(COORDINATE source = 0, bool forced = false) override;
  virtual void Per_Cell_Process(bool);
  virtual void Enter_Idle_Mode(bool initial = false);

  /*
  **	Map entry and exit logic.
  */
  bool Unlimbo(COORDINATE, DirType facing = DIR_N) override;
  void Detach(TARGET target, bool all) override;

  /*
  **	Facing translation tables that fix the flaw with 3D studio when
  **	it renders 45 degree angles.
  */
  static const int BodyShape[32];
  //		static int const TurretShape[32];
};

#endif
