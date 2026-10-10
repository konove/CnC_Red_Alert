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

/* $Header:   F:\projects\c&c\vcs\code\bullet.cpv   2.18   16 Oct 1995 16:50:00
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BULLET.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : March 18, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * BulletClass::AI -- Logic processing for bullet. *
 *   BulletClass::As_Target -- Converts the bullet into a target value. *
 *   BulletClass::BulletClass -- Bullet constructor. * BulletClass::BulletClass
 *-- Default constructor for bullet objects.                       *
 *   BulletClass::Detach -- Removes specified target from this bullet's
 *targeting system.      * BulletClass::Draw_It -- Displays the bullet at
 *location specified.                        * BulletClass::Init -- Clears the
 *bullets array for scenario preparation.                   * BulletClass::Mark
 *-- Performs related map refreshing under bullet.                        *
 *   BulletClass::Occupy_List -- Determines the bullet occupation list. *
 *   BulletClass::Unlimbo -- Transitions a bullet object into the game
 *render/logic system.    * BulletClass::delete -- Bullet memory delete. *
 *   BulletClass::new -- Allocates memory for bullet object. *
 *   BulletClass::Validate -- validates bullet pointer
 **
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/bullet.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "game/core/game_clock.h"
#include "game/core/object_heap.h"
#include "game/sim/direction.h"
#include "game/sim/heading.h"
#include "game/sim/mph.h"
#include "td/core/config.h"
#include "td/core/coord.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/core/object_heaps.h"
#include "td/core/rand.h"
#include "td/editor/mapedit.h"
#include "td/map/mouse.h"
#include "td/map/world.h"
#include "td/objects/aircraft.h"
#include "td/objects/anim.h"
#include "td/objects/cell.h"
#include "td/objects/combat.h"
#include "td/objects/fly.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"
#include "td/startup/conquer.h"

namespace td {

using engine::gfx::ShapeFlags;

#define GRAVITY 3

/***********************************************************************************************
 * BulletClass::Validate -- validates bullet pointer
 **
 *                                                                                             *
 * INPUT: * none.
 **
 *                                                                                             *
 * OUTPUT: * 1 = ok, 0 = error
 **
 *                                                                                             *
 * WARNINGS: * none.
 **
 *                                                                                             *
 * HISTORY: * 08/09/1995 BRR : Created. *
 *=============================================================================================*/
int BulletClass::Validate() const {
  if constexpr (config::kCheatKeysEnabled) {
    const int num = TheObjectHeaps().bullet().SlotIndexOf(this);
    if (num < 0 || num >= kBulletMax) {
      Validate_Error("BULLET");
    }
    return 1;
  } else {
    return 1;
  }
}

/***********************************************************************************************
 * BulletClass::BulletClass -- Default constructor for bullet objects. *
 *                                                                                             *
 *    This is the default constructor for bullet objects. A bullet constructed
 *by this routine * is not in a usable state for game purposes. It must be
 *constructed by the full           * (parameterized) constructor -- usually
 *called as part of the overloaded "new" operator.  *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Do not use bullets that were constructed solely by this routine.
 **
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. *
 *=============================================================================================*/

/***********************************************************************************************
 * BulletClass::new -- Allocates memory for bullet object. *
 *                                                                                             *
 *    This function will "allocate" a block of memory for a bullet object. *
 *    This memory block is merely lifted from a fixed pool of blocks. *
 *                                                                                             *
 * INPUT:   size  -- The size of the memory block needed. *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to an available bullet object block. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. *
 *=============================================================================================*/
void* BulletClass::operator new(std::size_t /*unused*/) noexcept {
  void* ptr = TheObjectHeaps().bullet().Allocate();
  if (ptr) {
    static_cast<BulletClass*>(ptr)->IsActive = true;
  }
  return ptr;
}

/***********************************************************************************************
 * BulletClass::delete -- Bullet memory delete. *
 *                                                                                             *
 *    Since bullets memory is merely "allocated" out of a pool, it never *
 *    actually gets deleted. *
 *                                                                                             *
 * INPUT:   ptr   -- Generic pointer to bullet object. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. *
 *=============================================================================================*/
void BulletClass::operator delete(void* ptr) {
  if (ptr) {
    static_cast<BulletClass*>(ptr)->IsActive = false;
  }
  TheObjectHeaps().bullet().Free(static_cast<BulletClass*>(ptr));
}

/***********************************************************************************************
 * BulletClass::BulletClass -- Bullet constructor. *
 *                                                                                             *
 *    This is the constructor for the bullet class. It handles all *
 *    initialization of the bullet and starting it in motion toward its *
 *    target. *
 *                                                                                             *
 * INPUT:   id       -- The type of bullet this is (could be missile). *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. * 06/20/1994 JLB : Firer is a base class
 *pointer.                                           * 12/10/1994 JLB : Auto
 *calculate range optional.                                           *
 *   12/12/1994 JLB : Handles small arms as an instantaneous effect. *
 *   12/23/1994 JLB : Fixed scatter algorithm for non-homing projectiles. *
 *   12/31/1994 JLB : Removed range parameter (not needed). *
 *=============================================================================================*/
BulletClass::BulletClass(BulletType id)
    : Class(&BulletTypeClass::As_Reference(id)) {
  //	IsLocked = false;

  Strength = Class->MaxStrength;
}

/***********************************************************************************************
 * BulletClass::Occupy_List -- Determines the bullet occupation list. *
 *                                                                                             *
 *    This function will determine the cell occupation list and return a pointer
 *to it. Most   * bullets are small and the list is usually short, but on
 *occasion, it can be a list that  * rivals the size of regular vehicles. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the cell offset list that covers all the
 *cells a bullet  * is over. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/20/1994 JLB : Created. * 01/05/1995 JLB : Handles projectiles
 *with altitude.                                       *
 *=============================================================================================*/
std::span<const int16_t> BulletClass::Occupy_List(bool /*placement*/) const {
  Validate();
  switch (*this) {
    case BULLET_FLAME:
      return Coord_Spillage_List(Coord, 25);

    case BULLET_NUKE_UP:
    case BULLET_NUKE_DOWN:
      return Coord_Spillage_List(Coord, 48);

    case BulletType::BULLET_NONE:
    case BulletType::BULLET_SNIPER:
    case BulletType::BULLET_BULLET:
    case BulletType::BULLET_APDS:
    case BulletType::BULLET_HE:
    case BulletType::BULLET_SSM:
    case BulletType::BULLET_SSM2:
    case BulletType::BULLET_SAM:
    case BulletType::BULLET_TOW:
    case BulletType::BULLET_CHEMSPRAY:
    case BulletType::BULLET_NAPALM:
    case BulletType::BULLET_GRENADE:
    case BulletType::BULLET_LASER:
    case BulletType::BULLET_HONEST_JOHN:
    case BulletType::BULLET_SPREADFIRE:
    case BulletType::BULLET_HEADBUTT:
    case BulletType::BULLET_TREXBITE:
    case BulletType::BULLET_COUNT:
    default:
      if (Altitude) {
        static int16_t _list[10];
        std::span<const int16_t> ptr = Coord_Spillage_List(Coord, 5);
        int index = 0;
        const Cell cell1 = Cell::Containing(Coord);

        while (base::At(ptr, base::ToSize(index)) != REFRESH_EOL) {
          base::At(_list, index) = base::At(ptr, base::ToSize(index));
          index++;
        }

        Coordinate coord = Coordinate::FromLeptons(0, Altitude);
        coord = Coord - coord;
        const Cell cell2 = Cell::Containing(coord);
        ptr = Coord_Spillage_List(coord, 5);
        while (ptr.front() != REFRESH_EOL) {
          base::At(_list, index++) = static_cast<int16_t>(
              base::ConsumeFront(ptr) + (cell2.index() - cell1.index()));
        }
        base::At(_list, index) = REFRESH_EOL;
        return _list;
      }
  }
  return Coord_Spillage_List(Coord, 10);
}

/***********************************************************************************************
 * BulletClass::Mark -- Performs related map refreshing under bullet. *
 *                                                                                             *
 *    This routine marks the objects under the bullet so that they will * be
 *redrawn. This is necessary as the bullet moves -- objects under * its path
 *need to be restored. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. *
 *=============================================================================================*/
bool BulletClass::Mark(MarkType mark) {
  Validate();
  if (ObjectClass::Mark(mark)) {
    if (!Class->IsInvisible) {
      TheMap().Refresh_Cells(Cell::Containing(Coord), Occupy_List());
    }
    return true;
  }
  return false;
}

/***********************************************************************************************
 * BulletClass::AI -- Logic processing for bullet. *
 *                                                                                             *
 *    This routine will perform all logic (flight) logic on the bullet. *
 *    Primarily this is motion, fuse tracking, and detonation logic. Call * this
 *routine no more than once per bullet per game tick. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/02/1994 JLB : Created. *
 *=============================================================================================*/
void BulletClass::AI() {
  Validate();

  ObjectClass::AI();

  /*
  **	Balistic objects are handled here.
  */
  bool forced = false;  // Forced explosion.
  if (Class->IsArcing) {
    Altitude += Riser;

    if (Altitude <= 0) {
      forced = true;
    }
    if (Riser > -100) {
      Riser -= GRAVITY;
    }
  }
  if (Class->IsDropping) {
    Altitude += Riser;

    if (Altitude <= 0) {
      forced = true;
    }
    if (Riser > -100) {
      Riser -= 1;
    }
  }

  /*
  **	Homing projectiles constantly change facing to face toward the target
  *but *	they only do so every other game frame (improves game speed and
  *makes *	missiles not so deadly).
  */
  if (game::CurrentFrame() % 2 != 0 && Class->IsHoming && TarCom) {
    PrimaryFacing.set_desired(Direction256(Coord, TarCom.AsCoordinate()));
  }

  /*
  **	Move the projectile forward according to its speed
  **	and direction.
  */
  Coordinate coord = Coord;
  if (Class->IsFlameEquipped) {
    if (IsToAnimate) {
      base::NewPooled<AnimClass>(ANIM_SMOKE_PUFF, coord, 1);
    }
    IsToAnimate = !IsToAnimate;
  }

  /*
  **	Handle any body rotation at this time. This process must
  **	occur every game fame in order to achieve smooth rotation.
  */
  if (PrimaryFacing.IsRotating()) {
    PrimaryFacing.Turn(Class->ROT);
  }

  switch (Physics(coord, PrimaryFacing.current())) {
    /*
    **	When a projectile reaches the edge of the world, it
    **	vanishes from existence -- presumed to explode off
    **	map.
    */
    case IMPACT_EDGE:
      //			if (IsLocked) {
      Mark();
      delete this;
      //			}
      break;

    default:
    case IMPACT_NONE:

    /*
    **	The projectile has moved. Check its fuse. If detonation
    **	is signaled, then do so. Otherwise, just move.
    */
    case IMPACT_NORMAL:
      Mark();
      //			IsLocked = true;
      if (!Class->IsHigh) {
        const CellClass* cellptr = &TheMap().at(Cell::Containing(coord));
        if (cellptr->Overlay != OVERLAY_NONE &&
            OverlayTypeClass::As_Reference(cellptr->Overlay).IsHigh) {
          forced = true;
          Coord = coord = Cell::Containing(coord).Center();
        }
      }

      /*
      **	Bullets are generaly more effective when they are fired at
      *aircraft.
      */
      if (Class->IsAntiAircraft && TarCom.AsAircraft() &&
          Distance(TarCom) < 0x0080) {
        forced = true;

        if (*this == BULLET_TOW) {
          Strength = static_cast<int16_t>(Strength + (Strength / 3));
        } else {
          Strength = static_cast<int16_t>(Strength + (Strength / 2));
        }
      }

      if (!forced && (Class->IsDropping || !Fuse_Checkup(coord))) {
        Coord = coord;
        Mark();

        /*
        **	Certain projectiles loose strength when they travel.
        */
        if (*this == BULLET_BULLET) {
          if (Strength > 5) {
            Strength--;
          }
        }

      } else {
        /*
        **	When the target is reached, explode and do the damage
        **	required of it. For homing objects, don't force the explosion to
        **	match the target position. Non-homing projectiles adjust
        *position so *	that they hit the target. This compensates for the error
        *in line of *	flight logic.
        */
        Mark();
        if (!forced && !Class->IsArcing && !Class->IsHoming && Fuse_Target()) {
          Coord = Fuse_Target();
        }

        /*
        **	Non-aircraft targets apply damage to the ground.
        */
        if (TarCom.kind() != KIND_AIRCRAFT ||
            TarCom.AsAircraft()->In_Which_Layer() == LAYER_GROUND) {
          Explosion_Damage(Coord, Strength, Payback, Class->Warhead);
        } else {
          /*
          **	Special damage apply for SAM missiles. This is the only way that
          *missile *	damage affects the aircraft target.
          */
          if (Distance(TarCom) < 0x0080) {
            AircraftClass* object = TarCom.AsAircraft();

            int str = Strength;
            if (object) {
              object->Take_Damage(str, 0, Class->Warhead, Payback);
            }
          }
        }

        /*
        **	For projectiles that are invisible while travelling toward the
        *target, *	allow scatter effect for the impact animation.
        */
        if (Class->IsInvisible) {
          Coord = Coord_Scatter(Coord, 0x0020);
        }
        if (Class->Explosion != ANIM_NONE) {
          base::NewPooled<AnimClass>(Class->Explosion, Coord);
        }
        delete this;
        return;
      }
      break;
  }
}

/***********************************************************************************************
 * BulletClass::Draw_It -- Displays the bullet at location specified. *
 *                                                                                             *
 *    This routine displays the bullet visual at the location specified. *
 *                                                                                             *
 * INPUT:   x,y   -- The center coordinate to render the bullet at. *
 *                                                                                             *
 *          window   -- The window to clip to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/20/1994 JLB : Created. * 06/27/1994 JLB : Takes a window
 *clipping parameter.                                       * 01/08/1995 JLB :
 *Handles translucent colors if necessary.                                 *
 *=============================================================================================*/
void BulletClass::Draw_It(PixelView& view, int x, int y,
                          WindowNumberType window) {
  Validate();
  const int facing = game::BodyFrame32(PrimaryFacing.current());

  /*
  **	Certain projectiles aren't visible. This includes small bullets (which
  *are actually *	invisible) and flame thrower flames (which are rendered
  *as an animation instead of a projectile).
  */
  if (Class->IsInvisible) {
    return;
  }

  /*
  **	If there is no shape loaded for this object, then
  **	it obviously can't be rendered -- just bail.
  */
  const auto shapeptr = Class->Get_Image_Data();
  if (shapeptr.empty()) {
    return;
  }

  /*
  **	Get the basic shape number for this projectile.
  */
  int shapenum = 0;
  if (!Class->IsFaceless) {
    shapenum = base::At(UnitClass::BodyShape, facing);
  }

  /*
  **	For tumbling projectiles, fetch offset stage.
  */
  if (*this == BULLET_NAPALM) {
    shapenum = static_cast<int>(shapenum + (game::CurrentFrame() % 6));
  }

  if (*this == BULLET_GRENADE) {
    shapenum = static_cast<int>(shapenum + (game::CurrentFrame() % 8));
    //		Timer++;
  }

  /*
  **	For flying projectiles, draw the shadow and adjust the actual projectile
  *body *	render position.
  */
  if (Altitude) {
    CC_Draw_Shape(view, shapeptr, shapenum, x, y, window,
                  ShapeFlags::kShadow | ShapeFlags::kCentered);
    y -= Lepton_To_Pixel(Altitude);
  }

  /*
  **	Draw the main body of the projectile.
  */
  ShapeFlags flags = ShapeFlags::kNone;
  if (Class->IsTranslucent) {
    flags = ShapeFlags::kGhost;
  }
  CC_Draw_Shape(view, shapeptr, shapenum, x, y, window,
                flags | ShapeFlags::kCentered, {}, MouseClass::UnitShadow);
}

/***********************************************************************************************
 * BulletClass::Init -- Clears the bullets array for scenario preparation. *
 *                                                                                             *
 *    This routine will zero out the bullet tracking list and object array in
 *preparation for  * the start of a new scenario. All bullets cease to exists
 *after this function is          * called. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/15/1994 JLB : Created. *
 *=============================================================================================*/
void BulletClass::Init() { TheObjectHeaps().bullet().FreeAll(); }

/***********************************************************************************************
 * BulletClass::Detach -- Removes specified target from this bullet's targeting
 *system.        *
 *                                                                                             *
 *    When an object is removed from the game system, it must be removed all
 *targeting and     * tracking systems as well. This routine is used to remove
 *the specified object from the   * bullet. If the object isn't part of this
 *bullet's tracking system, then no action is     * performed. *
 *                                                                                             *
 * INPUT:   target   -- The target to remove from this tracking system. *
 *                                                                                             *
 *          all      -- Is the target going away for good as opposed to just
 *cloaking/hiding?  *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/24/1994 JLB : Created. *
 *=============================================================================================*/
void BulletClass::Detach(Target target, bool all) {
  Validate();
  const ObjectClass* obj = target.AsObject();

  if (obj == Payback) {
    Payback = nullptr;
  }

  if (all && target == TarCom) {
    TarCom = Target{};
  }
}

/***********************************************************************************************
 * BulletClass::Unlimbo -- Transitions a bullet object into the game
 *render/logic system.      *
 *                                                                                             *
 *    This routine is used to take a bullet object that is in limbo and
 *transition it to the   * game system. A bullet object so transitioned, will be
 *drawn and logic processing         * performed. In effect, it comes into
 *existance.                                           *
 *                                                                                             *
 * INPUT:   coord -- The location where the bullet object is to appear. *
 *                                                                                             *
 *          dir   -- The initial facing for the bullet object. *
 *                                                                                             *
 * OUTPUT:  bool; Was the unlimbo successful? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/10/1995 JLB : Created. *
 *=============================================================================================*/
bool BulletClass::Unlimbo(Coordinate coord, game::Direction dir) {
  Validate();
  /*
  **	Try to unlimbo the bullet as far as the base class is concerned. Use the
  *already *	set direction and strength if the "punt" values were passed in.
  *This allows a bullet *	to be setup prior to being launched.
  */
  if (ObjectClass::Unlimbo(coord)) {
    Coordinate tcoord = TarCom.AsCoordinate();

    /*
    **	Homing projectiles (missiles) do NOT override facing. They just fire in
    *the *	direction specified and let the chips fall where they may.
    */
    if (!Class->IsHoming && !Class->IsDropping) {
      dir = DirectionTo(tcoord);
    }

    /*
    **	Possibly adjust the target if this projectile is inaccurate. This occurs
    *whenever *	certain weapons are trained upon targets they were never
    *designed to attack. Example: when *	turrets or anti-tank missiles
    *are fired at infantry. Indirect *	fire is inherently inaccurate.
    */
    if (IsInaccurate || Class->IsInaccurate ||
        ((TarCom.kind() == KIND_CELL || TarCom.kind() == KIND_INFANTRY) &&
         (Class->Warhead == WARHEAD_AP || Class->IsFueled))) {
      /*
      **	Inaccuracy for low velocity or homing projectiles manifests
      *itself as a standard *	Cicular Error of Probability (CEP) algorithm.
      *High speed projectiles usually *	just overshoot the target by extending
      *the straight line flight.
      */
      if (Class->IsHoming || Class->IsArcing) {
        int scatterdist = td::Distance(coord, tcoord) / 3;
        scatterdist = std::min(scatterdist, 0x0200);

        if (*this == BULLET_GRENADE) {
          scatterdist = td::Distance(coord, tcoord) / 4;
          scatterdist = std::min(scatterdist, 0x0080);
        }

        dir = dir + (Random_Pick(0, 10) - 5);
        tcoord = Coord_Scatter(tcoord, Random_Pick(0, scatterdist));
      } else {
        tcoord = Coord_Move(tcoord, dir,
                            static_cast<uint16_t>(Random_Pick(0, 0x0100)));
      }

      /*
      **	Limit scatter to the weapon range of the firer.
      */
      if (Payback &&
          (!Payback->In_Range(tcoord, 0) && !Payback->In_Range(tcoord, 1))) {
        tcoord = Coord_Move(
            tcoord, td::DirectionBetween(tcoord, Coord),
            static_cast<uint16_t>(
                Distance(tcoord) -
                std::max(Payback->Weapon_Range(0), Payback->Weapon_Range(1))));
      }
    }

    /*
    **	For very fast and invisible projectiles, just make the projectile exist
    *at the target *	location and dispense with the actual flight.
    */
    if (Class->MaxSpeed == game::kMphLightSpeed && Class->IsInvisible) {
      Coord = tcoord;
    }

    /*
    **	Set the range equal to either the class defined range or the calculated
    **	number of game frames it would take for the projectile to reach the
    *target.
    */
    int range = 0xFF;
    if (!Class->Range) {
      if (!Class->IsDropping) {
        range = (td::Distance(tcoord, Coord) / Class->MaxSpeed) + 4;
      }
    } else {
      range = Class->Range;
    }

    /*
    **	Projectile speed is usually the default value for that projectile, but
    **	certian projectiles alter speed according to the distance to the
    **	target.
    */
    int speed = Class->MaxSpeed;
    if (speed == game::kMphLightSpeed) {
      speed = game::kMphImmobile;
    }
    if (Class->IsArcing) {
      speed = Class->MaxSpeed + (Distance(tcoord) / 32);

      /*
      **	Set minimum speed (i.e., distance) for arcing projectiles.
      */
      speed = std::max(speed, 25);
    }
    if (!Class->IsDropping) {
      Fly_Speed(255, speed);
    }

    /*
    **	Arm the fuse.
    */
    Arm_Fuse(Coord, tcoord, range,
             TarCom.AsAircraft() != nullptr ? 0 : Class->Arming);

    /*
    **	Projectiles that make a ballistic flight to impact point must determine
    *a *	vertical component for the projectile launch. This is crudely
    *simulated *	by biasing ground speed according to target distance and
    *then giving *	enough vertical velocity to keep the projectile airborne
    *for the *	desired amount of time. The mathematically correct solution
    *would be to *	calculate launch angle (given fixed projectile velocity)
    *and then derive *	the vertical and horizontal components. This solution
    *would require a *	of square root and an arcsine lookup table. OUCH!
    */
    Altitude = 0;
    Riser = 0;
    if (Class->IsArcing) {
      Altitude = 1;
      Riser = static_cast<signed char>(Distance(tcoord) / 2 / (speed + 1) *
                                       GRAVITY);
      Riser = static_cast<signed char>(std::max<int>(Riser, 10));
    }
    if (Class->IsDropping) {
      Altitude = Pixel_To_Lepton(24);
      Riser = 0;
    }

    PrimaryFacing.SnapTo(dir);
    return true;
  }
  return false;
}

/***********************************************************************************************
 * BulletClass::As_Target -- Converts the bullet into a target value. *
 *                                                                                             *
 *    This support routine is used to convert the bullet (as a pointer) into a
 *target          * value (which is a number). *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns the bullet as a target value. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/08/1994 JLB : Created. *
 *=============================================================================================*/
Target BulletClass::As_Target() const {
  Validate();
  return Target::Make(KIND_BULLET, TheObjectHeaps().bullet().SlotIndexOf(this));
}

}  // namespace td
