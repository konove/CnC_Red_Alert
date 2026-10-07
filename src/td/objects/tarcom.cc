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

/* $Header:   F:\projects\c&c\vcs\code\tarcom.cpv   2.17   16 Oct 1995 16:52:20
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : TARCOM.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 16, 1994 *
 *                                                                                             *
 *                  Last Update : July 19, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   TarComClass::AI -- Handles the logical AI for the tarcom class.
 *   TarComClass::~TarComClass -- Destructor for turret object.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

/***********************************************************************************************
 * TarComClass::~TarComClass -- Destructor for turret object. *
 *                                                                                             *
 *    This is the destructor for turret objects. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1995 JLB : Created. *
 *=============================================================================================*/
#include "td/objects/tarcom.h"

#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "game/sim/facing.h"
#include "td/assets/audio.h"
#include "td/core/const.h"
#include "td/core/defines.h"
#include "td/objects/turret.h"
#include "td/objects/type.h"

namespace td {

TarComClass::~TarComClass() = default;

/***********************************************************************************************
 * TarComClass::AI -- Handles the logical AI for the tarcom class. *
 *                                                                                             *
 *    This handles the AI logic for the targeting computer. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/02/1994 JLB : Created. *
 *=============================================================================================*/
void TarComClass::AI() {
  TurretClass::AI();

  if (Class->Primary != WEAPON_NONE) {
    /*
    **	Determine which weapon can fire. First check for the primary weapon. If
    *that weapon *	cannot fire, then check any secondary weapon. If neither
    *weapon can fire, then the *	failure code returned is that from the
    *primary weapon.
    */
    const WeaponTypeClass* weapon = &Weapons.at(Class->Primary);
    int primary = 0;
    game::FireErrorType ok = Can_Fire(TarCom, 0);
    if ((ok != game::FIRE_OK) && (Can_Fire(TarCom, 1) == game::FIRE_OK)) {
      ok = game::FIRE_OK;
      primary = 1;
      weapon = &Weapons.at(Class->Secondary);
    }

    switch (ok) {
      case game::FIRE_OK:
        if (What_Am_I() != RTTI_UNIT) {
          IsFiring = false;
        } else {
          if (!this->Class->IsFireAnim) {
            IsFiring = false;
          }
        }

        if (TurretClass::Fire_At(TarCom, primary)) {
          Sound_Effect(weapon->Sound, Coord);
        }
        break;

      case game::FIRE_FACING:
        if (Class->IsLockTurret) {
          if (!NavCom && !IsDriving) {
            PrimaryFacing.Set_Desired(Direction(TarCom));
            SecondaryFacing.Set_Desired(PrimaryFacing.Desired());
          }
        } else {
          if (*this == UNIT_FTANK) {
            SecondaryFacing.Set_Desired(
                game::DirectionOf(game::NearestFacing(Direction(TarCom))));
          } else {
            SecondaryFacing.Set_Desired(Direction(TarCom));
          }
          //					SecondaryFacing.Set_Desired(Direction256(Center_Coord(),
          // TarCom.AsCoordinate()));
        }
        break;

      case game::FIRE_CLOAKED:
        IsFiring = false;
        Do_Uncloak();
        break;
      case game::FireErrorType::FIRE_AMMO:
      case game::FireErrorType::FIRE_REARM:
      case game::FireErrorType::FIRE_ROTATING:
      case game::FireErrorType::FIRE_ILLEGAL:
      case game::FireErrorType::FIRE_CANT:
      case game::FireErrorType::FIRE_MOVING:
      case game::FireErrorType::FIRE_RANGE:
      case game::FireErrorType::FIRE_BUSY:
      default:
        break;
    }
  }

  if (TarCom && !IsRotating) {
    const game::DirType dir = Direction(TarCom);

    if (Class->IsTurretEquipped) {
      SecondaryFacing.Set_Desired(dir);
    } else {
      /*
      **	Non turret equipped vehicles will rotate their body to face the
      *target only *	if the vehicle isn't currently moving or facing the
      *correct direction. This *	applies only to tracked vehicles.
      *Wheeled vehicles never rotate to face the *	target, since they
      *aren't maneuverable enough.
      */
      if ((Class->Speed == SPEED_TRACK || *this == UNIT_BIKE) && !NavCom &&
          !IsDriving && PrimaryFacing.Difference(dir)) {
        if (*this == UNIT_FTANK) {
          PrimaryFacing.Set_Desired(
              game::DirectionOf(game::NearestFacing(dir)));
        } else {
          PrimaryFacing.Set_Desired(dir);
        }
      }
    }
  }
}

}  // namespace td
