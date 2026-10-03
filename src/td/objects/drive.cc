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

/* $Header:   F:\projects\c&c\vcs\code\drive.cpv   2.17   16 Oct 1995 16:51:16
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DRIVE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 22, 1994 *
 *                                                                                             *
 *                  Last Update : July 30, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:
 *   DriveClass::AI -- Processes unit movement and rotation.
 *   DriveClass::Approach_Target -- Handles approaching the target in order
 *     to attack it.
 *   DriveClass::Assign_Destination -- Set the unit's NavCom.
 *   DriveClass::Class_Of -- Fetches a reference to the class type for this
 *     object.
 *   DriveClass::Do_Turn -- Tries to turn the vehicle to the specified
 *     direction.
 *   DriveClass::DriveClass -- Constructor for drive class object.
 *   DriveClass::Exit_Map -- Give the unit a movement order to exit the map.
 *   DriveClass::Fixup_Path -- Adds smooth start path to normal movement
 *     path.
 *   DriveClass::Force_Track -- Forces the unit to use the indicated track.
 *   DriveClass::Lay_Track -- Handles track laying logic for the unit.
 *   DriveClass::Offload_Tiberium_Bail -- Offloads one Tiberium quantum from
 *     the object.
 *   DriveClass::Ok_To_Move -- Checks to see if this object can begin
 *     moving.
 *   DriveClass::Overrun_Square -- Handles vehicle overrun of a cell.
 *   DriveClass::Per_Cell_Process -- Handles when unit finishes movement
 *     into a cell.
 *   DriveClass::Smooth_Turn -- Handles the low level coord calc for smooth
 *     turn logic.
 *   DriveClass::Start_Of_Move -- Tries to get a unit to advance toward
 *     cell.
 *   DriveClass::Tiberium_Load -- Determine the Tiberium load as a
 *     percentage.
 *   DriveClass::While_Moving -- Processes unit movement.
 *   DriveClass::Mark_Track -- Marks the midpoint of the track as occupied.
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/objects/drive.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <span>
#include <utility>

#include "base/array.h"
#include "base/buffer.h"
#include "base/cardinal.h"
#include "base/numeric.h"
#include "base/pooled.h"
#include "engine/platform/timer.h"
#include "game/core/cell_size.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "game/sim/facing.h"
#include "magic_enum/magic_enum.hpp"
#include "td/ai/special.h"
#include "td/assets/audio.h"
#include "td/core/const.h"
#include "td/core/defines.h"
#include "td/core/inline.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/objects/building.h"
#include "td/objects/cell.h"
#include "td/objects/foot.h"
#include "td/objects/house.h"
#include "td/objects/object.h"
#include "td/objects/overlay.h"
#include "td/objects/techno.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"
#include "td/startup/game_state.h"

namespace td {

DriveClass::DriveClass() = default;

/***********************************************************************************************
 * DriveClass::Do_Turn -- Tries to turn the vehicle to the specified direction.
 **
 *                                                                                             *
 *    This routine will set the vehicle to rotate to the direction specified.
 *For tracked      * vehicles, it is just a simple rotation. For wheeled
 *vehicles, it performs a series       * of short drives (three point turn) to
 *face the desired direction.                        *
 *                                                                                             *
 * INPUT:   dir   -- The direction that this vehicle should face. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/29/1995 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Do_Turn(game::DirType dir) {
  if (dir != PrimaryFacing) {
    /*
    **	Special rotation track is needed for units that
    **	cannot rotate in place.
    */
    if (TheSpecial().IsThreePoint && TrackNumber == -1 &&
        Class->Speed == SPEED_WHEEL) {
      // Signed difference between current and desired facing, in eighths;
      // the shift floors negative differences as the original did.
      // NOLINTNEXTLINE(bugprone-signed-bitwise)
      int facediff = PrimaryFacing.Difference(dir) >> 5;
      facediff = std::clamp(facediff, -2, 2);
      if (facediff) {
        const game::FacingType face =
            game::Dir_Facing(PrimaryFacing);  // Current facing (ordinal value).

        IsOnShortTrack = true;
        Force_Track(
            (static_cast<int>(face) *
             static_cast<int>(magic_enum::enum_count<game::FacingType>())) +
                (static_cast<int>(face) + facediff),
            Coord);

        base::At(Path, 0) = game::FACING_NONE;
        Set_Speed(0xFF);  // Full speed.
      }
    } else {
      PrimaryFacing.Set_Desired(dir);
      if (TheSpecial().IsJurassic && TheGameState().thingies_enabled() &&
          What_Am_I() == RTTI_UNIT && this->Class->IsPieceOfEight) {
        PrimaryFacing.Set_Current(dir);
      }
    }
  }
}

/***********************************************************************************************
 * DriveClass::Force_Track -- Forces the unit to use the indicated track. *
 *                                                                                             *
 *    This override (nuclear bomb) style routine is to be used when a unit needs
 *to start      * on a movement track but is outside the normal movement system.
 *This occurs when a        * harvester starts driving off of a refinery. *
 *                                                                                             *
 * INPUT:   track -- The track number to start on. *
 *                                                                                             *
 *          coord -- The coordinate that the unit will end up at when the
 *movement track       * is completed. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/17/1995 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Force_Track(int track, Coordinate coord) {
  TrackNumber = track;
  TrackIndex = 0;
  Start_Driver(coord);
}

/***********************************************************************************************
 * DriveClass::Tiberium_Load -- Determine the Tiberium load as a percentage. *
 *                                                                                             *
 *    Use this routine to determine what the Tiberium load is (as a fixed point
 *percentage).   *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the current "fullness" rating for the object. This will
 *be 0x0000 for * empty and 0x0100 for full. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/17/1995 JLB : Created. *
 *=============================================================================================*/
int DriveClass::Tiberium_Load() const {
  if (*this == UNIT_HARVESTER) {
    return base::CardinalToFixed(UnitTypeClass::kStepCount, Tiberium);
  }
  return 0x0000;
}

/***********************************************************************************************
 * DriveClass::Approach_Target -- Handles approaching the target in order to
 *attack it.        *
 *                                                                                             *
 *    This routine will check to see if the target is infantry and it can be
 *overrun. It will  * try to overrun the infantry rather than attack it. This
 *only applies to computer         * controlled vehicles. If it isn't the
 *infantry overrun case, then it falls into the       * base class for normal
 *(complex) approach algorithm.                                      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/17/1995 JLB : Created. * 07/12/1995 JLB : Flamethrower tanks
 *don't overrun -- their weapon is better.              *
 *=============================================================================================*/
void DriveClass::Approach_Target() {
  /*
  **	Only if there is a legal target should the approach check occur.
  */
  if (!House->IsHuman && TarCom && !NavCom) {
    /*
    **	Special case:
    **	If this is for a unit that can crush infantry, and the target is
    **	infantry, AND the infantry is pretty darn close, then just try
    **	to drive over the infantry instead of firing on it.
    */
    const TechnoClass* target = TarCom.AsTechno();
    if (Class->Primary != WEAPON_FLAME_TONGUE && Class->IsCrusher &&
        Distance(TarCom) < 0x0180 && target && target->Class_Of().IsCrushable) {
      Assign_Destination(TarCom);
      return;
    }
  }

  /*
  **	In the other cases, uses the more complex "get to just within weapon
  *range" *	algorithm.
  */
  FootClass::Approach_Target();
}

/***********************************************************************************************
 * DriveClass::Overrun_Square -- Handles vehicle overrun of a cell. *
 *                                                                                             *
 *    This routine is called when a vehicle enters a square or when it is about
 *to enter a     * square (controlled by parameter). When a vehicle that can
 *crush infantry enters a        * cell that contains infantry, then the
 *infantry will be destroyed (regardless of          * affiliation). When a
 *vehicle threatens to overrun a square, all occupying infantry       * will
 *attempt to get out of the way. *
 *                                                                                             *
 * INPUT:   cell     -- The cell that is, or soon will be, entered by a vehicle.
 **
 *                                                                                             *
 *          threaten -- Don't kill, but just threaten to enter the cell. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 01/19/1995 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Overrun_Square(Cell cell, bool threaten) {
  CellClass* cellptr = &TheMap().at(cell);

  if (Class->IsCrusher) {
    if (threaten) {
      /*
      **	If the cell contains infantry, then they will panic when a
      *vehicle tries *	drive over them. Have the infantry run away instead.
      */
      if ((cellptr->Flag.Composite & 0x1F) &&
          ((TheSpecial().IsDifficult || TheSpecial().IsScatter ||
            TheWorld().scenario() > 8) &&
           !TheSpecial().IsEasy))
      /*
      **	Scattering is controlled by the game difficulty level.
      */
      {
        cellptr->Incoming(Coordinate{}, true);
      }

    } else {
      ObjectClass* object = cellptr->Cell_Occupier();
      bool crushed = false;
      while (object) {
        if (object->Class_Of().IsCrushable && !House->Is_Ally(object) &&
            Distance(object->Center_Coord()) < 0x80) {
          ObjectClass* next = object->Next;
          crushed = true;

          /*
          ** Record credit for the kill(s)
          */
          Sound_Effect(VOC_SQUISH2, Coord);
          object->Record_The_Kill(this);
          object->Mark(MARK_UP);
          object->Limbo();
          base::DeletePooled(object);
          base::NewPooled<OverlayClass>(OVERLAY_SQUISH,
                                        Cell::Containing(Coord));

          object = next;
        } else {
          object = object->Next;
        }
      }
      if (crushed) {
        Do_Uncloak();
      }
    }
  }
}

/***********************************************************************************************
 * DriveClass::DriveClass -- Constructor for drive class object. *
 *                                                                                             *
 *    This will initialize the drive class to its default state. It is called as
 *a result      * of creating a unit. *
 *                                                                                             *
 * INPUT:   classid  -- The unit's ID class. It is passed on to the foot class
 *constructor.    *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/13/1994 JLB : Created. *
 *=============================================================================================*/
DriveClass::DriveClass(UnitType classid, HousesType house)
    : FootClass(house), Class(&UnitTypeClass::As_Reference(classid)) {
  /*
  **	For two shooters, clear out the second shot flag -- it will be set the
  *first time *	the object fires. For non two shooters, set the flag since it
  *will never be cleared *	and the second shot flag tells the system that
  *normal rearm times apply -- this is *	what is desired for non two
  *shooters.
  */
  IsSecondShot = Class->IsTwoShooter == 0;

  Strength = Class->MaxStrength;
}

/***********************************************************************************************
 * DriveClass::Exit_Map -- Give the unit a movement order to exit the map. *
 *                                                                                             *
 *    This routine is used to assign an appropriate movement destination for the
 *unit so that  * it will leave the map. The scripts are usually the one to call
 *this routine when it      * is determined that the unit has fulfilled its
 *mission and must "depart".                 *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Exit_Map() {
  Cell cell{};  // Map exit cell number.

  if (*this == UNIT_HOVER && !NavCom) {
    /*
    **	Scan a swath of cells from current position to the edge of the map and
    *if *	there is any blocking object, just wait so to try again later.
    */
    Mark(MARK_UP);
    for (int x = Cell::Containing(Center_Coord()).x() - 1;
         x <= Cell::Containing(Center_Coord()).x() + 1; x++) {
      for (int y = Cell::Containing(Center_Coord()).y() + 1;
           y < TheMap().MapCellY + TheMap().MapCellHeight; y++) {
        cell = Cell::FromXY(x, y);
        if (TheMap().at(cell).Cell_Techno()) {
          Mark(MARK_DOWN);
          return;
        }
      }
    }
    Mark(MARK_DOWN);

    /*
    **	A clear path to the map edge exists. Assign it as the navigation
    *computer *	destination and let the transport move.
    */
    cell = Cell::FromXY(Cell::Containing(Coord).x(),
                        TheMap().MapCellY + TheMap().MapCellHeight);
    IsReturning = true;
    Assign_Destination(Target::FromCellIndex(cell));
  }
}

/***********************************************************************************************
 * DriveClass::Smooth_Turn -- Handles the low level coord calc for smooth turn
 *logic.          *
 *                                                                                             *
 *    This routine calculates the new coordinate value needed for the * smooth
 *turn logic. The adjustment and flag values must be * determined prior to
 *entering this routine.                                               *
 *                                                                                             *
 * INPUT:   adj      -- The adjustment coordinate as lifted from the * correct
 *smooth turn table.                                             *
 *                                                                                             *
 *          dir      -- Pointer to dir for possible modification * according to
 *the flag bits.                                            *
 *                                                                                             *
 * OUTPUT:  Returns with the coordinate the unit should positioned to. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 03/14/1994 JLB : Created. * 07/13/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
Coordinate DriveClass::Smooth_Turn(Coordinate adj, game::DirType* dir) {
  game::DirType workdir = *dir;
  const TrackControlType flags = base::At(TrackControl, TrackNumber).Flag;

  int x = adj.x();
  int y = adj.y();

  if (base::Any(flags & F_T)) {
    const int temp = x;
    x = y;
    y = temp;
    workdir = game::AsDirection(static_cast<int>(game::DIR_W) -
                                static_cast<int>(workdir));
  }

  if (base::Any(flags & F_X)) {
    x = -x;
    workdir = game::AsDirection(-static_cast<int>(workdir));
  }

  if (base::Any(flags & F_Y)) {
    y = -y;
    workdir = game::AsDirection(static_cast<int>(game::DIR_S) -
                                static_cast<int>(workdir));
  }

  *dir = workdir;

  return Coordinate::FromLeptons(Head_To_Coord().x() + x,
                                 Head_To_Coord().y() + y);
}

/***********************************************************************************************
 * DriveClass::Assign_Destination -- Set the unit's NavCom. *
 *                                                                                             *
 *    This routine is used to set the unit's navigation computer to the *
 *    specified target. Once the navigation computer is set, the unit * will
 *start planning and moving toward the destination. *
 *                                                                                             *
 * INPUT:   target   -- The destination target for the unit to head to. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/07/1992 JLB : Created. * 04/15/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void DriveClass::Assign_Destination(Target target) {
  /*
  **	Abort early if there is anything wrong with the parameters
  **	or the unit already is assigned the specified destination.
  */
  if (target == NavCom) {
    return;
  }

#ifdef NEVER
  UnitClass* tunit;  // Destination unit pointer.

  /*
  ** When in move mode, a map position may really indicate
  **	a unit to guard.
  */
  if (target.kind() == KIND_CELL) {
    cell = target.AsCell();

    tunit = TheMap()[cell].Cell_Unit();
    if (tunit) {
      /*
      **	Prevent targeting of itself.
      */
      if (tunit != this) {
        target = tunit->As_Target();
      }
    } else {
      tbuilding = TheMap()[cell].Cell_Building();
      if (tbuilding) {
        target = tbuilding->As_Target();
      }
    }
  }
#endif

  /*
  **	For harvesting type vehicles, it might go into a dock and unload
  *procedure *	when the harvester is full and an empty refinery is selected as
  *a target.
  */
  BuildingClass* b = target.AsBuilding();

  /*
  **	Transport vehicles must tell all passengers that are about to load, that
  *they *	cannot proceed. This is accomplished with a radio message to
  *this effect.
  */
  // if (tunit && In_Radio_Contact() && Class->IsTransporter &&
  // Contact_With_Whom()->Is_Infantry()) {
  if (In_Radio_Contact() && Class->IsTransporter &&
      Contact_With_Whom()->Is_Infantry()) {
    Transmit_Message(RADIO_OVER_OUT);
  }

  /*
  **	If the player clicked on a friendly repair facility and the repair
  **	facility is currently not involved with some other unit (radio or
  *unloading).
  */
  if (b && *b == STRUCT_REPAIR) {
    if (b->In_Radio_Contact()) {
      target = Target{};
    } else {
      /*
      **	Establish radio contact protocol. If the facility responds
      *correctly, *	then remain in radio contact and proceed toward the
      *desired destination.
      */
      if (Transmit_Message(RADIO_HELLO, b) == RADIO_ROGER) {
        /*
        **	Last check to make sure that the loading square is free from
        *permanent *	occupation (such as a building).
        */
        const Cell cell =
            Cell::Containing(b->Center_Coord()).Offset(MAP_CELL_W - 1);
        if (Ground.at(TheMap().at(cell).Land_Type()).Cost.at(Class->Speed)) {
          if (Transmit_Message(RADIO_DOCKING) == RADIO_ROGER) {
            FootClass::Assign_Destination(target);
            base::At(Path, 0) = game::FACING_NONE;
            return;
          }

          /*
          **	Failure to establish a docking relationship with the refinery.
          **	Bail & await further instructions.
          */
          Transmit_Message(RADIO_OVER_OUT);
        }
      }
    }
  }

  /*
  **	Set the unit's navigation computer.
  */
  FootClass::Assign_Destination(target);
  base::At(Path, 0) = game::FACING_NONE;  // Force recalculation of path.
  if (!IsDriving) {
    Start_Of_Move();
  }
}

/***********************************************************************************************
 * DriveClass::While_Moving -- Processes unit movement. *
 *                                                                                             *
 *    This routine is used to process movement for the units as they move. * It
 *is called many times for each cell's worth of movement.   This * routine only
 *applies after the next cell HeadTo has been determined.                     *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  true/false; Should this routine be called again? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/02/1992 JLB : Created. * 04/15/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
bool DriveClass::While_Moving() {
  int actual = 0;  // Working movement addition value.

  /*
  **	Perform quick legality checks.
  */
  if (!IsDriving || TrackNumber == -1 ||
      (IsRotating && !Class->IsTurretEquipped)) {
    SpeedAccum =
        0;  // Kludge?  No speed should accumulate if movement is on hold.
    return false;
  }

  /*
  **	If enough movement has accumulated so that the unit can
  **	visibly move on the map, then process accordingly.
  ** Slow the unit down if he's carrying a flag.
  */
  if (dynamic_cast<UnitClass*>(this)->Flagged != HOUSE_NONE) {
    actual = SpeedAccum + base::FixedToCardinal(
                              static_cast<int>(Class->MaxSpeed) / 2, Speed);
  } else {
    actual = SpeedAccum +
             base::FixedToCardinal(static_cast<int>(Class->MaxSpeed), Speed);
  }

  if (actual > game::kLeptonsPerPixel) {
    int tracknum = 0;  // The track number being processed.

    const TurnTrackType* track = base::Suffix(TrackControl, TrackNumber)
                                     .data();  // Track control pointer.
    if (IsOnShortTrack) {
      tracknum = track->StartTrack;
    } else {
      tracknum = track->Track;
    }
    auto ptr = base::At(RawTracks, tracknum - 1)
                   .Track;  // Pointer to coord offset values.
    const game::FacingType nextface =
        base::At(Path, 0);  // Next facing queued in path.

    /*
    **	Determine if there is a turn coming up. If there is
    **	a turn, then track jumping might occur.
    */
    bool adj = false;  // Is a turn coming up?
    if (nextface != game::FACING_NONE &&
        game::Dir_Facing(track->Facing) != nextface) {
      adj = true;
    }

    /*
    **	Skip ahead the number of track steps required (limited only
    **	by track length). Set the unit to the new position and
    **	flag the unit accordingly.
    */
    Mark(MARK_UP);
    while (actual > game::kLeptonsPerPixel) {
      actual -= game::kLeptonsPerPixel;

      const Coordinate offset = base::At(ptr, base::ToSize(TrackIndex)).Offset;
      if (offset || !TrackIndex) {
        game::DirType dir = base::At(ptr, base::ToSize(TrackIndex)).Facing;
        Coord = Smooth_Turn(offset, &dir);

        PrimaryFacing.Set(dir);

        /*
        **	See if "per cell" processing is necessary.
        */
        if (TrackIndex &&
            base::At(RawTracks, tracknum - 1).Cell == TrackIndex) {
          Per_Cell_Process(false);
          if (!IsActive) {
            return false;
          }
        }

        /*
        **	The unit could "jump tracks". Check to see if the unit should
        **	do so.
        */
        if (*this != UNIT_GUNBOAT && nextface != game::FACING_NONE && adj &&
            base::At(RawTracks, tracknum - 1).Jump == TrackIndex &&
            TrackIndex) {
          const int tnum =
              (static_cast<int>(game::Dir_Facing(track->Facing)) *
               static_cast<int>(magic_enum::enum_count<game::FacingType>())) +
              static_cast<int>(nextface);
          const TurnTrackType* newtrack =
              base::Suffix(TrackControl, tnum)
                  .data();  // Proposed jump-to track.
          if (newtrack->Track &&
              base::At(RawTracks, newtrack->Track - 1).Entry) {
            Coordinate c = Head_To_Coord();
            const int oldspeed = Speed;

            c = Adjacent_Cell(c, nextface);

            switch (Can_Enter_Cell(Cell::Containing(c), nextface)) {
              case game::MOVE_OK:
                IsOnShortTrack = false;  // Shouldn't be necessary, but...
                TrackNumber = tnum;
                track = newtrack;

                tracknum = track->Track;
                TrackIndex = base::At(RawTracks, tracknum - 1).Entry -
                             1;  // Anticipate increment.
                ptr = base::At(RawTracks, tracknum - 1).Track;
                adj = false;

                Stop_Driver();
                Per_Cell_Process(true);
                if (Start_Driver(c)) {
                  Set_Speed(oldspeed);
                  base::MoveBytes(std::as_writable_bytes(base::Suffix(Path, 0)),
                                  std::as_bytes(base::Suffix(Path, 1)),
                                  kConquerPathMax - 1);
                  base::At(Path, kConquerPathMax - 1) = game::FACING_NONE;
                } else {
                  base::At(Path, 0) = game::FACING_NONE;
                  TrackNumber = -1;
                  actual = 0;
                }
                break;

              case game::MOVE_CLOAK:
                TheMap().at(Cell::Containing(c)).Shimmer();
                break;

              case game::MOVE_TEMP:
                if (*this == UNIT_HARVESTER || !House->IsHuman) {
                  const bool old = TheSpecial().IsScatter;
                  TheSpecial().IsScatter = true;
                  TheMap().at(Cell::Containing(c)).Incoming(Coordinate{}, true);
                  TheSpecial().IsScatter = old;
                }
                break;
              case game::MoveType::MOVE_MOVING_BLOCK:
              case game::MoveType::MOVE_DESTROYABLE:
              case game::MoveType::MOVE_NO:
              default:
                break;
            }
          }
        }
        TrackIndex++;

      } else {
        actual = 0;
        Coord = Head_To_Coord();
        Stop_Driver();
        TrackNumber = -1;
        TrackIndex = 0;

        /*
        **	Perform "per cell" activities.
        */
        Per_Cell_Process(true);

        break;
      }
    }
    if (IsActive) {
      Mark(MARK_DOWN);
    }
  }

  /*
  **	Replace any remainder back into the unit's movement
  **	accumulator to be processed next pass.
  */
  SpeedAccum = static_cast<unsigned char>(actual);
  return true;
}

/***********************************************************************************************
 * DriveClass::Per_Cell_Process -- Handles when unit finishes movement into a
 *cell.            *
 *                                                                                             *
 *    This routine is called when a unit has mostly or completely * entered a
 *cell. The unit might be in the middle of a movement track * when this routine
 *is called. It's primary purpose is to perform                          *
 *    sighting and other "per cell" activities. *
 *                                                                                             *
 * INPUT:   center   -- Is the unit safely at the center of a cell?  If it is
 *merely "close"   * to the center, then this parameter will be false. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 11/03/1993 JLB : Created. * 03/30/1994 JLB : Revamped for track
 *system.                                               * 04/15/1994 JLB :
 *Converted to member function.                                            *
 *   06/18/1994 JLB : Converted to virtual function. * 06/18/1994 JLB :
 *Distinguishes between center and near-center conditions.                 *
 *=============================================================================================*/
void DriveClass::Per_Cell_Process(bool center) {
  const Cell cell = Cell::Containing(Coord);

  /*
  **	Check to see if it has reached its destination. If so, then clear the
  *NavCom *	regardless of the remaining path list.
  */
  if (center && NavCom.AsCell() == cell) {
    IsTurretLockedDown = false;
    NavCom = Target{};
    base::At(Path, 0) = game::FACING_NONE;
  }

#ifdef NEVER
  /*
  **	A "lemon" vehicle will have a tendency to break down as
  **	it moves about the terrain.
  */
  if (Is_A_Lemon) {
    if (Random_Pick(1, 4) == 1) {
      Take_Damage(1);
    }
  }
#endif

  Lay_Track();

  FootClass::Per_Cell_Process(center);
}

/***********************************************************************************************
 * DriveClass::Start_Of_Move -- Tries to get a unit to advance toward cell. *
 *                                                                                             *
 *    This will try to start a unit advancing toward the cell it is * facing. It
 *will check for and handle legality and reserving of the * necessary cell. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  true/false; Should this routine be called again because * initial
 *start operation is temporarily delayed?                        *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 02/02/1992 JLB : Created. * 10/18/1993 JLB : This should be called
 *repeatedly until HeadTo is not NULL.               * 03/16/1994 JLB : Revamped
 *for track logic.                                                * 04/15/1994
 *JLB : Converted to member function. * 06/19/1995 JLB : Fixed so that it won't
 *fire on ground unnecessarily.                     * 07/13/1995 JLB : Handles
 *bumping into cloaked objects.                                    *
 *=============================================================================================*/
bool DriveClass::Start_Of_Move() {
  int facediff = 0;     // Difference between current and desired facing.
  int speed = 0;        // Speed of unit.
  Cell destcell{};      // Cell of destination.
  Coordinate dest{};    // Destination coordinate.

  game::FacingType facing =
      base::At(Path, 0);  // Direction movement will commence.

  if (!NavCom && facing == game::FACING_NONE) {
    IsTurretLockedDown = false;
    Stop_Driver();
    if (Mission == MISSION_MOVE) {
      Enter_Idle_Mode();
    }
    return false;  // Why is it calling this routine!?!
  }

#ifdef NEVER
  /*
  **	Movement start logic can't begin until a unit that requires
  **	a locked down turret gets to a locked down state (i.e., the
  **	turret rotation stops.
  */
  if (ClassF & CLASSF_LOCKTURRET) {
    Set_Secondary_Facing(facing << 5);
    if (Is_Rotating) {
      return (true);
    }
  }
#endif

  /*
  **	Reduce the path length if the target is a unit and the
  **	range to the unit is less than the precalculated path steps.
  */
  if (facing != game::FACING_NONE &&
      (NavCom.kind() == KIND_UNIT || NavCom.kind() == KIND_INFANTRY)) {
    const int dist = Lepton_To_Cell(Distance(NavCom));

    //			if (dist > game::kCellWidthLeptons ||
    //				!NavCom.AsTechno()->Techno_Type_Class()->IsCrushable
    //|| 				!Class->IsCrusher) {

    if (dist < kConquerPathMax) {
      base::At(Path, dist) = game::FACING_NONE;
      facing = base::At(Path, 0);  // Maybe needed.
    }
    //			}
  }

  /*
  **	If the path is invalid at this point, then generate one. If
  **	generating a new path fails, then abort NavCom.
  */
  if (facing == game::FACING_NONE) {
    /*
    **	If after a path search, there is still no valid path, then set the
    **	NavCom to null and let the script take care of assigning a new
    **	navigation target.
    */
    if (!PathDelay.IsFinished()) {
      return false;
    }
    if (!Basic_Path()) {
      if (Distance(NavCom) < 0x0280 &&
          (Mission == MISSION_MOVE || Mission == MISSION_GUARD_AREA)) {
        Assign_Destination(Target{});
      } else {
        /*
        **	If a basic path could be found, but the immediate move
        *destination is *	blocked by a friendly temporary blockage, then
        *cause that blockage *	to scatter.
        */
        const Cell cell = Adjacent_Cell(Cell::Containing(Center_Coord()),
                                        PrimaryFacing.Current());
        if (TheMap().In_Radar(cell) &&
            (Can_Enter_Cell(cell) == game::MOVE_TEMP)) {
          CellClass* cellptr = &TheMap().at(cell);
          const TechnoClass* blockage = cellptr->Cell_Techno();
          if (blockage && House->Is_Ally(blockage)) {
            const bool old = TheSpecial().IsScatter;
            TheSpecial().IsScatter = true;
            cellptr->Incoming(Coordinate{}, true);
            TheSpecial().IsScatter = old;
          }
        }

        if (TryTryAgain) {
          TryTryAgain--;
        } else {
          Assign_Destination(Target{});
          if (IsNewNavCom) {
            Sound_Effect(VOC_SCOLD);
          }
          IsNewNavCom = false;
        }
      }
      Stop_Driver();
      TrackNumber = -1;
      IsTurretLockedDown = false;
      return false;
    }

    /*
    **	If a basic path could be found, but the immediate move destination is
    **	blocked by a friendly temporary blockage, then cause that blockage
    **	to scatter.
    */
    const Cell cell =
        Adjacent_Cell(Cell::Containing(Center_Coord()), base::At(Path, 0));
    if (TheMap().In_Radar(cell) && (Can_Enter_Cell(cell) == game::MOVE_TEMP)) {
      CellClass* cellptr = &TheMap().at(cell);
      const TechnoClass* blockage = cellptr->Cell_Techno();
      if (blockage && House->Is_Ally(blockage)) {
        const bool old = TheSpecial().IsScatter;
        TheSpecial().IsScatter = true;
        cellptr->Incoming(Coordinate{}, true);
        TheSpecial().IsScatter = old;
      }
    }

    TryTryAgain = kPathRetry;
    facing = base::At(Path, 0);
  }

  if (Class->IsLockTurret || !Class->IsTurretEquipped) {
    IsTurretLockedDown = true;
  }

#ifdef NEVER
  /*
  **	If the turret needs to match the body's facing before
  **	movement can occur, then start it's rotation and
  **	don't start a movement track until it is aligned.
  */
  if (!Ok_To_Move(BodyFacing)) {
    return (true);
  }
#endif

  /*
  **	Determine the coordinate of the next cell to move into.
  */
  dest = Adjacent_Cell(Coord, facing);
  const game::DirType dir =
      game::Facing_Dir(facing);  // Desired actual facing toward destination.

  /*
  **	Set the facing correctly if it isn't already correct. This
  **	means starting a rotation track if necessary.
  */
  facediff = PrimaryFacing.Difference(dir);
  if (facediff) {
    /*
    **	Request a change of facing.
    */
    Do_Turn(dir);
    return true;
  }
  /* NOTE:  Beyond this point, actual track assignment can begin.
   **
   **	If the cell to move into is impassable (probably for some unexpected
   **	reason), then abort the path list and set the speed to zero. The
   ** next time this routine is called, a new path will be generated.
   */
  destcell = Cell::Containing(dest);
  Mark(MARK_UP);
  game::MoveType cando = Can_Enter_Cell(destcell, facing);
  Mark(MARK_DOWN);

  if (cando != game::MOVE_OK) {
    if (Mission == MISSION_MOVE && House->IsHuman &&
        Distance(NavCom) < 0x0200) {
      Assign_Destination(Target{});
    }

    /*
    **	If a temporary friendly object is blocking the path, then cause
    *it to *	get out of the way.
    */
    if (cando == game::MOVE_TEMP) {
      const bool old = TheSpecial().IsScatter;
      TheSpecial().IsScatter = true;
      TheMap().at(destcell).Incoming(Coordinate{}, true);
      TheSpecial().IsScatter = old;
    }

    /*
    **	If a cloaked object is blocking, then shimmer the cell.
    */
    if (cando == game::MOVE_CLOAK) {
      TheMap().at(destcell).Shimmer();
    }

    Stop_Driver();
    if (cando != game::MOVE_MOVING_BLOCK) {
      base::At(Path, 0) = game::FACING_NONE;  // Path is blocked!
    }

    /*
    ** If blocked by a moving block then just exit start of move and
    ** try again next tick.
    */
    if (cando == game::MOVE_DESTROYABLE) {
      if (TheMap().at(destcell).Cell_Object()) {
        if (!House->Is_Ally(TheMap().at(destcell).Cell_Object())) {
          Override_Mission(MISSION_ATTACK,
                           TheMap().at(destcell).Cell_Object()->As_Target(),
                           Target{});
        }
      } else {
        if (TheMap().at(destcell).Overlay != OVERLAY_NONE &&
            OverlayTypeClass::As_Reference(TheMap().at(destcell).Overlay)
                .IsWall) {
          Override_Mission(MISSION_ATTACK, Target::FromCellIndex(destcell),
                           Target{});
        }
      }
    } else {
      if (IsNewNavCom) {
        Sound_Effect(VOC_SCOLD);
      }
    }
    IsNewNavCom = false;
    TrackNumber = -1;
    return true;
  }

  /*
  **	Determine the speed that the unit can travel to the desired square.
  */
  const LandType ground =
      TheMap().at(destcell).Land_Type();  // Ground unit is entering.
  speed = Ground.at(ground).Cost.at(Class->Speed);
  if (!speed) {
    speed = 128;
  }

#ifdef NEVER
  /*
  **	Set the jiggle flag if the terrain would cause the unit
  **	to jiggle when travelled over.
  */
  BaseF &= ~BASEF_JIGGLE;
  if (Ground[ground].Jiggle) {
    BaseF |= BASEF_JIGGLE;
  }
#endif

  /*
  **	A damaged unit has a reduced speed.
  */
  if (Class->MaxStrength / 2 > Strength) {
    speed -= speed / 4;  // Three quarters speed.
  }
  if (std::cmp_not_equal(speed, Speed) /* || !SpeedAdd*/) {
    Set_Speed(speed);  // Full speed.
  }

  /*
  **	Reserve the destination cell so that it won't become
  **	occupied AS this unit is moving into it.
  */
  if (cando != game::MOVE_OK) {
    base::At(Path, 0) = game::FACING_NONE;  // Path is blocked!
    TrackNumber = -1;
    dest = {};
  } else {
    Overrun_Square(Cell::Containing(dest), true);

    /*
    **	Determine which track to use (based on recorded path).
    */
    game::FacingType nextface = base::At(Path, 1);
    if (nextface == game::FACING_NONE) {
      nextface = facing;
    }

    IsOnShortTrack = false;
    TrackNumber =
        (static_cast<int>(facing) *
         static_cast<int>(magic_enum::enum_count<game::FacingType>())) +
        static_cast<int>(nextface);
    if (base::At(TrackControl, TrackNumber).Track == 0) {
      base::At(Path, 0) = game::FACING_NONE;
      TrackNumber = -1;
      return true;
    }
    if (base::Any(base::At(TrackControl, TrackNumber).Flag & F_D)) {
      /*
      **	If the middle cell of a two cell track contains a crate,
      **	the check for goodies before movement starts.
      */
      if (!TheMap().at(destcell).Goodie_Check(this)) {
        cando = game::MOVE_NO;
      } else {
        dest = Adjacent_Cell(dest, nextface);
        destcell = Cell::Containing(dest);
        cando = Can_Enter_Cell(destcell);
      }

      if (cando != game::MOVE_OK) {
        /*
        **	If a temporary friendly object is blocking the path, then cause
        *it to *	get out of the way.
        */
        if (cando == game::MOVE_TEMP) {
          const bool old = TheSpecial().IsScatter;
          TheSpecial().IsScatter = true;
          TheMap().at(destcell).Incoming(Coordinate{}, true);
          TheSpecial().IsScatter = old;
        }

        /*
        **	If a cloaked object is blocking, then shimmer the cell.
        */
        if (cando == game::MOVE_CLOAK) {
          TheMap().at(destcell).Shimmer();
        }

        base::At(Path, 0) = game::FACING_NONE;  // Path is blocked!
        TrackNumber = -1;
        dest = {};
        if (cando == game::MOVE_DESTROYABLE) {
          if (TheMap().at(destcell).Cell_Object()) {
            if (!House->Is_Ally(TheMap().at(destcell).Cell_Object())) {
              Override_Mission(MISSION_ATTACK,
                               TheMap().at(destcell).Cell_Object()->As_Target(),
                               Target{});
            }
          } else {
            if (TheMap().at(destcell).Overlay != OVERLAY_NONE &&
                OverlayTypeClass::As_Reference(TheMap().at(destcell).Overlay)
                    .IsWall) {
              Override_Mission(MISSION_ATTACK, Target::FromCellIndex(destcell),
                               Target{});
            }
          }
          IsNewNavCom = false;
          TrackIndex = 0;
          return true;
        }
      } else {
        base::MoveBytes(std::as_writable_bytes(base::Suffix(Path, 0)),
                        std::as_bytes(base::Suffix(Path, 2)),
                        kConquerPathMax - 2);
        base::At(Path, kConquerPathMax - 2) = game::FACING_NONE;
        IsPlanningToLook = true;
      }
    } else {
      base::MoveBytes(std::as_writable_bytes(base::Suffix(Path, 0)),
                      std::as_bytes(base::Suffix(Path, 1)),
                      kConquerPathMax - 1);
    }
    base::At(Path, kConquerPathMax - 1) = game::FACING_NONE;
  }

  IsNewNavCom = false;
  TrackIndex = 0;
  if (!Start_Driver(dest)) {
    TrackNumber = -1;
    base::At(Path, 0) = game::FACING_NONE;
    Set_Speed(0);
  }
  return false;
}

/***********************************************************************************************
 * DriveClass::AI -- Processes unit movement and rotation. *
 *                                                                                             *
 *    This routine is used to process unit movement and rotation. It * functions
 *autonomously from the script system. Thus, once a unit * is give rotation
 *command or movement path, it will follow this                           *
 *    until specifically instructed to stop. The advantage of this * method is
 *that it allows smooth movement of units, faster game * execution, and reduced
 *script complexity (since actual movement                          * dynamics
 *need not be controlled directly by the scripts). *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   This routine relies on the process control bits for the *
 *             specified unit (for speed reasons). Thus, only setting *
 *             movement, rotation, or path list will the unit perform * any
 *physics.                                                                    *
 *                                                                                             *
 * HISTORY: * 09/26/1993 JLB : Created. * 04/15/1994 JLB : Converted to member
 *function.                                            *
 *=============================================================================================*/
void DriveClass::AI() {
  FootClass::AI();

  /*
  **	If the unit is following a track, then continue
  **	to do so -- mindlessly.
  */
  if (TrackNumber != -1) {
    /*
    **	Perform the movement accumulation.
    */
    While_Moving();
    if (!IsActive) {
      return;
    }
    if (TrackNumber == -1 &&
        (NavCom || base::At(Path, 0) != game::FACING_NONE)) {
      Start_Of_Move();
      While_Moving();
      if (!IsActive) {
        return;
      }
    }

  } else {
    /*
    **	For tracked units that are rotating in place, perform the rotation now.
    */
    if ((Class->Speed == SPEED_FLOAT || Class->Speed == SPEED_HOVER ||
         Class->Speed == SPEED_TRACK ||
         (Class->Speed == SPEED_WHEEL && !TheSpecial().IsThreePoint)) &&
        PrimaryFacing.Is_Rotating()) {
      if (PrimaryFacing.Rotation_Adjust(Class->ROT)) {
        Mark(MARK_CHANGE);
      }
      if (!IsRotating) {
        Per_Cell_Process(true);
        if (!IsActive) {
          return;
        }
      }

    } else {
      /*
      **	The unit has no track to follow, but if there
      **	is a navigation target or a remaining path,
      **	then start on a new track.
      */
      if (Mission != MISSION_GUARD || NavCom != Target{}) {
        if (NavCom || base::At(Path, 0) != game::FACING_NONE) {
          Start_Of_Move();
          While_Moving();
          if (!IsActive) {
            return;
          }
        } else {
          Stop_Driver();
        }
      }
    }
  }
}

/***********************************************************************************************
 * DriveClass::Fixup_Path -- Adds smooth start path to normal movement path. *
 *                                                                                             *
 *    This routine modifies the path of the specified unit so that it * will not
 *start out with a rotation. This is necessary for those * vehicles that have
 *difficulty with rotating in place. Typically,                         * this
 *includes wheeled vehicles. *
 *                                                                                             *
 * INPUT:   unit  -- Pointer to the unit to adjust. *
 *                                                                                             *
 *          path  -- Pointer to path structure. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   Only units that require a fixup get modified. The * modification
 *only occurs, if there is a legal path to                           * do so. *
 *                                                                                             *
 * HISTORY: * 04/03/1994 JLB : Created. * 04/06/1994 JLB : Uses path structure.
 ** 04/10/1994 JLB : Diagonal smooth turn added. * 04/15/1994 JLB : Converted to
 *member function.                                            *
 *=============================================================================================*/
void DriveClass::Fixup_Path(PathType* path) {
  game::FacingType stage[6] = {
      game::FACING_N, game::FACING_N, game::FACING_N,
      game::FACING_N, game::FACING_N, game::FACING_N};  // Prefix path elements.
  static game::FacingType _path[4][6] = {
      {static_cast<game::FacingType>(2), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(3), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(4), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(4), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(1), static_cast<game::FacingType>(0)}};
  static game::FacingType _dpath[4][6] = {
      {static_cast<game::FacingType>(0), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(3), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(0), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(4), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(1), static_cast<game::FacingType>(0)},
      {static_cast<game::FacingType>(5), static_cast<game::FacingType>(0),
       static_cast<game::FacingType>(2), static_cast<game::FacingType>(2),
       static_cast<game::FacingType>(1), static_cast<game::FacingType>(0)}};

  int counter = 0;            // Path addition
  std::span<game::FacingType> ptr;  // Path list pointer.

  /*
  **	Verify that the unit is valid and there is a path problem to resolve.
  */
  if (!path || base::At(path->Command, 0) == game::FACING_NONE) {
    return;
  }

  /*
  **	Only wheeled vehicles need a path fixup -- to avoid 3 point turns.
  */
  if (!TheSpecial().IsThreePoint || Class->Speed != SPEED_WHEEL) {
    return;
  }

  /*
  **	If the original path starts in the same direction as the unit, then
  **	there is no problem to resolve -- abort.
  */
  const auto first_direction = game::Facing_Dir(base::At(path->Command, 0));
  // The facing difference value (0..4 | 0..-4); the shift floors negative
  // differences as the original did.
  // NOLINTNEXTLINE(bugprone-signed-bitwise)
  int facediff = PrimaryFacing.Difference(first_direction) >> 5;

  if (!facediff) {
    return;
  }

  // The diagonal facings are the odd ones.
  if (static_cast<int>(game::Dir_Facing(PrimaryFacing)) % 2 != 0) {
    ptr = base::Suffix(base::At(_dpath, std::abs(facediff) - 1),
                       1);  // Pointer to path adjust list.
    counter =
        static_cast<int>(base::At(base::At(_dpath, std::abs(facediff) - 1),
                                  0));  // Number of path adjusts.
  } else {
    ptr = base::Suffix(base::At(_path, std::abs(facediff) - 1),
                       1);  // Pointer to path adjust list.
    counter = static_cast<int>(base::At(base::At(_path, std::abs(facediff) - 1),
                                        0));  // Number of path adjusts.
  }
  const auto ptr2 = ptr;  // Copy of new path list pointer.

  bool ok = true;  // Presume adjustment is all ok.
  Cell cell = Cell::Containing(
      Coord);  // Working cell value.              // Starting cell.
  game::FacingType nextpath =
      game::Dir_Facing(PrimaryFacing);  // Next path value.  // Starting path.
  for (int index = 0; index < counter; index++) {
    /*
    **	Determine next path element and add it to the
    **	working path list.
    */
    if (facediff > 0) {
      nextpath = nextpath + base::ConsumeFront(ptr);
    } else {
      nextpath = nextpath - base::ConsumeFront(ptr);
    }
    base::At(stage, index) = nextpath;
    cell = Adjacent_Cell(cell, nextpath);
    // cell = Cell::Containing(Adjacent_Cell(cell.Center(), nextpath));

    /*
    **	If it can't enter this cell, then abort the path
    **	building operation without adjusting the unit's
    **	path.
    */
    if (Can_Enter_Cell(cell, nextpath) != game::MOVE_OK) {
      ok = false;
      break;
    }
  }

  /*
  **	If veering to the left was not successful, then try veering
  **	to the right. This only makes sense if the vehicle is trying
  **	to turn 180 degrees.
  */
  if (!ok && std::abs(facediff) == 4) {
    ptr = ptr2;  // Pointer to path adjust list.
    facediff = -facediff;
    ok = true;                             // Presume adjustment is all ok.
    cell = Cell::Containing(Coord);        // Starting cell.
    nextpath = game::Dir_Facing(PrimaryFacing);  // Starting path.
    for (int index = 0; index < counter; index++) {
      /*
      **	Determine next path element and add it to the
      **	working path list.
      */
      if (facediff > 0) {
        nextpath = nextpath + base::ConsumeFront(ptr);
      } else {
        nextpath = nextpath - base::ConsumeFront(ptr);
      }
      base::At(stage, index) = nextpath;
      cell = Cell::Containing(Adjacent_Cell(cell.Center(), nextpath));

      /*
      **	If it can't enter this cell, then abort the path
      **	building operation without adjusting the unit's
      **	path.
      */
      if (Can_Enter_Cell(cell, nextpath) != game::MOVE_OK) {
        ok = false;
        break;
      }
    }
  }

  /*
  **	If a legal path addition was created, then install it in place
  **	of the first path value. The initial path entry is to be replaced
  **	with a sequence of path entries that create smooth turning.
  */
  if (ok) {
    if (path->Length <= 1) {
      std::ranges::copy(
          std::span(stage).first(base::ToSize(std::max(counter, 1))),
          path->Command.begin());
      path->Length = counter;
    } else {
      /*
      **	Optimize the transition path step from the smooth turn
      **	first part as it joins with the rest of the normal
      **	path. The normal prefix path steps are NOT to be optimized.
      */
      if (counter) {
        counter--;
        base::At(path->Command, 0) = base::At(stage, counter);
        Optimize_Moves(path, game::MOVE_OK);
      }

      /*
      **	If there is more than one prefix path element, then
      **	insert the rest now.
      */
      if (counter) {
        base::MoveBytes(std::as_writable_bytes(
                            path->Command.subspan(base::ToSize(counter))),
                        std::as_bytes(path->Command),
                        base::ToSize(40 - counter));
        base::CopyBytes(std::as_writable_bytes(path->Command),
                        base::ObjectBytes(stage), base::ToSize(counter));
        path->Length += counter;
      }
    }
    base::At(path->Command, base::ToSize(path->Length)) = game::FACING_NONE;
  }
}

/***********************************************************************************************
 * DriveClass::Lay_Track -- Handles track laying logic for the unit. *
 *                                                                                             *
 *    This routine handles the track laying for the unit. This entails examining
 *the unit's    * current location as well as the direction and whether this
 *unit is allowed to lay        * tracks in the first place. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/28/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Lay_Track() {
#ifdef NEVER
  static IconCommandType* _trackdirs[8] = {TrackN_S,   TrackNE_SW, TrackE_W,
                                           TrackNW_SE, TrackN_S,   TrackNE_SW,
                                           TrackE_W,   TrackNW_SE};

  if (!(ClassF & CLASSF_TRACKS)) {
    return;
  }

  Icon_Install(Cell::Containing(Coord), _trackdirs[Facing_To_8(BodyFacing)]);
#endif
}

/***********************************************************************************************
 * DriveClass::Mark_Track -- Marks the midpoint of the track as occupied. *
 *                                                                                             *
 *    This routine will ensure that the midpoint (if any) of the track that the
 *unit is        * following, will be marked according to the mark type
 *specified.                          *
 *                                                                                             *
 * INPUT:   headto   -- The head to coordinate. *
 *                                                                                             *
 *          type     -- The type of marking to perform. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/30/1995 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Mark_Track(Coordinate headto, MarkType type) {
  const bool value = type != MARK_UP;

  if (headto) {
    if (!IsOnShortTrack && TrackNumber != -1) {
      /*
      ** If we have not passed the per cell process point we need
      ** to deal with it.
      */
      const int tracknum = base::At(TrackControl, TrackNumber).Track;
      if (tracknum) {
        const auto ptr = base::At(RawTracks, tracknum - 1).Track;
        const int cellidx = base::At(RawTracks, tracknum - 1).Cell;
        if (cellidx > -1) {
          game::DirType dir = base::At(ptr, base::ToSize(cellidx)).Facing;

          if (TrackIndex < cellidx && cellidx != -1) {
            const Coordinate offset =
                Smooth_Turn(base::At(ptr, base::ToSize(cellidx)).Offset, &dir);
            TheMap().at(Cell::Containing(offset)).Flag.Occupy.Vehicle = value;
          }
        }
      }
    }
    TheMap().at(Cell::Containing(headto)).Flag.Occupy.Vehicle = value;
  }
}

/***********************************************************************************************
 * DriveClass::Offload_Tiberium_Bail -- Offloads one Tiberium quantum from the
 *object.         *
 *                                                                                             *
 *    This routine will offload one Tiberium packet/quantum/bail from the
 *object. Multiple     * calls to this routine are needed in order to fully
 *offload all Tiberium.                 *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of credits offloaded for the one call. If
 *zero is returned,* then this indicates that all Tiberium has been offloaded. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/19/1995 JLB : Created. *
 *=============================================================================================*/
int DriveClass::Offload_Tiberium_Bail() {
  if (Tiberium) {
    Tiberium--;
    if (House->IsHuman) {
      return UnitTypeClass::kFullLoadCredits / UnitTypeClass::kStepCount;
    }
    return UnitTypeClass::kFullLoadCredits +
           (UnitTypeClass::kFullLoadCredits / 3 / UnitTypeClass::kStepCount);
  }
  return 0;
}

/***********************************************************************************************
 * DriveClass::Ok_To_Move -- Checks to see if this object can begin moving. *
 *                                                                                             *
 *    This routine is used to verify that this object is allowed to move. Some
 *objects can     * be temporarily occupied and thus cannot move until the
 *situation permits.                *
 *                                                                                             *
 * INPUT:   direction   -- The direction that movement would be desired. *
 *                                                                                             *
 * OUTPUT:  Can the unit move in the direction specified? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
bool DriveClass::Ok_To_Move(game::DirType /*unused*/) { return true; }

/***********************************************************************************************
 * DriveClass::Class_Of -- Fetches a reference to the class type for this
 *object.              *
 *                                                                                             *
 *    This routine will fetch a reference to the TypeClass of this object. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with reference to the type class of this object. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/29/1995 JLB : Created. *
 *=============================================================================================*/
const ObjectTypeClass& DriveClass::Class_Of() const { return *Class; }

/***************************************************************************
**	Smooth turn track tables. These are coordinate offsets from the center
**	of the destination cell. These are the raw tracks that are modified
**	by negating the X and Y portions as necessary. Also for reverse
*travelling *	direction, the track list can be processed backward.
**
**	Track 1 = N
**	Track 2 = NE
**	Track 3 = N->NE 45 deg (double path consumption)
**	Track 4 = N->E 90 deg (double path consumption)
**	Track 5 = NE->SE 90 deg (double path consumption)
** Track 6 = NE->N 45 deg (double path consumption)
**	Track 7 = N->NE (facing change only)
**	Track 8 = NE->E (facing change only)
**	Track 9 = N->E (facing change only)
**	Track 10= NE->SE (facing change only)
**	Track 11= back up into refinery
**	Track 12= drive out of refinery
*/
const DriveClass::TrackType DriveClass::Track1[24] = {
    {Coordinate::FromLeptons(0x0000, 0x00F5), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00EA), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00DF), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00D4), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00C9), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00BE), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00B3), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x00A8), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x009D), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0092), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0087), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x007C),
     static_cast<game::DirType>(0)},  // Track jump check here.
    {Coordinate::FromLeptons(0x0000, 0x0071), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0066), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x005B), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0050), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0045), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x003A), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x002F), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0024), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0019), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x000E), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0000, 0x0003), static_cast<game::DirType>(0)},
    {Coordinate{}, static_cast<game::DirType>(0)}};

const DriveClass::TrackType DriveClass::Track2[] = {
    {Coordinate::FromLeptons(0xFF08, 0x00F8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF10, 0x00F0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF18, 0x00E8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF20, 0x00E0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF28, 0x00D8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF30, 0x00D0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF38, 0x00C8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF40, 0x00C0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF48, 0x00B8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF50, 0x00B0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF58, 0x00A8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF60, 0x00A0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF68, 0x0098), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF70, 0x0090), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF78, 0x0088), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF80, 0x0080),
     static_cast<game::DirType>(32)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0x0078), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF90, 0x0070), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF98, 0x0068), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFA0, 0x0060), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFA8, 0x0058), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFB0, 0x0050), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFB8, 0x0048), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFC0, 0x0040), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFC8, 0x0038), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFD0, 0x0030), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFD8, 0x0028), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFE0, 0x0020), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFE8, 0x0018), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF0, 0x0010), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF8, 0x0008), static_cast<game::DirType>(32)},
    {Coordinate{}, static_cast<game::DirType>(32)}};

const DriveClass::TrackType DriveClass::Track3[] = {
    {Coordinate::FromLeptons(0xFF00, 0x01F5), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01EA), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01DF), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01D4), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01C9), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01BE), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01B3), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01A8), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x019D), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0192), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0187), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0180), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0175),
     static_cast<game::DirType>(0)},  // Jump entry point here.
    {Coordinate::FromLeptons(0xFF00, 0x016B), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF02, 0x0160), static_cast<game::DirType>(1)},
    {Coordinate::FromLeptons(0xFF04, 0x0155), static_cast<game::DirType>(3)},
    {Coordinate::FromLeptons(0xFF06, 0x014C), static_cast<game::DirType>(4)},
    {Coordinate::FromLeptons(0xFF08, 0x0141), static_cast<game::DirType>(5)},
    {Coordinate::FromLeptons(0xFF0B, 0x0137), static_cast<game::DirType>(7)},
    {Coordinate::FromLeptons(0xFF0F, 0x012E), static_cast<game::DirType>(8)},
    {Coordinate::FromLeptons(0xFF13, 0x0124), static_cast<game::DirType>(9)},
    {Coordinate::FromLeptons(0xFF17, 0x011A), static_cast<game::DirType>(11)},
    {Coordinate::FromLeptons(0xFF1B, 0x0110), static_cast<game::DirType>(12)},
    {Coordinate::FromLeptons(0xFF1F, 0x0107),
     static_cast<game::DirType>(13)},  // Center cell processing here.
    {Coordinate::FromLeptons(0xFF24, 0x00FC), static_cast<game::DirType>(15)},
    {Coordinate::FromLeptons(0xFF28, 0x00F3), static_cast<game::DirType>(16)},
    {Coordinate::FromLeptons(0xFF2C, 0x00EC), static_cast<game::DirType>(17)},
    {Coordinate::FromLeptons(0xFF32, 0x00E0), static_cast<game::DirType>(19)},
    {Coordinate::FromLeptons(0xFF36, 0x00D7), static_cast<game::DirType>(20)},
    {Coordinate::FromLeptons(0xFF3D, 0x00CF), static_cast<game::DirType>(21)},
    {Coordinate::FromLeptons(0xFF42, 0x00C6), static_cast<game::DirType>(23)},
    {Coordinate::FromLeptons(0xFF49, 0x00BA), static_cast<game::DirType>(24)},
    {Coordinate::FromLeptons(0xFF4D, 0x00B0), static_cast<game::DirType>(25)},
    {Coordinate::FromLeptons(0xFF58, 0x00A8), static_cast<game::DirType>(27)},
    {Coordinate::FromLeptons(0xFF60, 0x00A0), static_cast<game::DirType>(28)},
    {Coordinate::FromLeptons(0xFF68, 0x0098), static_cast<game::DirType>(29)},
    {Coordinate::FromLeptons(0xFF70, 0x0090), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFF78, 0x0088), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF80, 0x0080),
     static_cast<game::DirType>(32)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0x0078), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF90, 0x0070), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF98, 0x0068), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFA0, 0x0060), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFA8, 0x0058), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFB0, 0x0050), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFB8, 0x0048), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFC0, 0x0040), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFC8, 0x0038), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFD0, 0x0030), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFD8, 0x0028), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFE0, 0x0020), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFE8, 0x0018), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF0, 0x0010), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF8, 0x0008), static_cast<game::DirType>(32)},
    {Coordinate{}, static_cast<game::DirType>(32)}};

const DriveClass::TrackType DriveClass::Track4[] = {
    {Coordinate::FromLeptons(0xFF00, 0x00F5), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00EB), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00E0), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00D5), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF01, 0x00CB), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF03, 0x00C0), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFF05, 0x00B5), static_cast<game::DirType>(1)},
    {Coordinate::FromLeptons(0xFF07, 0x00AB), static_cast<game::DirType>(1)},
    {Coordinate::FromLeptons(0xFF0A, 0x00A0), static_cast<game::DirType>(2)},
    {Coordinate::FromLeptons(0xFF0D, 0x0095), static_cast<game::DirType>(3)},
    {Coordinate::FromLeptons(0xFF10, 0x008B), static_cast<game::DirType>(4)},
    {Coordinate::FromLeptons(0xFF14, 0x0080),
     static_cast<game::DirType>(5)},  // Track entry here.
    {Coordinate::FromLeptons(0xFF18, 0x0075), static_cast<game::DirType>(8)},
    {Coordinate::FromLeptons(0xFF1C, 0x006D), static_cast<game::DirType>(12)},
    {Coordinate::FromLeptons(0xFF22, 0x0063), static_cast<game::DirType>(16)},
    {Coordinate::FromLeptons(0xFF25, 0x005A), static_cast<game::DirType>(20)},
    {Coordinate::FromLeptons(0xFF2B, 0x0052), static_cast<game::DirType>(23)},
    {Coordinate::FromLeptons(0xFF32, 0x0048), static_cast<game::DirType>(27)},
    {Coordinate::FromLeptons(0xFF37, 0x0040), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFF3D, 0x0038), static_cast<game::DirType>(36)},
    {Coordinate::FromLeptons(0xFF46, 0x0030), static_cast<game::DirType>(39)},
    {Coordinate::FromLeptons(0xFF4F, 0x002B), static_cast<game::DirType>(43)},
    {Coordinate::FromLeptons(0xFF58, 0x0024), static_cast<game::DirType>(47)},
    {Coordinate::FromLeptons(0xFF60, 0x0020), static_cast<game::DirType>(51)},
    {Coordinate::FromLeptons(0xFF6D, 0x001B), static_cast<game::DirType>(54)},
    {Coordinate::FromLeptons(0xFF79, 0x0017), static_cast<game::DirType>(57)},
    {Coordinate::FromLeptons(0xFF82, 0x0014),
     static_cast<game::DirType>(60)},  // Track jump here.
    {Coordinate::FromLeptons(0xFF8F, 0x0011), static_cast<game::DirType>(62)},
    {Coordinate::FromLeptons(0xFF98, 0x000D), static_cast<game::DirType>(63)},
    {Coordinate::FromLeptons(0xFFA2, 0x0009), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFAC, 0x0006), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFB5, 0x0004), static_cast<game::DirType>(66)},
    {Coordinate::FromLeptons(0xFFC0, 0x0003), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFCB, 0x0002), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x0001), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFE0, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFEB, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate{}, static_cast<game::DirType>(64)}};

const DriveClass::TrackType DriveClass::Track5[] = {
    {Coordinate::FromLeptons(0xFE08, 0xFFF8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE10, 0xFFF0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE18, 0xFFE8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE20, 0xFFE0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE28, 0xFFD8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE30, 0xFFD0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE38, 0xFFC8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE40, 0xFFC0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE48, 0xFFB8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE50, 0xFFB0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE58, 0xFFA8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE60, 0xFFA0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE68, 0xFF98), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE70, 0xFF90), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE78, 0xFF88), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE80, 0xFF80),
     static_cast<game::DirType>(32)},  // Track entry here.
    {Coordinate::FromLeptons(0xFE88, 0xFF78), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE90, 0xFF71), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE97, 0xFF6A), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE9F, 0xFF62), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEA8, 0xFF5A), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEB0, 0xFF53), static_cast<game::DirType>(35)},
    {Coordinate::FromLeptons(0xFEB7, 0xFF4B), static_cast<game::DirType>(38)},
    {Coordinate::FromLeptons(0xFEBE, 0xFF44), static_cast<game::DirType>(41)},
    {Coordinate::FromLeptons(0xFEC4, 0xFF3E), static_cast<game::DirType>(44)},
    {Coordinate::FromLeptons(0xFECE, 0xFF39), static_cast<game::DirType>(47)},
    {Coordinate::FromLeptons(0xFED8, 0xFF34), static_cast<game::DirType>(50)},
    {Coordinate::FromLeptons(0xFEE0, 0xFF30), static_cast<game::DirType>(53)},
    {Coordinate::FromLeptons(0xFEEB, 0xFF2D), static_cast<game::DirType>(56)},
    {Coordinate::FromLeptons(0xFEF5, 0xFF2C), static_cast<game::DirType>(59)},
    {Coordinate::FromLeptons(0xFF00, 0xFF2B), static_cast<game::DirType>(62)},
    {Coordinate::FromLeptons(0xFF0B, 0xFF2C), static_cast<game::DirType>(66)},
    {Coordinate::FromLeptons(0xFF15, 0xFF2D), static_cast<game::DirType>(69)},
    {Coordinate::FromLeptons(0xFF1F, 0xFF30), static_cast<game::DirType>(72)},
    {Coordinate::FromLeptons(0xFF28, 0xFF34), static_cast<game::DirType>(75)},
    {Coordinate::FromLeptons(0xFF30, 0xFF39), static_cast<game::DirType>(78)},
    {Coordinate::FromLeptons(0xFF3A, 0xFF3E), static_cast<game::DirType>(81)},
    {Coordinate::FromLeptons(0xFF44, 0xFF44), static_cast<game::DirType>(84)},
    {Coordinate::FromLeptons(0xFF4B, 0xFF4B), static_cast<game::DirType>(87)},
    {Coordinate::FromLeptons(0xFF50, 0xFF53), static_cast<game::DirType>(90)},
    {Coordinate::FromLeptons(0xFF58, 0xFF5A), static_cast<game::DirType>(93)},
    {Coordinate::FromLeptons(0xFF60, 0xFF62), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF68, 0xFF6A), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF70, 0xFF71), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF78, 0xFF78), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF80, 0xFF80),
     static_cast<game::DirType>(96)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0xFF88), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF90, 0xFF90), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFF98, 0xFF98), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFA0, 0xFFA0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFA8, 0xFFA8), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFB0, 0xFFB0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFB8, 0xFFB8), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFC0, 0xFFC0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFC8, 0xFFC8), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFD0, 0xFFD0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFD8, 0xFFD8), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFE0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFE8, 0xFFE8), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFF0, 0xFFF0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFF8, 0xFFF8), static_cast<game::DirType>(96)},
    {Coordinate{}, static_cast<game::DirType>(96)}};

const DriveClass::TrackType DriveClass::Track6[] = {
    {Coordinate::FromLeptons(0xFE00, 0x0100), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE08, 0x00F8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE10, 0x00F0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE18, 0x00E8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE20, 0x00E0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE28, 0x00D8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE30, 0x00D0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE38, 0x00C8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE40, 0x00C0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE48, 0x00B8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE50, 0x00B0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE58, 0x00A8), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE60, 0x00A0), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE68, 0x0098), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE70, 0x0090), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE78, 0x0088), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE80, 0x0080),
     static_cast<game::DirType>(32)},  // Jump entry point here.
    {Coordinate::FromLeptons(0xFE88, 0x0078), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE90, 0x0070), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFE98, 0x0068), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEA0, 0x0060), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEA8, 0x0058), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEAE, 0x0055), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFEB8, 0x004E), static_cast<game::DirType>(35)},
    {Coordinate::FromLeptons(0xFEC0, 0x0048), static_cast<game::DirType>(37)},
    {Coordinate::FromLeptons(0xFEC9, 0x0042), static_cast<game::DirType>(40)},
    {Coordinate::FromLeptons(0xFED2, 0x003B), static_cast<game::DirType>(43)},
    {Coordinate::FromLeptons(0xFEDA, 0x0037), static_cast<game::DirType>(45)},
    {Coordinate::FromLeptons(0xFEE3, 0x0032), static_cast<game::DirType>(48)},
    {Coordinate::FromLeptons(0xFEEB, 0x002B), static_cast<game::DirType>(51)},
    {Coordinate::FromLeptons(0xFEF5, 0x0026), static_cast<game::DirType>(53)},
    {Coordinate::FromLeptons(0xFEFE, 0x0022), static_cast<game::DirType>(56)},
    {Coordinate::FromLeptons(0xFF08, 0x001C), static_cast<game::DirType>(59)},
    {Coordinate::FromLeptons(0xFF12, 0x0019), static_cast<game::DirType>(61)},
    {Coordinate::FromLeptons(0xFF1B, 0x0015), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF26, 0x0011), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF30, 0x000E), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF39, 0x000B), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF43, 0x0009), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF4E, 0x0007), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF57, 0x0005), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF62, 0x0003), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF6D, 0x0001), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF77, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF80, 0x0000),
     static_cast<game::DirType>(64)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF8B, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFF95, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFA0, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFAB, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFB5, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFC0, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFCB, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFE0, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFEB, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate{}, static_cast<game::DirType>(64)}};

const DriveClass::TrackType DriveClass::Track7[] = {
    {Coordinate::FromLeptons(0xFFFF, 0x0006), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0xFFFE, 0x000C), static_cast<game::DirType>(4)},
    {Coordinate::FromLeptons(0xFFFC, 0x0011), static_cast<game::DirType>(8)},
    {Coordinate::FromLeptons(0xFFFA, 0x0018), static_cast<game::DirType>(12)},
    {Coordinate::FromLeptons(0xFFF6, 0x001F), static_cast<game::DirType>(16)},
    {Coordinate::FromLeptons(0xFFF3, 0x0024), static_cast<game::DirType>(19)},
    {Coordinate::FromLeptons(0xFFF0, 0x002B), static_cast<game::DirType>(22)},
    {Coordinate::FromLeptons(0xFFFD, 0x0030), static_cast<game::DirType>(23)},
    {Coordinate::FromLeptons(0xFFEB, 0x0035), static_cast<game::DirType>(24)},
    {Coordinate::FromLeptons(0xFFE8, 0x0038), static_cast<game::DirType>(25)},
    {Coordinate::FromLeptons(0xFFE6, 0x003C), static_cast<game::DirType>(26)},
    {Coordinate::FromLeptons(0xFFE3, 0x0040), static_cast<game::DirType>(27)},
    {Coordinate::FromLeptons(0xFFE0, 0x0043), static_cast<game::DirType>(28)},
    {Coordinate::FromLeptons(0xFFDD, 0x0046), static_cast<game::DirType>(29)},
    {Coordinate::FromLeptons(0xFFDF, 0x0043), static_cast<game::DirType>(30)},
    {Coordinate::FromLeptons(0xFFE1, 0x0040), static_cast<game::DirType>(30)},
    {Coordinate::FromLeptons(0xFFE3, 0x003C), static_cast<game::DirType>(30)},
    {Coordinate::FromLeptons(0xFFE5, 0x0038), static_cast<game::DirType>(30)},
    {Coordinate::FromLeptons(0xFFE7, 0x0035), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFFE9, 0x0030), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFFEB, 0x002B), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFFED, 0x0024), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFFF1, 0x001F), static_cast<game::DirType>(31)},
    {Coordinate::FromLeptons(0xFFF4, 0x0018), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF7, 0x0011), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFFA, 0x000C), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFFD, 0x0006), static_cast<game::DirType>(32)},
    {Coordinate{}, static_cast<game::DirType>(32)}};

const DriveClass::TrackType DriveClass::Track8[] = {
    {Coordinate::FromLeptons(0xFFFC, 0x0003), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFF7, 0x0006), static_cast<game::DirType>(36)},
    {Coordinate::FromLeptons(0xFFF1, 0x000A), static_cast<game::DirType>(40)},
    {Coordinate::FromLeptons(0xFFEB, 0x000C), static_cast<game::DirType>(44)},
    {Coordinate::FromLeptons(0xFFE4, 0x000D), static_cast<game::DirType>(46)},
    {Coordinate::FromLeptons(0xFFDC, 0x000E), static_cast<game::DirType>(48)},
    {Coordinate::FromLeptons(0xFFD5, 0x000F), static_cast<game::DirType>(50)},
    {Coordinate::FromLeptons(0xFFD0, 0x0010), static_cast<game::DirType>(52)},
    {Coordinate::FromLeptons(0xFFC9, 0x0011), static_cast<game::DirType>(54)},
    {Coordinate::FromLeptons(0xFFC2, 0x0012), static_cast<game::DirType>(56)},
    {Coordinate::FromLeptons(0xFFC0, 0x0011), static_cast<game::DirType>(58)},
    {Coordinate::FromLeptons(0xFFC2, 0x0010), static_cast<game::DirType>(60)},
    {Coordinate::FromLeptons(0xFFC9, 0x000E), static_cast<game::DirType>(62)},
    {Coordinate::FromLeptons(0xFFCF, 0x000C), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x000A), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFDA, 0x0008), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFE2, 0x0006), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFE9, 0x0004), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFEF, 0x0002), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0001), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0xFFF9, 0x0000), static_cast<game::DirType>(64)},
    {Coordinate{}, static_cast<game::DirType>(64)}};

const DriveClass::TrackType DriveClass::Track9[] = {
    {Coordinate::FromLeptons(0x0002, 0xFFF5), static_cast<game::DirType>(0)},
    {Coordinate::FromLeptons(0x0004, 0xFFEB), static_cast<game::DirType>(2)},
    {Coordinate::FromLeptons(0x0006, 0xFFE0), static_cast<game::DirType>(4)},
    {Coordinate::FromLeptons(0x0009, 0xFFD5), static_cast<game::DirType>(6)},
    {Coordinate::FromLeptons(0x000C, 0xFFCE), static_cast<game::DirType>(9)},
    {Coordinate::FromLeptons(0x000F, 0xFFC8), static_cast<game::DirType>(11)},
    {Coordinate::FromLeptons(0x0012, 0xFFC0), static_cast<game::DirType>(13)},
    {Coordinate::FromLeptons(0x0015, 0xFFB8), static_cast<game::DirType>(16)},
    {Coordinate::FromLeptons(0x0012, 0xFFC0), static_cast<game::DirType>(18)},
    {Coordinate::FromLeptons(0x000E, 0xFFC8), static_cast<game::DirType>(20)},
    {Coordinate::FromLeptons(0x000A, 0xFFCE), static_cast<game::DirType>(22)},
    {Coordinate::FromLeptons(0x0004, 0xFFD5), static_cast<game::DirType>(24)},
    {Coordinate::FromLeptons(0x0000, 0xFFDE), static_cast<game::DirType>(26)},
    {Coordinate::FromLeptons(0xFFF8, 0xFFE9), static_cast<game::DirType>(28)},
    {Coordinate::FromLeptons(0xFFF2, 0xFFEE), static_cast<game::DirType>(30)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFF5), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0xFFE1, 0xFFFD), static_cast<game::DirType>(34)},
    {Coordinate::FromLeptons(0xFFD8, 0x0002), static_cast<game::DirType>(36)},
    {Coordinate::FromLeptons(0xFFD2, 0x0007), static_cast<game::DirType>(39)},
    {Coordinate::FromLeptons(0xFFCB, 0x000B), static_cast<game::DirType>(41)},
    {Coordinate::FromLeptons(0xFFC5, 0x0010), static_cast<game::DirType>(43)},
    {Coordinate::FromLeptons(0xFFBE, 0x0013), static_cast<game::DirType>(45)},
    {Coordinate::FromLeptons(0xFFB7, 0x0015), static_cast<game::DirType>(48)},
    {Coordinate::FromLeptons(0xFFBE, 0x0013), static_cast<game::DirType>(50)},
    {Coordinate::FromLeptons(0xFFC5, 0x0011), static_cast<game::DirType>(52)},
    {Coordinate::FromLeptons(0xFFCC, 0x000B), static_cast<game::DirType>(54)},
    {Coordinate::FromLeptons(0xFFD4, 0x0008), static_cast<game::DirType>(56)},
    {Coordinate::FromLeptons(0xFFDF, 0x0005), static_cast<game::DirType>(58)},
    {Coordinate::FromLeptons(0xFFEB, 0x0003), static_cast<game::DirType>(62)},
    {Coordinate::FromLeptons(0xFFF5, 0x0001), static_cast<game::DirType>(64)},
    {Coordinate{}, static_cast<game::DirType>(64)}};

const DriveClass::TrackType DriveClass::Track10[] = {
    {Coordinate::FromLeptons(0x000B, 0xFFF6), static_cast<game::DirType>(32)},
    {Coordinate::FromLeptons(0x0015, 0xFFF0), static_cast<game::DirType>(37)},
    {Coordinate::FromLeptons(0x0020, 0xFFEB), static_cast<game::DirType>(42)},
    {Coordinate::FromLeptons(0x002B, 0xFFE9), static_cast<game::DirType>(47)},
    {Coordinate::FromLeptons(0x0032, 0xFFE5), static_cast<game::DirType>(52)},
    {Coordinate::FromLeptons(0x0038, 0xFFE3), static_cast<game::DirType>(57)},
    {Coordinate::FromLeptons(0x0040, 0xFFE0), static_cast<game::DirType>(60)},
    {Coordinate::FromLeptons(0x0038, 0xFFE2), static_cast<game::DirType>(62)},
    {Coordinate::FromLeptons(0x0032, 0xFFE4), static_cast<game::DirType>(64)},
    {Coordinate::FromLeptons(0x002A, 0xFFE5), static_cast<game::DirType>(68)},
    {Coordinate::FromLeptons(0x001E, 0xFFE6), static_cast<game::DirType>(70)},
    {Coordinate::FromLeptons(0x0015, 0xFFE7), static_cast<game::DirType>(72)},
    {Coordinate::FromLeptons(0x000B, 0xFFE8), static_cast<game::DirType>(74)},
    {Coordinate::FromLeptons(0x0000, 0xFFE9), static_cast<game::DirType>(76)},
    {Coordinate::FromLeptons(0xFFF5, 0xFFE8), static_cast<game::DirType>(78)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFE7), static_cast<game::DirType>(80)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFE6), static_cast<game::DirType>(82)},
    {Coordinate::FromLeptons(0xFFD5, 0xFFE5), static_cast<game::DirType>(84)},
    {Coordinate::FromLeptons(0xFFCE, 0xFFE4), static_cast<game::DirType>(86)},
    {Coordinate::FromLeptons(0xFFC5, 0xFFE2), static_cast<game::DirType>(88)},
    {Coordinate::FromLeptons(0xFFC0, 0xFFE0), static_cast<game::DirType>(90)},
    {Coordinate::FromLeptons(0xFFC5, 0xFFE3), static_cast<game::DirType>(92)},
    {Coordinate::FromLeptons(0xFFCE, 0xFFE5), static_cast<game::DirType>(94)},
    {Coordinate::FromLeptons(0xFFD5, 0xFFE9), static_cast<game::DirType>(95)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFEB), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFF0), static_cast<game::DirType>(96)},
    {Coordinate::FromLeptons(0xFFF5, 0xFFF6), static_cast<game::DirType>(96)},
    {Coordinate{}, static_cast<game::DirType>(96)}};

const DriveClass::TrackType DriveClass::Track11[] = {
    {Coordinate::FromLeptons(0x0000, 0x0100), game::DIR_SW},
    {Coordinate::FromLeptons(0x0008, 0x00F3), game::DIR_SW},
    {Coordinate::FromLeptons(0x0010, 0x00E5), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0018, 0x00D6), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0020, 0x00C8), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0028, 0x00B9), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0030, 0x00AB), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0038, 0x009C), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0040, 0x008D), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0048, 0x007F), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0050, 0x0071), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0058, 0x0064), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0060, 0x0055), game::DIR_SW_X2},

    {Coordinate{}, game::DIR_SW_X2}};

const DriveClass::TrackType DriveClass::Track12[] = {
    {Coordinate::FromLeptons(0x0060, 0xFF55), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0058, 0xFF64), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0050, 0xFF71), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0048, 0xFF7F), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0040, 0xFF8D), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0038, 0xFF9C), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0030, 0xFFAB), game::DIR_SW_X2},
    {Coordinate::FromLeptons(0x0028, 0xFFB9), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0020, 0xFFC8), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0018, 0xFFD6), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0010, 0xFFE5), game::DIR_SW_X1},
    {Coordinate::FromLeptons(0x0008, 0xFFF3), game::DIR_SW},

    {Coordinate{}, game::DIR_SW}};

/*
**	Drive out of weapon's factory.
*/
const DriveClass::TrackType DriveClass::Track13[] = {
    {Pixel_Offset_Coord(10, -21),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(10, -21),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(10, -20),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(10, -20),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(9, -18),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(9, -18),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(9, -17),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(8, -16),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(8, -15),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(7, -14),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(7, -13),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(6, -12),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(6, -11),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(5, -10),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(5, -9),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(4, -8),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(4, -7),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(3, -6),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 10)},
    {Pixel_Offset_Coord(3, -5),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 9)},
    {Pixel_Offset_Coord(2, -4),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 7)},
    {Pixel_Offset_Coord(2, -3),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 5)},
    {Pixel_Offset_Coord(1, -2),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 3)},
    {Pixel_Offset_Coord(1, -1),
     static_cast<game::DirType>(static_cast<int>(game::DIR_SW) - 1)},

    {Coordinate{}, game::DIR_SW}};

/*
**	There are a limited basic number of tracks that a vehicle can follow.
*These *	are they. Each track can be interpreted differently but this is
*controlled *	by the TrackControl structure elaborated elsewhere.
*/
const DriveClass::RawTrackType DriveClass::RawTracks[13] = {
    {Track1, -1, 0, -1},  {Track2, -1, 0, -1},  {Track3, 37, 12, 22},
    {Track4, 26, 11, 19}, {Track5, 45, 15, 31}, {Track6, 44, 16, 27},
    {Track7, -1, 0, -1},  {Track8, -1, 0, -1},  {Track9, -1, 0, -1},
    {Track10, -1, 0, -1}, {Track11, -1, 0, -1}, {Track12, -1, 0, -1},
    {Track13, -1, 0, -1}};

/***************************************************************************
**	Smooth turning control table. Given two directions in a path list, this
**	table determines which track to use and what modifying operations need
**	be performed on the track data.
*/
const DriveClass::TurnTrackType DriveClass::TrackControl[67] = {
    {1, 0, game::DIR_N, F_},                         //	0-0
    {3, 7, game::DIR_NE, F_D},                       //	0-1 (raw chart)
    {4, 9, game::DIR_E, F_D},                        //	0-2 (raw chart)
    {0, 0, game::DIR_SE, F_},                        //	0-3 !
    {0, 0, game::DIR_S, F_},                         //	0-4 !
    {0, 0, game::DIR_SW, F_},                        //	0-5 !
    {4, 9, game::DIR_W, (F_X | F_D)},                //	0-6
    {3, 7, game::DIR_NW, (F_X | F_D)},               //	0-7
    {6, 8, game::DIR_N, (F_T | F_X | F_Y | F_D)},    //	1-0
    {2, 0, game::DIR_NE, F_},                        //	1-1 (raw chart)
    {6, 8, game::DIR_E, F_D},                        //	1-2 (raw chart)
    {5, 10, game::DIR_SE, F_D},                      //	1-3 (raw chart)
    {0, 0, game::DIR_S, F_},                         //	1-4 !
    {0, 0, game::DIR_SW, F_},                        //	1-5 !
    {0, 0, game::DIR_W, F_},                         //	1-6 !
    {5, 10, game::DIR_NW, (F_T | F_X | F_Y | F_D)},  //	1-7
    {4, 9, game::DIR_N, (F_T | F_X | F_Y | F_D)},    //	2-0
    {3, 7, game::DIR_NE, (F_T | F_X | F_Y | F_D)},   //	2-1
    {1, 0, game::DIR_E, (F_T | F_X)},                //	2-2
    {3, 7, game::DIR_SE, (F_T | F_X | F_D)},         //	2-3
    {4, 9, game::DIR_S, (F_T | F_X | F_D)},          //	2-4
    {0, 0, game::DIR_SW, F_},                        //	2-5 !
    {0, 0, game::DIR_W, F_},                         //	2-6 !
    {0, 0, game::DIR_NW, F_},                        //	2-7 !
    {0, 0, game::DIR_N, F_},                         //	3-0 !
    {5, 10, game::DIR_NE, (F_Y | F_D)},              //	3-1
    {6, 8, game::DIR_E, (F_Y | F_D)},                //	3-2
    {2, 0, game::DIR_SE, F_Y},                       //	3-3
    {6, 8, game::DIR_S, (F_T | F_X | F_D)},          //	3-4
    {5, 10, game::DIR_SW, (F_T | F_X | F_D)},        //	3-5
    {0, 0, game::DIR_W, F_},                         //	3-6 !
    {0, 0, game::DIR_NW, F_},                        //	3-7 !
    {0, 0, game::DIR_N, F_},                         //	4-0 !
    {0, 0, game::DIR_NE, F_},                        //	4-1 !
    {4, 9, game::DIR_E, (F_Y | F_D)},                //	4-2
    {3, 7, game::DIR_SE, (F_Y | F_D)},               //	4-3
    {1, 0, game::DIR_S, F_Y},                        //	4-4
    {3, 7, game::DIR_SW, (F_X | F_Y | F_D)},         //	4-5
    {4, 9, game::DIR_W, (F_X | F_Y | F_D)},          //	4-6
    {0, 0, game::DIR_NW, F_},                        //	4-7 !
    {0, 0, game::DIR_N, F_},                         //	5-0 !
    {0, 0, game::DIR_NE, F_},                        //	5-1 !
    {0, 0, game::DIR_E, F_},                         //	5-2 !
    {5, 10, game::DIR_SE, (F_T | F_D)},              //	5-3
    {6, 8, game::DIR_S, (F_T | F_D)},                //	5-4
    {2, 0, game::DIR_SW, F_T},                       //	5-5
    {6, 8, game::DIR_W, (F_X | F_Y | F_D)},          //	5-6
    {5, 10, game::DIR_NW, (F_X | F_Y | F_D)},        //	5-7
    {4, 9, game::DIR_N, (F_T | F_Y | F_D)},          //	6-0
    {0, 0, game::DIR_NE, F_},                        //	6-1 !
    {0, 0, game::DIR_E, F_},                         //	6-2 !
    {0, 0, game::DIR_SE, F_},                        //	6-3 !
    {4, 9, game::DIR_S, (F_T | F_D)},                //	6-4
    {3, 7, game::DIR_SW, (F_T | F_D)},               //	6-5
    {1, 0, game::DIR_W, F_T},                        //	6-6
    {3, 7, game::DIR_NW, (F_T | F_Y | F_D)},         //	6-7
    {6, 8, game::DIR_N, (F_T | F_Y | F_D)},          //	7-0
    {5, 10, game::DIR_NE, (F_T | F_Y | F_D)},        //	7-1
    {0, 0, game::DIR_E, F_},                         //	7-2 !
    {0, 0, game::DIR_SE, F_},                        //	7-3 !
    {0, 0, game::DIR_S, F_},                         //	7-4 !
    {5, 10, game::DIR_SW, (F_X | F_D)},              //	7-5
    {6, 8, game::DIR_W, (F_X | F_D)},                //	7-6
    {2, 0, game::DIR_NW, F_X},                       //	7-7

    {11, 11, game::DIR_SW, F_},     // Backup harvester into refinery.
    {12, 12, game::DIR_SW_X2, F_},  // Drive back into refinery.
    {13, 13, game::DIR_SW, F_}      // Drive out of weapons factory.
};

}  // namespace td
