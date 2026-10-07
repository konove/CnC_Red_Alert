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

/* $Header: /CounterStrike/VESSEL.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : VESSEL.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 03/13/96 *
 *                                                                                             *
 *                  Last Update : March 13, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_OBJECTS_VESSEL_H_
#define CNC_RED_ALERT_RA_OBJECTS_VESSEL_H_

#include <cstddef>
#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "engine/file/ini_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "game/sim/direction.h"
#include "game/sim/facing.h"
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

class VesselClass : public DriveClass {
 public:
  /*
  **	This points to the controling static characteristic data associated with
  **	this vessel.
  */
  const VesselTypeClass* Class = nullptr;

  /*
  ** Has this sea vessel been told to move to a shipyard?  If so, then
  ** when we get there, start the repair process.
  */
  bool IsToSelfRepair : 1 = false;

  /*
  ** Is this sea vessel parked next to a shipyard/subpen, and therefore
  ** in the special self-repair mode?
  */
  bool IsSelfRepairing : 1 = false;

  /*
  ** If this is an LST, is it time to shut the door?
  */
  engine::platform::Timer<game::FrameTickSource> DoorShutCountDown{0};

  /*
  ** If this is a sub, has the sonar pulse worn off, such that we can
  ** re-submerge?
  */
  engine::platform::Timer<game::FrameTickSource> PulseCountDown{0};

  VesselClass(VesselType classid, HousesType house);
  void* operator new(std::size_t size) noexcept;
  void* operator new(std::size_t /*unused*/,
                     void* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND) noexcept {
    return ptr;
  }
  void operator delete(void* ptr);
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator VesselType() const { return Class->Type; }

  static void Init();

  ~VesselClass() override;
  VesselClass(const VesselClass&) = delete;
  VesselClass& operator=(const VesselClass&) = delete;
  VesselClass(VesselClass&&) = delete;
  VesselClass& operator=(VesselClass&&) = delete;
  [[nodiscard]] const ObjectTypeClass& Class_Of() const override;

  [[nodiscard]] virtual MZoneType Zone_Check_Type() const {
    return MZONE_WATER;
  }
  [[nodiscard]] int Shape_Number() const;
  void Rotation_AI();
  void Combat_AI();
  bool Edge_Of_World_AI();
  void Repair_AI();
  [[nodiscard]] game::Direction Turret_Facing() const override {
    if (Class->IsTurretEquipped) {
      return SecondaryFacing.Current();
    }
    return PrimaryFacing.Current();
  }
  bool Start_Driver(Coordinate& headto) override;
  int Mission_Retreat() override;
  int Mission_Unload() override;
  void LST_Open_Door();
  void LST_Close_Door();
  [[nodiscard]] Coordinate Fire_Coord(int which) const override;
  [[nodiscard]] game::MoveType Can_Enter_Cell(
      Cell cell,
      game::FacingType from = game::FacingType::kNone) const override;
  void Draw_It(PixelView& view, int x, int y,
               WindowNumberType window) const override;
  [[nodiscard]] std::span<const int16_t> Overlap_List(
      bool redraw = false) const override;
  game::Direction Desired_Load_Dir(ObjectClass* passenger,
                                   Cell& moveto) const override;
  RadioMessageType Receive_Message(RadioClass* from, RadioMessageType message,
                                   int32_t& param) override;
  void AI() override;
  [[nodiscard]] bool Per_Cell_Process(PCPType why) override;
  void Assign_Destination(Target target) override;

  game::ResultType Take_Damage(int& damage, int distance, WarheadType warhead,
                               TechnoClass* source = nullptr,
                               bool forced = false) override;
  [[nodiscard]] game::FireErrorType Can_Fire(Target target,
                                             int which) const override;

  void Enter_Idle_Mode(bool initial = false) override;
  ActionType What_Action(ObjectClass* object) override;
  [[nodiscard]] ActionType What_Action(Cell cell) const override;
  void Active_Click_With(ActionType action, Cell cell) override;
  void Active_Click_With(ActionType action, ObjectClass* object) override;
  Target Greatest_Threat(ThreatType threat) override;  // const;
  [[nodiscard]] bool Is_Allowed_To_Recloak() const override;
  BulletClass* Fire_At(Target target, int which = 0) override;
  /*
  **	File I/O.
  */
  static void Read_INI(engine::file::IniFile& ini);
  static void Write_INI(engine::file::IniFile& ini);
  static const char* INI_Name() { return "SHIPS"; }
  // Saved-game support; defined in ioobj.cc.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	Scenario and debug support.
  */

 protected:
  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  game::FacingClass SecondaryFacing;

  // Shell for ObjectHeap::Load; Serialize() supplies every value.
  VesselClass() = default;
  friend class game::ObjectHeap<VesselClass>;
};

extern template void VesselClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void VesselClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_OBJECTS_VESSEL_H_
