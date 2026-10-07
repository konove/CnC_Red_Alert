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

/* $Header:   F:\projects\c&c\vcs\code\turret.h_v   2.17   16 Oct 1995 16:48:00
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TURRET.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 25, 1994 *
 *                                                                                             *
 *                  Last Update : April 25, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_OBJECTS_TURRET_H_
#define CNC_RED_ALERT_TD_OBJECTS_TURRET_H_

#include "engine/platform/timer.h"
#include "game/core/game_clock.h"
#include "game/sim/facing.h"
#include "td/core/defines.h"
#include "td/objects/bullet.h"
#include "td/objects/drive.h"

class ArchiveReader;
class ArchiveWriter;

namespace td {

class TurretClass : public DriveClass {
 public:
  // Field-wise state and checked references; load shells have no scenario effects.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	This is the timer that controls the reload rate. The MSAM rocket
  **	launcher is the primary user of this.
  */
  engine::platform::Timer<game::FrameTickSource> Reload;

  /*
  **	This is the facing of the turret. It can be, and usually is,
  **	rotated independently of the body it is attached to.
  */
  game::FacingClass SecondaryFacing;

  bool Unlimbo(Coordinate /*coord*/ /*unused*/,
               game::DirType dir = game::DirType::kNorth) override;

  /*
  **	File I/O.
  */

  ~TurretClass() override = default;
  TurretClass(const TurretClass&) = delete;
  TurretClass& operator=(const TurretClass&) = delete;
  TurretClass(TurretClass&&) = delete;
  TurretClass& operator=(TurretClass&&) = delete;

  BulletClass* Fire_At(Target target, int which) override;
  game::DirType Fire_Direction() const override;
  game::FireErrorType Can_Fire(Target target, int which) const override;
  bool Ok_To_Move(game::DirType dir) override;
  void AI() override;
  Coordinate Fire_Coord(int which) const override;

 protected:
  TurretClass(UnitType classid, HousesType house);
  TurretClass() = default;
};

extern template void TurretClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void TurretClass::Serialize<ArchiveReader>(ArchiveReader&);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_OBJECTS_TURRET_H_
