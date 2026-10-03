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

/* $Header: /CounterStrike/AIRCRAFT.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : AIRCRAFT.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : July 22, 1994 *
 *                                                                                             *
 *                  Last Update : November 28, 1994 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_OBJECTS_AIRCRAFT_H_
#define CNC_RED_ALERT_RA_OBJECTS_AIRCRAFT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "game/sim/direction.h"
#include "game/sim/facing.h"
#include "ra/core/defines.h"
#include "ra/data/ccini.h"
#include "ra/objects/bullet.h"
#include "ra/objects/fly.h"
#include "ra/objects/foot.h"
#include "ra/objects/object.h"
#include "ra/objects/radio.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

/*
**	This aircraft class is used for all flying sentient objects. This
*includes fixed wing *	aircraft as well as helicopters. It excludes bullets
*even though some bullets might *	be considered to be "flying" in a loose
*interpretatin of the word.
*/
class AircraftClass : public FootClass, public FlyClass {
 public:
  /*
  **	This is a pointer to the class control structure for the aircraft.
  */
  const AircraftTypeClass* Class = nullptr;

  //-----------------------------------------------------------------------------
  void* operator new(std::size_t /*unused*/) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* /*ptr*/);
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator AircraftType() const { return Class->Type; }
  AircraftClass(AircraftType classid, HousesType house);
  ~AircraftClass() override;
  AircraftClass(const AircraftClass&) = delete;
  AircraftClass& operator=(const AircraftClass&) = delete;
  AircraftClass(AircraftClass&&) = delete;
  AircraftClass& operator=(AircraftClass&&) = delete;

  static void Init();

  int Mission_Attack() override;
  int Mission_Unload() override;
  int Mission_Hunt() override;
  int Mission_Retreat() override;
  int Mission_Move() override;
  int Mission_Enter() override;
  int Mission_Guard() override;
  int Mission_Guard_Area() override;

  void Assign_Destination(Target dest) override;
  /*
  **	State machine support routines.
  */
  bool Process_Take_Off();
  bool Process_Landing();
  int Process_Fly_To(bool slowdown, Target dest);

  /*
  **	Query functions.
  */
  [[nodiscard]] LayerType In_Which_Layer() const override;
  [[nodiscard]] game::DirType Turret_Facing() const override {
    return SecondaryFacing.Current();
  }
  [[nodiscard]] int Shape_Number() const;
  [[nodiscard]] game::MoveType Can_Enter_Cell(
      Cell cell, game::FacingType facing = game::FACING_NONE) const override;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override {
    return *Class;
  }
  ActionType What_Action(ObjectClass* target) override;
  [[nodiscard]] ActionType What_Action(Cell cell) const override;
  game::DirType Desired_Load_Dir(ObjectClass* object,
                                 Cell& moveto) const override;
  [[nodiscard]] int Pip_Count() const override;
  [[nodiscard]] Target Good_Fire_Location(Target target) const;
  [[nodiscard]] bool Cell_Seems_Ok(Cell cell, bool strict = false) const;
  [[nodiscard]] game::DirType Pose_Dir() const;
  [[nodiscard]] Target Good_LZ() const;
  [[nodiscard]] game::DirType Fire_Direction() const override;
  [[nodiscard]] game::FireErrorType Can_Fire(Target target,
                                             int which) const override;

  /*
  **	Landing zone support functionality.
  */
  [[nodiscard]] bool Per_Cell_Process(PCPType why) override;
  [[nodiscard]] bool Is_LZ_Clear(Target target) const;
  [[nodiscard]] Target New_LZ(Target oldlz) const;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  [[nodiscard]] Coordinate Sort_Y() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Unlimbo(Coordinate /*coord*/ /*unused*/,
               game::DirType dir = game::DIR_N) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  void Look(bool incremental = false) override;
  void Draw_Rotors(PixelView& view, int x, int y,
                   WindowNumberType window) const;
  int Exit_Object(TechnoClass* /*unit*/ /*unused*/) override;
  [[nodiscard]] std::span<const int16_t> Overlap_List(
      bool redraw = false) const override;
  void Draw_It(PixelView& view, int x, int y,
               WindowNumberType window) const override;
  void Set_Speed(int speed) override;

  /*
  **	User I/O.
  */
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, Cell cell) override;
  void Player_Assign_Mission(MissionType mission, Target target = Target{},
                             Target destination = Target{}) override;
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;

  /*
  **	Combat related.
  */
  game::ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                               TechnoClass* source,
                               bool forced = false) override;
  BulletClass* Fire_At(Target target, int which) override;

  /*
  **	AI.
  */
  bool Landing_Takeoff_AI();
  bool Edge_Of_World_AI();
  void Movement_AI();
  void Rotation_AI();
  int Paradrop_Cargo();
  void AI() override;
  void Enter_Idle_Mode(bool initial = false) override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   int32_t& param) override;
  void Scatter(Coordinate threat, bool forced = false,
               bool nokidding = false) override;

  /*
  **	File I/O.
  */
  static void Read_INI(CCINIClass& ini);
  static const char* INI_Name() { return "AIRCRAFT"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	This is the facing used for the body of the aircraft. Typically, this is
  *the same *	as the PrimaryFacing, but in the case of helicopters, it can be
  *different.
  */
  game::FacingClass SecondaryFacing;

  /*
  **	If this is a passenger carrying aircraft then this flag will be set.
  *This is *	necessary because once the passengers are unloaded, the fact
  *that it was a *	passenger carrier must still be known.
  */
  bool Passenger = false;

 private:
  /*
  **	Aircraft can be in either state of landing, taking off, or in steady
  *altitude. *	These flags are used to control transition between flying and
  *landing. It is *	necessary to handle the transition in this manner so
  *that it occurs smoothly *	during the graphic processing section.
  */
  bool IsLanding : 1 = false;
  bool IsTakingOff : 1 = false;

  /*
  **	It is very common for aircraft to be homing in on a target. When this
  *flag is *	true, the aircraft will constantly adjust its facing toward the
  *TarCom. When the *	target is very close (one cell away or less), then this
  *flag is automatically cleared. *	This is because the homing algorithm is
  *designed to get the aircraft to the destination *	but no more. Checking
  *when this flag is cleared is a way of flagging transition into *	a new
  *mode. Example: Transport helicopters go into a hovering into correct position
  **	mode when the target is reached.
  */
  bool IsHoming : 1 = false;

  /*
  **	Helicopters that are about to land must hover into a position exactly
  *above the landing *	zone. When this flag is true, the aircraft will be
  *adjusted so that it is exactly over *	the TarCom. The facing of the
  *aircraft is not altered by this movement. The affect *	like the
  *helicopter is hovering and shifting sideways to position over the landing
  **	zone. When the position is over the landing zone, then this flag is set
  *to false.
  */
  bool IsHovering : 1 = false;

  /*
  **	This is the jitter tracker to be used when the aircraft is a helicopter
  *and *	is flying. It is most noticeable when the helicopter is
  *hovering.
  */
  unsigned char Jitter = 0;

  /*
  **	This timer controls when the aircraft will reveal the terrain around
  *itself. *	When this timer expires and this aircraft has a sight range,
  *then the *	look around process will occur.
  */
  engine::platform::Timer<game::FrameTickSource> SightTimer{0};

  /*
  **	Most attack aircraft can make several attack runs. This value contains
  *the *	number of attack runs the aircraft has left. When this value
  *reaches *	zero then the aircraft is technically out of ammo.
  */
  char AttacksRemaining = 1;

  // Shell for ObjectHeap::Load; Serialize() supplies every value.
  AircraftClass() = default;
  friend class game::ObjectHeap<AircraftClass>;
};

bool Building_Check();

extern template void AircraftClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void AircraftClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_AIRCRAFT_H_
