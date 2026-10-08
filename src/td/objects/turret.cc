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

/* $Header:   F:\projects\c&c\vcs\code\turret.cpv   2.18   16 Oct 1995 16:50:38
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TURRET.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 25, 1994 *
 *                                                                                             *
 *                  Last Update : August 13, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   TurretClass::AI -- Handles the reloading of the turret weapon.
 *   TurretClass::Can_Fire -- Determines if turret can fire upon target.
 *   TurretClass::Fire_At -- Try to fire upon the target specified.
 *   TurretClass::Fire_Coord -- Determines the coorindate that projectile
 *     would appear.
 *   TurretClass::Fire_Direction -- Determines the directinon of firing.
 *   TurretClass::Ok_To_Move -- Queries whether the vehicle can move.
 *   TurretClass::TurretClass -- Normal constructor for the turret class.
 *   TurretClass::TurretClass -- The default constructor for turret class
 *     objects.
 *   TurretClass::Unlimbo -- Unlimboes turret object.
 *   TurretClass::~TurretClass -- Default destructor for turret class
 *     objects.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/turret.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>

#include "base/array.h"
#include "base/cardinal.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "td/core/const.h"
#include "td/core/coord.h"
#include "td/core/defines.h"
#include "td/objects/bullet.h"
#include "td/objects/drive.h"
#include "td/objects/type.h"

namespace td {

/***********************************************************************************************
 * TurretClass::TurretClass -- Normal constructor for the turret class. *
 *                                                                                             *
 *    This is the normal constructor for the turret class. It merely sets the
 *turret up to     * face north. *
 *                                                                                             *
 * INPUT:   classid  -- The type id for this particular unit. *
 *                                                                                             *
 *          house    -- The house that this unit will belong to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/02/1995 JLB : Created. *
 *=============================================================================================*/
TurretClass::TurretClass(UnitType classid, HousesType house)
    : DriveClass(classid, house) {
  Reload.Set(0);
}

/***********************************************************************************************
 * TurretClass::Ok_To_Move -- Queries whether the vehicle can move. *
 *                                                                                             *
 *    This virtual routine is used to determine if the vehicle is allowed * to
 *start moving. It is typically called when the vehicle desires * to move but
 *needs confirmation from the turret logic before                              *
 *    proceeding. This happens when dealing with a vehicle that must have * its
 *turret face the same direction as the body before the vehicle * may begin
 *movement. *
 *                                                                                             *
 * INPUT:   dir      -- The facing the unit wants to travel in. *
 *                                                                                             *
 * OUTPUT:  bool; Can the unit begin forward movement now? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/12/1994 JLB : Created. *
 *=============================================================================================*/
bool TurretClass::Ok_To_Move(game::Direction dir) {
  if (Class->IsLockTurret) {
    if (IsRotating) {
      return false;
    }
    if (SecondaryFacing.current() != dir) {
      SecondaryFacing.set_desired(dir);
      return false;
    }
  }
  return true;
}

/***********************************************************************************************
 * TurretClass::AI -- Handles the reloading of the turret weapon. *
 *                                                                                             *
 *    This processes the reloading of the turret. It does this by decrementing
 *the arming      * countdown  timer and when it reaches zero, the turret may
 *fire.                          *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/21/1994 JLB : Created. *
 *=============================================================================================*/
void TurretClass::AI() {
  DriveClass::AI();

  /*
  **	A unit with a constant rotating radar dish is handled here.
  */
  if (Class->IsRadarEquipped) {
    SecondaryFacing.SnapTo(SecondaryFacing.current() + 8);
    Mark(MARK_CHANGE);
  } else {
    IsRotating = false;
    if (Class->IsTurretEquipped) {
      if (IsTurretLockedDown) {
        SecondaryFacing.set_desired(PrimaryFacing.current());
      }

      if (SecondaryFacing.IsRotating()) {
        if (SecondaryFacing.Turn(Class->ROT + 1)) {
          Mark(MARK_CHANGE);
        }

        /*
        **	If no further rotation is necessary, flag that the rotation
        **	has stopped.
        */
        IsRotating = SecondaryFacing.IsRotating();
      } else {
        if (!IsTurretLockedDown && !TarCom) {
          if (!NavCom) {
            SecondaryFacing.set_desired(PrimaryFacing.current());
          } else {
            SecondaryFacing.set_desired(DirectionTo(NavCom));
          }
        }
      }
    }
  }
}

/***********************************************************************************************
 * TurretClass::Fire_At -- Try to fire upon the target specified. *
 *                                                                                             *
 *    This routine is the auto-fire logic for the turret. It will check * to see
 *if firing is technically legal given the specified target. * If it is legal to
 *fire, it does so. It is safe to call this routine                      * every
 *game tick. *
 *                                                                                             *
 * INPUT:   target   -- The target to fire upon. *
 *                                                                                             *
 *          which    -- Which weapon to use when firing. 0=primary, 1=secondary.
 **
 *                                                                                             *
 * OUTPUT:  bool; Did firing occur? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/26/1994 JLB : Created. *
 *=============================================================================================*/
BulletClass* TurretClass::Fire_At(Target target, int which) {
  BulletClass* bullet = nullptr;

  if (Can_Fire(target, which) == game::FIRE_OK) {
    bullet = DriveClass::Fire_At(target, which);

    if (bullet) {
      /*
      **	Possible reload timer set.
      */
      if (*this == UNIT_MSAM && Reload.IsFinished()) {
        Reload.Set(int64_t{kTicksPerSecond} * 30);
      }
    }
  }

  return bullet;
}

/***********************************************************************************************
 * TurretClass::Can_Fire -- Determines if turret can fire upon target. *
 *                                                                                             *
 *    This routine determines if the turret can fire upon the target *
 *    specified. *
 *                                                                                             *
 * INPUT:   target   -- The target to fire upon. *
 *                                                                                             *
 *          which    -- Which weapon to use to determine legality to fire.
 *0=primary,          * 1=secondary. *
 *                                                                                             *
 * OUTPUT:  Returns the fire status type that indicates if firing is allowed and
 *if not, why.  *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 04/26/1994 JLB : Created. * 06/01/1994 JLB : Returns reason why it
 *can't fire.                                        *
 *=============================================================================================*/
game::FireErrorType TurretClass::Can_Fire(Target target, int which) const {
  int diff = 0;
  const game::FireErrorType fire = DriveClass::Can_Fire(target, which);

  if (fire == game::FIRE_OK) {
    const WeaponTypeClass* weapon = which == 0 ? &Weapons.at(Class->Primary)
                                               : &Weapons.at(Class->Secondary);

    /*
    **	If this unit cannot fire while moving, then bail.
    */
    if ((!Class->IsTurretEquipped || Class->IsLockTurret) && NavCom) {
      return game::FIRE_MOVING;
    }

    /*
    **	If the turret is rotating and the projectile isn't a homing type, then
    **	firing must be delayed until the rotation stops.
    */
    if (!IsFiring && IsRotating &&
        !BulletTypeClass::As_Reference(weapon->Fires).IsHoming) {
      return game::FIRE_ROTATING;
    }

    const game::Direction dir =
        DirectionTo(target);  // The facing to impart upon the projectile.

    /*
    **	Determine if the turret facing isn't too far off of facing the target.
    */
    if (Class->IsTurretEquipped) {
      diff = game::ShortestTurn(SecondaryFacing, dir);
    } else {
      diff = game::ShortestTurn(PrimaryFacing, dir);
    }
    diff = std::abs(diff);

    /*
    **	Special flame tank logic.
    */
    if (weapon->Fires == BULLET_FLAME) {
      if (game::NearestFacing(dir) == game::NearestFacing(PrimaryFacing)) {
        diff = 0;
      }
    }

    if (BulletTypeClass::As_Reference(weapon->Fires).IsHoming) {
      diff /= 4;
    }
    if (diff < 8) {
      return DriveClass::Can_Fire(target, which);
    }
    return game::FIRE_FACING;
  }
  return fire;
}

/***********************************************************************************************
 * TurretClass::Fire_Coord -- Determines the coorindate that projectile would
 *appear.          *
 *                                                                                             *
 *    Use this routine to determine the exact coordinate that a projectile would
 *appear if it  * were fired from this unit. For units with turrets, typically,
 *this would be at the end   * of the barrel. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with coordinate of where a projectile should appear if this
 *unit were      * to fire one. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/28/1994 JLB : Created. *
 *=============================================================================================*/
Coordinate TurretClass::Fire_Coord(int which) const {
  Coordinate coord = Center_Coord();
  int dist = 0;
  int lateral = 0;
  game::Direction dir = PrimaryFacing.current();

  if (Class->IsTurretEquipped) {
    dir = SecondaryFacing.current();
  }

  switch (Class->Type) {
    case UNIT_GUNBOAT:
      coord = Coord_Move(coord, PrimaryFacing.current(),
                         base::At(Pixel2Lepton, Class->TurretOffset));
      dist = 0x0060;
      break;

    case UNIT_ARTY:
      coord = Coord_Move(coord, game::Direction::kNorth, 0x0040);
      dist = 0x0060;
      break;

    case UNIT_FTANK:
      dist = 0x30;
      if (IsSecondShot) {
        coord = Coord_Move(coord, dir + game::Direction::kEast, 0x20);
      } else {
        coord = Coord_Move(coord, dir + game::Direction::kWest, 0x20);
      }
      break;

    case UNIT_HTANK:
      coord = Coord_Move(coord, game::Direction::kNorth, 0x0040);
      if (which == 0) {
        dist = 0x00C0;
        lateral = 0x0028;
      } else {
        dist = 0x0008;
        lateral = 0x0040;
      }
      if (IsSecondShot) {
        coord = Coord_Move(coord, dir + game::Direction::kEast,
                           static_cast<uint16_t>(lateral));
      } else {
        coord = Coord_Move(coord, dir + game::Direction::kWest,
                           static_cast<uint16_t>(lateral));
      }
      break;

    case UNIT_LTANK:
      coord = Coord_Move(coord, game::Direction::kNorth, 0x0020);
      dist = 0x00C0;
      break;

    case UNIT_MTANK:
      coord = Coord_Move(coord, game::Direction::kNorth, 0x0030);
      dist = 0x00C0;
      break;

    case UNIT_APC:
    case UNIT_JEEP:
    case UNIT_BUGGY:
      coord = Coord_Move(coord, game::Direction::kNorth, 0x0030);
      dist = 0x0030;
      break;
    case UnitType::UNIT_NONE:
    case UnitType::UNIT_STANK:
    case UnitType::UNIT_VICE:
    case UnitType::UNIT_MLRS:
    case UnitType::UNIT_HARVESTER:
    case UnitType::UNIT_MSAM:
    case UnitType::UNIT_HOVER:
    case UnitType::UNIT_MHQ:
    case UnitType::UNIT_MCV:
    case UnitType::UNIT_BIKE:
    case UnitType::UNIT_TRIC:
    case UnitType::UNIT_TREX:
    case UnitType::UNIT_RAPT:
    case UnitType::UNIT_STEG:
    case UnitType::UNIT_COUNT:
    default:
      break;
  }

  if (dist) {
    coord = Coord_Move(coord, dir, static_cast<uint16_t>(dist));
  }

  return coord;
}

/***********************************************************************************************
 * TurretClass::Unlimbo -- Unlimboes turret object. *
 *                                                                                             *
 *    This routine is called when a turret equipped unit unlimboes. It sets the
 *turret to      * face the same direction as the body. *
 *                                                                                             *
 * INPUT:   coord -- The coordinate where the unit is unlimboing. *
 *                                                                                             *
 *          dir   -- The desired body and turret facing to use. *
 *                                                                                             *
 * OUTPUT:  Was the unit unlimboed successfully? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
bool TurretClass::Unlimbo(Coordinate coord, game::Direction dir) {
  if (DriveClass::Unlimbo(coord, dir)) {
    SecondaryFacing.SnapTo(dir);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * TurretClass::Fire_Direction -- Determines the directinon of firing. *
 *                                                                                             *
 *    This routine will return with the facing that a projectile will travel if
 *it was         * fired at this instant. The facing should match the turret
 *facing for those units         * equipped with a turret. If the unit doesn't
 *have a turret, then it will be the facing    * of the body. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the default firing direction for a projectile. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Created. *
 *=============================================================================================*/
game::Direction TurretClass::Fire_Direction() const {
  if (Class->IsTurretEquipped) {
    if (*this == UNIT_MSAM) {
      int diff1 = game::ShortestTurn(SecondaryFacing, game::Direction::kEast);
      int diff2 = game::ShortestTurn(SecondaryFacing, game::Direction::kWest);
      diff1 = std::abs(diff1);
      diff2 = std::abs(diff2);
      const int diff = std::min(diff1, diff2);
      const int to_north =
          game::ShortestTurn(SecondaryFacing, game::Direction::kNorth);
      const int adj = base::FixedToCardinal(std::abs(to_north), 64 - diff);
      if (to_north < 0) {
        return SecondaryFacing.current() - adj;
      }
      return SecondaryFacing + game::Direction(adj);
    }
    return SecondaryFacing.current();
  }

  return PrimaryFacing.current();
}

}  // namespace td
