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

/* $Header: /CounterStrike/UNIT.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : UNIT.H *
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

#ifndef CNC_RED_ALERT_RA_OBJECTS_UNIT_H_
#define CNC_RED_ALERT_RA_OBJECTS_UNIT_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "base/fixed.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "game/sim/direction.h"
#include "game/sim/heading.h"
#include "ra/core/defines.h"
#include "ra/objects/bullet.h"
#include "ra/objects/drive.h"
#include "ra/objects/object.h"
#include "ra/objects/radio.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"

class ArchiveReader;
class ArchiveWriter;

namespace ra {

/****************************************************************************
**	For each instance of a unit (vehicle) in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular unit.
*/
class UnitClass final : public DriveClass {
 public:
  /*
  **	This points to the static control data that gives 'this' unit its
  *characteristics.
  */
  const UnitTypeClass* Class = nullptr;

  /*
  **	This records the house flag that this object is currently carrying.
  */
  HousesType Flagged = HOUSE_NONE;

  /*
  ** This flag is used for when the harvester dumps ore, to track its
  ** special animation.
  */
  bool IsDumping : 1 = false;

  /*
  ** This is a count of the # of loads of the various minerals that the
  ** unit has harvested.
  */
  uint8_t Gold : 5 = 0;
  uint8_t Gems : 5 = 0;

  /*
  ** This flag tells a unit that, if after reaching its destination, it
  ** should scatter away.  It's meant to help a LST unload its units by
  ** having its previous passengers get out of the way.
  */
  bool IsToScatter : 1 = false;

  /*
  **	This records the number of "loads" of Tiberium the unit is carrying.
  *Only *	harvesters use this field.
  */
  int Tiberium = 0;

  /*
  ** This is the area where a mobile gap generator stores the previously-held
  ** shroud values for the cells surrounding itself.
  */
  uint64_t ShroudBits = 0xFFFFFFFFUL;

  /*
  ** This is the center coordinate for the mobile gap generator, as to
  ** what cells should be revealed (according to ShroudBits)
  */
  Cell ShroudCenter{};

  /*
  **	This is the timer that controls the reload rate. The MSAM rocket
  **	launcher is the primary user of this.
  */
  engine::platform::Timer<game::FrameTickSource> Reload{0};

  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  game::Heading SecondaryFacing;

 protected:
  // Shell for ObjectHeap::Load; Serialize() supplies every value.
  UnitClass() = default;
  friend class game::ObjectHeap<UnitClass>;

 public:
  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  void* operator new(std::size_t size) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  UnitClass(UnitType classid, HousesType house);
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator UnitType() const { return Class->Type; }
  ~UnitClass() override;
  UnitClass(const UnitClass&) = delete;
  UnitClass& operator=(const UnitClass&) = delete;
  UnitClass(UnitClass&&) = delete;
  UnitClass& operator=(UnitClass&&) = delete;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override;
  static void Init();

  bool Goto_Clear_Spot();
  bool Try_To_Deploy();
  void Scatter(Coordinate threat, bool forced = false,
               bool nokidding = false) override;

  bool Tiberium_Check(Cell& center, int x, int y);
  bool Flag_Attach(HousesType house);
  bool Flag_Remove();
  bool Goto_Tiberium(int radius);
  bool Harvesting();
  void APC_Close_Door();
  void APC_Open_Door();

  /*
  **	Query functions.
  */
  bool Should_Crush_It(const TechnoClass* it) const;
  [[nodiscard]] int Credit_Load() const;
  [[nodiscard]] game::Direction Turret_Facing() const override {
    if (Class->IsTurretEquipped) {
      return SecondaryFacing.current();
    }
    return PrimaryFacing.current();
  }
  [[nodiscard]] int Shape_Number() const;
  [[nodiscard]] int Pip_Count() const override;
  [[nodiscard]] InfantryType Crew_Type() const override;
  [[nodiscard]] game::Direction Fire_Direction() const override;
  bool Ok_To_Move(game::Direction dir) override;
  [[nodiscard]] game::FireErrorType Can_Fire(Target target,
                                             int which) const override;
  [[nodiscard]] Fixed Tiberium_Load() const override;

  /*
  **	Coordinate inquiry functions. These are used for both display and
  **	combat purposes.
  */
  [[nodiscard]] Coordinate Sort_Y() const override;

  /*
  **	Object entry and exit from the game system.
  */
  bool Limbo() override;
  bool Unlimbo(Coordinate /*coord*/ /*unused*/,
               game::Direction dir = game::Direction::kNorth) override;

  /*
  **	Display and rendering support functionality. Supports imagery and how
  **	object interacts with the map and thus indirectly controls rendering.
  */
  [[nodiscard]] std::span<const int16_t> Overlap_List(
      bool redraw = false) const override;
  void Draw_It(PixelView& view, int x, int y,
               WindowNumberType window) const override;

  /*
  **	User I/O.
  */
  [[nodiscard]] ActionType What_Action(Cell cell) const override;
  ActionType What_Action(ObjectClass* object) override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  void Active_Click_With(ActionType action, Cell cell) override;

  /*
  **	Combat related.
  */
  game::ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                               TechnoClass* source = nullptr,
                               bool forced = false) override;
  BulletClass* Fire_At(Target target, int which = 0) override;

  /*
  **	Driver control support functions. These are used to control cell
  **	occupation flags and driver instructions.
  */
  bool Start_Driver(Coordinate& headto) override;

  /*
  **	AI.
  */
  Target Greatest_Threat(ThreatType threat) override;  // const;
  game::Direction Desired_Load_Dir(ObjectClass* passenger,
                                   Cell& moveto) const override;
  RadioMessageType Receive_Message(TechnoClass* from, RadioMessageType message,
                                   int32_t& param) override;
  void AI() override;
  int Mission_Guard_Area() override;
  int Mission_Unload() override;
  int Mission_Guard() override;
  int Mission_Harvest() override;
  int Mission_Hunt() override;
  int Mission_Repair() override;
  int Mission_Move() override;
  void Rotation_AI();
  void Firing_AI();
  void Reload_AI();
  bool Edge_Of_World_AI();

  /*
  **	Scenario and debug support.
  */

  /*
  **	Movement and animation.
  */
  void Assign_Destination(Target target) override;
  void Overrun_Square(Cell cell, bool threaten = true) override;
  void Approach_Target() override;
  void Enter_Idle_Mode(bool initial = false) override;
  [[nodiscard]] game::MoveType Can_Enter_Cell(
      Cell cell, game::Facing facing = game::Facing::kNone) const override;
  [[nodiscard]] bool Per_Cell_Process(PCPType why) override;
  void Exit_Repair();
  void Shroud_Regen();

  /*
  **	File I/O.
  */
  static void Read_INI(engine::file::IniFile& ini);
  static void Write_INI(engine::file::IniFile& ini);
  static const char* INI_Name() { return "UNITS"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);
};

extern template void UnitClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void UnitClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_UNIT_H_
