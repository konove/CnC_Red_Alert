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

/* $Header: /CounterStrike/DRIVE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
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
 *                  Last Update : October 31, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * DriveClass::AI -- Processes unit movement and rotation. *
 *   DriveClass::Approach_Target -- Handles approaching the target in order to
 *   attack it. * DriveClass::Assign_Destination -- Set the unit's NavCom. *
 *   DriveClass::Class_Of -- Fetches a reference to the class type for this
 *   object. * DriveClass::Do_Turn -- Tries to turn the vehicle to the specified
 *   direction. * DriveClass::DriveClass -- Constructor for drive class object.
 * * DriveClass::Force_Track -- Forces the unit to use the indicated track. *
 *   DriveClass::Lay_Track -- Handles track laying logic for the unit. *
 *   DriveClass::Limbo -- Prepares vehicle and then limbos it. *
 *   DriveClass::Mark_Track -- Marks the midpoint of the track as occupied. *
 *   DriveClass::Ok_To_Move -- Checks to see if this object can begin moving. *
 *   DriveClass::Per_Cell_Process -- Handles when unit finishes movement into a
 *   cell. * DriveClass::Response_Attack -- Voice feedback when ordering the
 * unit to attack a target. * DriveClass::Response_Move -- Voice feedback when
 *   ordering the unit to move. * DriveClass::Response_Select -- Voice feedback
 *   when selecting the unit. * DriveClass::Scatter -- Causes the unit to travel
 *   to a nearby safe cell. * DriveClass::Smooth_Turn -- Handles the low level
 *   coord calc for smooth turn logic. * DriveClass::Start_Of_Move -- Tries to
 *   get a unit to advance toward cell. * DriveClass::Stop_Driver -- Handles
 *   removing occupation bits when driving stops. * DriveClass::Teleport_To --
 *   Teleport object to specified location. * DriveClass::While_Moving --
 *   Processes unit movement. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/objects/drive.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <span>

#include "absl/log/check.h"
#include "base/array.h"
#include "base/buffer.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "game/core/cell_size.h"
#include "game/sim/direction.h"
#include "game/sim/enums.h"
#include "game/sim/heading.h"
#include "game/sim/mph.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/team.h"
#include "ra/assets/audio.h"
#include "ra/core/coord.h"
#include "ra/core/defines.h"
#include "ra/core/random_pick.h"
#include "ra/core/target.h"
#include "ra/data/rules.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/objects/building.h"
#include "ra/objects/cell.h"
#include "ra/objects/foot.h"
#include "ra/objects/house.h"
#include "ra/objects/mission.h"
#include "ra/objects/object.h"
#include "ra/objects/techno.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/startup/game_state.h"

namespace ra {

/***********************************************************************************************
 * DriveClass::Response_Select -- Voice feedback when selecting the unit. *
 *                                                                                             *
 *    This is the voice to play when the unit is selected. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/30/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Response_Select() {
  DCHECK(IsActive);

  static const VocType _response[] = {VOC_VEHIC,  VOC_REPORT, VOC_YESSIR,
                                      VOC_YESSIR, VOC_YESSIR, VOC_AWAIT};
  const VocType response =
      base::At(_response, LocalRandomPick<int>(0, std::ssize(_response) - 1));
  if (TheGameState().allow_voice()) {
    PlaySoundEffect(response, Fixed(1), -(ID + 1));
  }
}

/***********************************************************************************************
 * DriveClass::Response_Move -- Voice feedback when ordering the unit to move. *
 *                                                                                             *
 *    This plays the audio feedback when ordering this unit to move to a new
 *destination.      *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/30/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Response_Move() {
  DCHECK(IsActive);

  static const VocType _response[] = {
      VOC_ACKNOWL,
      VOC_AFFIRM,
  };
  const VocType response =
      base::At(_response, LocalRandomPick<int>(0, std::ssize(_response) - 1));
  if (TheGameState().allow_voice()) {
    PlaySoundEffect(response, Fixed(1), -(ID + 1));
  }
}

/***********************************************************************************************
 * DriveClass::Response_Attack -- Voice feedback when ordering the unit to
 *attack a target.    *
 *                                                                                             *
 *    This plays the audio feedback when ordering this unit to attack. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/30/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Response_Attack() {
  DCHECK(IsActive);

  static const VocType _response[] = {VOC_AFFIRM, VOC_ACKNOWL};
  const VocType response =
      base::At(_response, LocalRandomPick<int>(0, std::ssize(_response) - 1));
  if (TheGameState().allow_voice()) {
    PlaySoundEffect(response, Fixed(1), -(ID + 1));
  }
}

/***********************************************************************************************
 * DriveClass::Scatter -- Causes the unit to travel to a nearby safe cell. *
 *                                                                                             *
 *    This routine is called when the unit discovers that it should get out of
 *the "hot seat"  * and move to an adjacent cell. Since the safety of the
 *adjacent cell is not determined    * before the move begins, it will appear
 *that the unit is just scattering (which it        * should). *
 *                                                                                             *
 * INPUT:   threat   -- The coordinate of the source of the threat. The unit
 *will try to move  * roughly away from the threat. *
 *                                                                                             *
 *          forced   -- The threat is real and a serious effort to scatter
 *should be made.     *
 *                                                                                             *
 *          nokidding-- The scatter should affect the player's infantry even if
 *it otherwise   * wouldn't have. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 09/25/1994 JLB : Created. * 09/27/1995 JLB : Revised to never
 *scatter if already moving.                              * 07/09/1996 JLB :
 *Moved to DriveClass so that ships will scatter too.                      *
 *   08/02/1996 JLB : Added the "nokidding" parameter. *
 *=============================================================================================*/
void DriveClass::Scatter(Coordinate threat, bool forced, bool nokidding) {
  DCHECK(IsActive);

  /*
  **	Certain missions prevent scattering regardless of whether it would be
  **	a good idea or not.
  */
  if (TheRules().mission_control().at(mission_state().current()).IsParalyzed) {
    return;
  }

  if (((What_Am_I() != RTTI_UNIT ||
        !dynamic_cast<UnitClass*>(this)->IsDumping) &&
       (!NavCom || (nokidding && !IsRotating))) &&
      (!TarCom || forced || RandomPick(1, 4) == 1)) {
    game::Facing toface = game::Facing::kNone;

    if (threat) {
      toface = game::NearestFacing(Direction8(threat, Coord));
      toface = toface + static_cast<game::Facing>(RandomPick(0, 2) - 1);
    } else {
      toface = game::NearestFacing(PrimaryFacing.current());
      toface = toface + static_cast<game::Facing>(RandomPick(0, 2) - 1);
    }

    for (const game::Facing face : magic_enum::enum_values<game::Facing>()) {
      const game::Facing newface = toface + face;
      const Cell newcell = AdjacentCell(Cell::Containing(Coord), newface);

      if (TheMap().In_Radar(newcell) &&
          Can_Enter_Cell(newcell) == game::MOVE_OK) {
        Assign_Destination(Target::FromCell(newcell));
      }
    }
  }
}

/***********************************************************************************************
 * DriveClass::Limbo -- Prepares vehicle and then limbos it. *
 *                                                                                             *
 *    This routine removes the occupation bits for the vehicle and also handles
 *cleaning up    * any vehicle reservation bits. After this, it then proceeds
 *with limboing the unit.       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the vehicle limboed? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
bool DriveClass::Limbo() {
  if (!IsInLimbo) {
    Stop_Driver();
    TrackNumber = -1;
  }
  return FootClass::Limbo();
}

/***********************************************************************************************
 * DriveClass::Stop_Driver -- Handles removing occupation bits when driving
 *stops.             *
 *                                                                                             *
 *    This routine will remove the "reservation" flag (if present) when the
 *vehicle is         * required to stop movement. *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  bool; Was the vehicle stopped? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 12/22/1994 JLB : Created. *
 *=============================================================================================*/
bool DriveClass::Stop_Driver() {
  DCHECK(IsActive);

  /*
  ** We only need to do something if the vehicle is actually going
  ** somewhere.
  */
  if (Head_To_Coord()) {
    /*
    ** Safe off whether the vehicle is down or not so we know whether
    ** we have to put it back down.
    */
    const int temp = IsDown;

    /*
    ** If the vehicle is down, pick it up so it doesn't interfere with
    ** our flags.
    */
    if (temp) {
      Mark(MARK_UP);
    }

    /*
    ** Call the drive class function which will let us release the
    ** reserved track.
    */
    Mark_Track(Head_To_Coord(), MARK_UP);

    /*
    ** If it was down it should be down when we are done.
    */
    if (temp) {
      Mark(MARK_DOWN);
    }
  }
  return FootClass::Stop_Driver();
}

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
void DriveClass::Do_Turn(game::Direction dir) {
  DCHECK(IsActive);

  if (dir != PrimaryFacing.current()) {
    PrimaryFacing.set_desired(dir);
  }
}

/***********************************************************************************************
 * DriveClass::Teleport_To -- Teleport object to specified location. *
 *                                                                                             *
 *    This will teleport the object to the specified location or as close as
 *possible to it    * if the destination is blocked. *
 *                                                                                             *
 * INPUT:   cell  -- The desired destination cell to teleport to. *
 *                                                                                             *
 * OUTPUT:  bool; Was the teleport successful? *
 *                                                                                             *
 * WARNINGS:   All current activity of this object will be terminated by the
 *teleport. It will * arrive at the destination in static guard mode. *
 *                                                                                             *
 * HISTORY: * 10/21/1996 JLB : Created. * 10/31/1996 JLB : Handles flag teleport
 *case.                                              *
 *=============================================================================================*/
bool DriveClass::Teleport_To(Cell cell) {
  /*
  **	All cargo gets destroyed.
  */
  if (TheRules().IsChronoKill) {
    Kill_Cargo(nullptr);
  }

  Stop_Driver();
  Force_Track(-1, Coordinate{});
  PrimaryFacing.set_current(PrimaryFacing.desired());
  Transmit_Message(RADIO_OVER_OUT);
  Assign_Destination(Target{});
  Assign_Target(Target{});
  Assign_Mission(MISSION_NONE);
  Commence();
  Mark(MARK_UP);

  /*
  **	A teleported unit will drop the flag right where it's at.
  */
  if (What_Am_I() == RTTI_UNIT &&
      dynamic_cast<UnitClass*>(this)->Flagged != HOUSE_NONE) {
    HouseClass::As_Pointer(dynamic_cast<UnitClass*>(this)->Flagged)
        ->Flag_Attach(Cell::Containing(Coord));
  }

  if (Can_Enter_Cell(cell) != game::MOVE_OK) {
    cell = TheMap().Nearby_Location(cell, Techno_Type_Class()->Speed);
  }
  Coord = cell.Center();
  Mark(MARK_DOWN);
  return true;
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
  DCHECK(IsActive);

  TrackNumber = track;
  TrackIndex = 0;
  if (coord) {
    Start_Driver(coord);
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
DriveClass::DriveClass(RTTIType rtti, int id, HousesType house)
    : FootClass(rtti, id, house) {}

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
Coordinate DriveClass::Smooth_Turn(Coordinate adj, game::Direction& dir) {
  DCHECK(IsActive);

  game::Direction workdir = dir;
  const TrackControlType flags = base::At(TrackControl, TrackNumber).Flag;

  int x = adj.x();
  int y = adj.y();

  if (base::Any(flags & F_T)) {
    const int temp = x;
    x = y;
    y = temp;
    workdir = game::Direction::kWest - workdir;
  }

  if (base::Any(flags & F_X)) {
    x = -x;
    workdir = game::Direction::kNorth - workdir;
  }

  if (base::Any(flags & F_Y)) {
    y = -y;
    workdir = game::Direction::kSouth - workdir;
  }

  dir = workdir;

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
  DCHECK(IsActive);

  /*
  **	Abort early if there is anything wrong with the parameters
  **	or the unit already is assigned the specified destination.
  */
  if (target == NavCom) {
    return;
  }

  /*
  **	For harvesting type vehicles, it might go into a dock and unload
  *procedure *	when the harvester is full and an empty refinery is selected as
  *a target.
  */
  BuildingClass* b = target.AsBuilding();

  /*
  **	If the player clicked on refinery but it is not busy, then assign
  **	it to unload at the refinery.
  */
  if (b != nullptr && *b == STRUCT_REFINERY && What_Am_I() == RTTI_UNIT &&
      dynamic_cast<const UnitTypeClass&>(*Techno_Type_Class()).IsToHarvest) {
    if (Contact_With_Whom() != b && !b->radio().InContact()) {
      /*
      **	Establish radio contact protocol. If the facility responds
      *correctly, *	then remain in radio contact and proceed toward the
      *desired destination.
      */
      if (Transmit_Message(RADIO_HELLO, b) == RADIO_ROGER) {
        if (mission_state().current() != MISSION_ENTER &&
            mission_state().current() != MISSION_HARVEST) {
          Assign_Mission(MISSION_ENTER);
          target = Target{};
        } else {
          //					target = Target{};
        }
      } else {
        //				target = Target{};
      }
    } else {
      //			target = Target{};
    }
  }

  /*
  **	Set the unit's navigation computer.
  */
  FootClass::Assign_Destination(target);

  base::At(Path, 0) = game::Facing::kNone;  // Force recalculation of path.
  if (!IsDriving && mission_state().current() != MISSION_UNLOAD) {
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
  DCHECK(IsActive);

  /*
  **	Perform quick legality checks.
  */
  if (!IsDriving || TrackNumber == -1 ||
      (IsRotating && !Techno_Type_Class()->IsTurretEquipped)) {
    SpeedAccum =
        0;  // Kludge?  No speed should accumulate if movement is on hold.
    return false;
  }

  /*
  **	If enough movement has accumulated so that the unit can
  **	visibly move on the map, then process accordingly.
  ** Slow the unit down if he's carrying a flag.
  */
  int maxspeed = std::min(
      Techno_Type_Class()->MaxSpeed * SpeedBias * House->GroundspeedBias,
      game::kMphLightSpeed);
  if (IsFormationMove) {
    maxspeed = FormationMaxSpeed;
  }

  int actual = 0;  // Working movement addition value.
  if (What_Am_I() == RTTI_UNIT &&
      dynamic_cast<UnitClass*>(this)->Flagged != HOUSE_NONE) {
    actual = SpeedAccum + maxspeed / 2 * Fixed(Speed, 256);
  } else {
    actual = SpeedAccum + maxspeed * Fixed(Speed, 256);
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
    std::span<const TrackType> ptr =
        base::At(RawTracks, tracknum - 1)
            .Track;  // Pointer to coord offset values.
    const game::Facing nextface =
        base::At(Path, 0);  // Next facing queued in path.

    /*
    **	Determine if there is a turn coming up. If there is
    **	a turn, then track jumping might occur.
    */
    bool adj = false;  // Is a turn coming up?
    if (nextface != game::Facing::kNone &&
        game::NearestFacing(track->Facing) != nextface) {
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
        game::Direction dir = base::At(ptr, base::ToSize(TrackIndex)).Facing;
        Coord = Smooth_Turn(offset, dir);

        PrimaryFacing.SnapTo(dir);

        /*
        **	See if "per cell" processing is necessary.
        */
        if (TrackIndex &&
            base::At(RawTracks, tracknum - 1).Cell == TrackIndex) {
          Mark(MARK_DOWN);
          if (Per_Cell_Process(PCP_DURING)) {
            return false;
          }
          Mark(MARK_UP);
        }

        /*
        **	The unit could "jump tracks". Check to see if the unit should
        **	do so.
        */
        if (/**this != UNIT_GUNBOAT &&*/ nextface != game::Facing::kNone &&
            adj && base::At(RawTracks, tracknum - 1).Jump == TrackIndex &&
            TrackIndex) {
          const int tnum =
              (static_cast<int>(game::NearestFacing(track->Facing)) *
               static_cast<int>(magic_enum::enum_count<game::Facing>())) +
              static_cast<int>(nextface);
          const TurnTrackType* newtrack =
              base::Suffix(TrackControl, tnum)
                  .data();  // Proposed jump-to track.
          if (newtrack->Track &&
              base::At(RawTracks, newtrack->Track - 1).Entry) {
            Coordinate c = Head_To_Coord();
            const int oldspeed = Speed;

            c = AdjacentCell(c, nextface);

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
                IsDriving = true;
                if (Per_Cell_Process(PCP_END)) {
                  return false;
                }
                IsDriving = false;
                if (Start_Driver(c)) {
                  Set_Speed(oldspeed);
                  base::MoveBytes(std::as_writable_bytes(base::Suffix(Path, 0)),
                                  std::as_bytes(base::Suffix(Path, 1)),
                                  kConquerPathMax - 1);
                  base::At(Path, kConquerPathMax - 1) = game::Facing::kNone;
                } else {
                  base::At(Path, 0) = game::Facing::kNone;
                  TrackNumber = -1;
                  actual = 0;
                }
                break;

              case game::MOVE_CLOAK:
                TheMap().at(c).Shimmer();
                break;

              case game::MOVE_TEMP:
                if (!House->IsHuman) {
                  TheMap().at(c).Incoming(Coordinate{}, true, true);
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
        Mark(MARK_DOWN);
        if (Per_Cell_Process(PCP_END)) {
          return false;
        }
        Mark(MARK_UP);

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
  SpeedAccum = actual;
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
 * INPUT:   why   -- Specifies the circumstances under which this routine was
 *called.          *
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
bool DriveClass::Per_Cell_Process(PCPType why) {
  DCHECK(IsActive);

  if (why == PCP_END) {
    const Cell cell = Cell::Containing(Coord);

    /*
    **	Check to see if it has reached its destination. If so, then clear the
    *NavCom *	regardless of the remaining path list.
    */
    if (NavCom.AsCell() == cell) {
      IsTurretLockedDown = false;
      NavCom = Target{};
      base::At(Path, 0) = game::Facing::kNone;
    }

    Lay_Track();
  }

  return FootClass::Per_Cell_Process(why);
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
 *repeatedly until HeadTo is not nullptr.               * 03/16/1994 JLB :
 * Revamped for track logic.                                                *
 * 04/15/1994 JLB : Converted to member function. * 06/19/1995 JLB : Fixed so
 * that it won't fire on ground unnecessarily.                     * 07/13/1995
 * JLB : Handles bumping into cloaked objects.   * 09/22/1995 JLB : Breaks out
 * of hopeless hunt mode. * 07/10/1996 JLB : Sets scan limit if necessary.
 *                                      *
 *=============================================================================================*/
bool DriveClass::Start_Of_Move() {
  DCHECK(IsActive);

  game::Facing facing = base::At(Path, 0);  // Direction movement will commence.

  if (!NavCom && facing == game::Facing::kNone) {
    IsTurretLockedDown = false;
    Stop_Driver();
    if (mission_state().current() == MISSION_MOVE) {
      Enter_Idle_Mode();
    }
    return false;  // Why is it calling this routine!?!
  }

  /*
  **	Reduce the path length if the target is a unit and the
  **	range to the unit is less than the precalculated path steps.
  */
  if (facing != game::Facing::kNone &&
      (NavCom.kind() == RTTI_VESSEL || NavCom.kind() == RTTI_UNIT ||
       NavCom.kind() == RTTI_INFANTRY)) {
    const int dist = Lepton_To_Cell(Distance(NavCom));
    if (dist < std::ssize(Path)) {
      base::At(Path, dist) = game::Facing::kNone;
      facing = base::At(Path, 0);  // Maybe needed.
    }
  }

  /*
  **	If the path is invalid at this point, then generate one. If
  **	generating a new path fails, then abort NavCom.
  */
  if (facing == game::Facing::kNone) {
    /*
    **	If after a path search, there is still no valid path, then set the
    **	NavCom to null and let the script take care of assigning a new
    **	navigation target.
    */
    if (PathDelay.HasTimeLeft()) {
      return false;
    }

    if (!Basic_Path()) {
      /*
      **	If the unit is close enough to the target then just stop
      **	driving now. This prevents the fidgeting that would occur
      **	if they mindlessly kept trying to get to the exact location
      **	desired. This is quite necessary since it is typical to move
      **	several units with the same mouse click.
      */
      if (!Is_On_Priority_Mission() &&
          Distance(NavCom) < TheRules().CloseEnoughDistance &&
          (mission_state().current() == MISSION_MOVE ||
           mission_state().current() == MISSION_GUARD_AREA)) {
        Assign_Destination(Target{});
        if (!IsActive) {
          return false;
        }
      } else {
        /*
        **	If a basic path could not be found, but the immediate move
        *destination is *	blocked by a friendly temporary blockage, then
        *cause that blockage *	to scatter.
        */
        const Cell cell = AdjacentCell(Cell::Containing(Center_Coord()),
                                       PrimaryFacing.current());
        if (TheMap().In_Radar(cell)) {
          const game::MoveType ok = Can_Enter_Cell(cell);
          if (ok == game::MOVE_TEMP) {
            CellClass* cellptr = &TheMap().at(cell);
            const TechnoClass* blockage = cellptr->Cell_Techno();
            if (blockage && House->Is_Ally(blockage)) {
              /*
              **	If the target can be told to get out of the way, only
              *bother *	to do so if we aren't very close to the target and this
              **	object can just say "good enough" and stop here.
              */
              if (Distance(NavCom) < TheRules().CloseEnoughDistance &&
                  !radio().InContact()) {
                Assign_Destination(Target{});
                return false;
              }
              cellptr->Incoming(Coordinate{}, true, false);
            }
          }
        }

        if (TryTryAgain > 0) {
          TryTryAgain--;
        } else {
          Assign_Destination(Target{});
          if (!IsActive) {
            return false;
          }
          if (IsNewNavCom) {
            PlaySoundEffect(VOC_SCOLD);
          }
          IsNewNavCom = false;
        }
      }

      /*
      **	Since the path was blocked, check to make sure that it was
      *completely *	blocked. If so and it has a valid TarCom and it is out
      *of range of the *	TarCom, then give this unit a range limit so
      *that it might not pick *	a "can't reach" target again.
      */
      if (!NavCom && TarCom && !In_Range(TarCom)) {
        IsScanLimited = true;
        if (Team != nullptr) {
          Team->Scan_Limit();
        }
        Assign_Target(Target{});
      }

      /*
      **	Stop the movement, for now, and let the subsequent logic in
      *later game *	frames resume movement as appropriate.
      */
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
        AdjacentCell(Cell::Containing(Center_Coord()), base::At(Path, 0));
    if (TheMap().In_Radar(cell)) {
      const game::MoveType ok = Can_Enter_Cell(cell);
      if (ok == game::MOVE_TEMP) {
        CellClass* cellptr = &TheMap().at(cell);
        const TechnoClass* blockage = cellptr->Cell_Techno();
        if (blockage && House->Is_Ally(blockage)) {
          /*
          **	If the target can be told to get out of the way, only bother
          **	to do so if we aren't very close to the target and this
          **	object can just say "good enough" and stop here.
          */
          if (Distance(NavCom) < TheRules().CloseEnoughDistance &&
              !radio().InContact()) {
            Assign_Destination(Target{});
            return false;
          }
          cellptr->Incoming(Coordinate{}, true, false);
        }
      }
    }

    TryTryAgain = kPathRetry;
    facing = base::At(Path, 0);
  }

  /*
  **	Determine the coordinate of the next cell to move into.
  */
  Coordinate dest = AdjacentCell(Coord, facing);  // Destination coordinate.
  const game::Direction dir =
      game::DirectionOf(facing);  // Desired actual facing toward destination.

  /*
  **	Set the facing correctly if it isn't already correct. This
  **	means starting a rotation track if necessary.
  */
  if (PrimaryFacing.current() != dir) {
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
  Cell destcell = Cell::Containing(dest);  // Cell of destination.
  Mark(MARK_UP);
  game::MoveType cando = Can_Enter_Cell(destcell, facing);
  Mark(MARK_DOWN);

  if (cando != game::MOVE_OK) {
    if (mission_state().current() == MISSION_MOVE /*KO&& House->IsHuman */ &&
        Distance(NavCom) < TheRules().CloseEnoughDistance) {
      Assign_Destination(Target{});
      if (!IsActive) {
        return false;  // BG
      }
    }

    /*
    **	If a temporary friendly object is blocking the path, then cause
    *it to *	get out of the way.
    */
    if (cando == game::MOVE_TEMP) {
      TheMap().at(destcell).Incoming(Coordinate{}, true, true);
    }

    /*
    **	If a cloaked object is blocking, then shimmer the cell.
    */
    if (cando == game::MOVE_CLOAK) {
      TheMap().at(destcell).Shimmer();
    }

    Stop_Driver();
    if (cando != game::MOVE_MOVING_BLOCK) {
      base::At(Path, 0) = game::Facing::kNone;  // Path is blocked!
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
          Override_Mission(MISSION_ATTACK, Target::FromCell(destcell),
                           Target{});
        }
      }
    } else {
      if (IsNewNavCom) {
        PlaySoundEffect(VOC_SCOLD);
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
  int speed =
      TheRules().ground().at(ground).Cost.at(Techno_Type_Class()->Speed) *
      256;  // Speed of unit.

  /* change speed if it's related to a team move */
  if (IsFormationMove) {
    speed = TheRules().ground().at(ground).Cost.at(FormationSpeed) * 256;
  }
  if (!speed) {
    speed = 128;
  }

  /*
  **	A damaged unit has a reduced speed.
  */
  if (Health_Ratio() <= TheRules().ConditionYellow /*(Techno_Type_Class()->MaxStrength>>1) > Strength*/) {
    speed -= speed / 4;  // Three quarters speed.
  }
  if (speed != Speed /* || !SpeedAdd*/) {
    Set_Speed(speed);  // Full speed.
  }

  /*
  **	Reserve the destination cell so that it won't become
  **	occupied AS this unit is moving into it.
  */
  if (cando != game::MOVE_OK) {
    base::At(Path, 0) = game::Facing::kNone;  // Path is blocked!
    TrackNumber = -1;
    dest = Coordinate{};
  } else {
    Overrun_Square(Cell::Containing(dest), true);

    /*
    **	Determine which track to use (based on recorded path).
    */
    game::Facing nextface = base::At(Path, 1);
    if (nextface == game::Facing::kNone) {
      nextface = facing;
    }

    IsOnShortTrack = false;
    TrackNumber = (static_cast<int>(facing) *
                   static_cast<int>(magic_enum::enum_count<game::Facing>())) +
                  static_cast<int>(nextface);
    if (base::At(TrackControl, TrackNumber).Track == 0) {
      base::At(Path, 0) = game::Facing::kNone;
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
        if (!IsActive) {
          return false;
        }
      } else {
        if (!IsActive) {
          return false;
        }
        dest = AdjacentCell(dest, nextface);
        destcell = Cell::Containing(dest);
        cando = Can_Enter_Cell(destcell);
      }
      if (!IsActive) {
        return false;
      }

      if (cando != game::MOVE_OK) {
        /*
        **	If a temporary friendly object is blocking the path, then cause
        *it to *	get out of the way.
        */
        if (cando == game::MOVE_TEMP) {
          TheMap().at(destcell).Incoming(Coordinate{}, true, true);
        }

        /*
        **	If a cloaked object is blocking, then shimmer the cell.
        */
        if (cando == game::MOVE_CLOAK) {
          TheMap().at(destcell).Shimmer();
        }

        base::At(Path, 0) = game::Facing::kNone;  // Path is blocked!
        TrackNumber = -1;
        dest = Coordinate{};
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
              Override_Mission(MISSION_ATTACK, Target::FromCell(destcell),
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
        base::At(Path, kConquerPathMax - 2) = game::Facing::kNone;
        IsPlanningToLook = true;
      }
    } else {
      base::MoveBytes(std::as_writable_bytes(base::Suffix(Path, 0)),
                      std::as_bytes(base::Suffix(Path, 1)),
                      kConquerPathMax - 1);
    }
    base::At(Path, kConquerPathMax - 1) = game::Facing::kNone;
  }

  IsNewNavCom = false;
  TrackIndex = 0;
  if (!Start_Driver(dest)) {
    TrackNumber = -1;
    base::At(Path, 0) = game::Facing::kNone;
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
  DCHECK(IsActive);

  FootClass::AI();
  if (!IsActive || Height > 0) {
    return;
  }

  /*
  ** Is this a unit that's been teleported using the chronosphere, and if so,
  ** has his timer expired such that he needs to teleport back?
  */
  if (IsMoebius &&
      (What_Am_I() != RTTI_UNIT ||
       dynamic_cast<UnitClass*>(this)->Class->Type != UNIT_CHRONOTANK) &&
      MoebiusCountDown.IsFinished()) {
    IsMoebius = false;
    Teleport_To(MoebiusCell);
    MoebiusCell = Cell{};
  }

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
        (NavCom || base::At(Path, 0) != game::Facing::kNone) &&
        (What_Am_I() != RTTI_UNIT ||
         !dynamic_cast<UnitClass*>(this)->IsDumping)) {
      Start_Of_Move();
      if (!IsActive) {
        return;
      }
      While_Moving();
      if (!IsActive) {
        return;
      }
    }

  } else {
    /*
    **	For tracked units that are rotating in place, perform the rotation now.
    */
    if (PrimaryFacing.IsRotating()) {
      Mark(MARK_CHANGE_REDRAW);
      if (PrimaryFacing.Turn(Techno_Type_Class()->ROT *
                             House->GroundspeedBias)) {
        Mark(MARK_CHANGE_REDRAW);
      }
      if (!IsRotating && Per_Cell_Process(PCP_ROTATION)) {
        return;
      }

    } else {
      /*
      **	The unit has no track to follow, but if there
      **	is a navigation target or a remaining path,
      **	then start on a new track.
      */
      if ((mission_state().current() != MISSION_GUARD || NavCom) &&
          mission_state().current() != MISSION_UNLOAD) {
        if (NavCom || base::At(Path, 0) != game::Facing::kNone) {
          /*
          **	Double check to make sure that the movement destination is
          **	in a zone that this unit can travel to. If not, then abort
          **	the navigation target.
          */
          if (IsLocked && mission_state().current() != MISSION_ENTER &&
              NavCom && !Is_In_Same_Zone(NavCom.AsCell())) {
            Stop_Driver();
            Assign_Destination(Target{});
          } else {
            Start_Of_Move();
            if (!IsActive) {
              return;
            }
            While_Moving();
            if (!IsActive) {
              return;
            }
          }
        } else {
          Stop_Driver();
        }
      }
    }
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
void DriveClass::Lay_Track() { DCHECK(IsActive); }

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
  DCHECK(IsActive);

  const bool value = type != MARK_UP;

  if (headto) {
    if (!IsOnShortTrack && TrackNumber != -1) {
      /*
      ** If we have not passed the per cell process point we need
      ** to deal with it.
      */
      const int tracknum = base::At(TrackControl, TrackNumber).Track;
      if (tracknum) {
        const std::span<const TrackType> ptr =
            base::At(RawTracks, tracknum - 1).Track;
        const int cellidx = base::At(RawTracks, tracknum - 1).Cell;
        if (cellidx > -1) {
          game::Direction dir = base::At(ptr, base::ToSize(cellidx)).Facing;

          if (TrackIndex < cellidx && cellidx != -1) {
            const Coordinate offset =
                Smooth_Turn(base::At(ptr, base::ToSize(cellidx)).Offset, dir);
            TheMap().at(offset).Flag.Occupy.Vehicle = value;
          }
        }
      }
    }
    TheMap().at(headto).Flag.Occupy.Vehicle = value;
  }
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
bool DriveClass::Ok_To_Move(game::Direction /*unused*/) {
  DCHECK(IsActive);

  return true;
}

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
    {Coordinate::FromLeptons(0x0000, 0x00F5), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00EA), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00DF), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00D4), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00C9), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00BE), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00B3), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x00A8), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x009D), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0092), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0087), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x007C),
     game::Direction(0)},  // Track jump check here.
    {Coordinate::FromLeptons(0x0000, 0x0071), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0066), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x005B), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0050), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0045), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x003A), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x002F), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0024), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0019), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x000E), game::Direction(0)},
    {Coordinate::FromLeptons(0x0000, 0x0003), game::Direction(0)},
    {Coordinate{}, game::Direction(0)}};

const DriveClass::TrackType DriveClass::Track2[] = {
    {Coordinate::FromLeptons(0xFF08, 0x00F8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF10, 0x00F0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF18, 0x00E8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF20, 0x00E0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF28, 0x00D8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF30, 0x00D0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF38, 0x00C8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF40, 0x00C0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF48, 0x00B8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF50, 0x00B0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF58, 0x00A8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF60, 0x00A0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF68, 0x0098), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF70, 0x0090), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF78, 0x0088), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF80, 0x0080),
     game::Direction(32)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0x0078), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF90, 0x0070), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF98, 0x0068), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFA0, 0x0060), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFA8, 0x0058), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFB0, 0x0050), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFB8, 0x0048), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFC0, 0x0040), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFC8, 0x0038), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFD0, 0x0030), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFD8, 0x0028), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFE0, 0x0020), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFE8, 0x0018), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF0, 0x0010), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF8, 0x0008), game::Direction(32)},
    {Coordinate{}, game::Direction(32)}};

const DriveClass::TrackType DriveClass::Track3[] = {
    {Coordinate::FromLeptons(0xFF00, 0x01F5), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01EA), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01DF), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01D4), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01C9), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01BE), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01B3), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x01A8), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x019D), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0192), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0187), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0180), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x0175),
     game::Direction(0)},  // Jump entry point here.
    {Coordinate::FromLeptons(0xFF00, 0x016B), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF02, 0x0160), game::Direction(1)},
    {Coordinate::FromLeptons(0xFF04, 0x0155), game::Direction(3)},
    {Coordinate::FromLeptons(0xFF06, 0x014C), game::Direction(4)},
    {Coordinate::FromLeptons(0xFF08, 0x0141), game::Direction(5)},
    {Coordinate::FromLeptons(0xFF0B, 0x0137), game::Direction(7)},
    {Coordinate::FromLeptons(0xFF0F, 0x012E), game::Direction(8)},
    {Coordinate::FromLeptons(0xFF13, 0x0124), game::Direction(9)},
    {Coordinate::FromLeptons(0xFF17, 0x011A), game::Direction(11)},
    {Coordinate::FromLeptons(0xFF1B, 0x0110), game::Direction(12)},
    {Coordinate::FromLeptons(0xFF1F, 0x0107),
     game::Direction(13)},  // Center cell processing here.
    {Coordinate::FromLeptons(0xFF24, 0x00FC), game::Direction(15)},
    {Coordinate::FromLeptons(0xFF28, 0x00F3), game::Direction(16)},
    {Coordinate::FromLeptons(0xFF2C, 0x00EC), game::Direction(17)},
    {Coordinate::FromLeptons(0xFF32, 0x00E0), game::Direction(19)},
    {Coordinate::FromLeptons(0xFF36, 0x00D7), game::Direction(20)},
    {Coordinate::FromLeptons(0xFF3D, 0x00CF), game::Direction(21)},
    {Coordinate::FromLeptons(0xFF42, 0x00C6), game::Direction(23)},
    {Coordinate::FromLeptons(0xFF49, 0x00BA), game::Direction(24)},
    {Coordinate::FromLeptons(0xFF4D, 0x00B0), game::Direction(25)},
    {Coordinate::FromLeptons(0xFF58, 0x00A8), game::Direction(27)},
    {Coordinate::FromLeptons(0xFF60, 0x00A0), game::Direction(28)},
    {Coordinate::FromLeptons(0xFF68, 0x0098), game::Direction(29)},
    {Coordinate::FromLeptons(0xFF70, 0x0090), game::Direction(31)},
    {Coordinate::FromLeptons(0xFF78, 0x0088), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF80, 0x0080),
     game::Direction(32)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0x0078), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF90, 0x0070), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF98, 0x0068), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFA0, 0x0060), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFA8, 0x0058), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFB0, 0x0050), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFB8, 0x0048), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFC0, 0x0040), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFC8, 0x0038), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFD0, 0x0030), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFD8, 0x0028), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFE0, 0x0020), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFE8, 0x0018), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF0, 0x0010), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF8, 0x0008), game::Direction(32)},
    {Coordinate{}, game::Direction(32)}};

const DriveClass::TrackType DriveClass::Track4[] = {
    {Coordinate::FromLeptons(0xFF00, 0x00F5), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00EB), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00E0), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF00, 0x00D5), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF01, 0x00CB), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF03, 0x00C0), game::Direction(0)},
    {Coordinate::FromLeptons(0xFF05, 0x00B5), game::Direction(1)},
    {Coordinate::FromLeptons(0xFF07, 0x00AB), game::Direction(1)},
    {Coordinate::FromLeptons(0xFF0A, 0x00A0), game::Direction(2)},
    {Coordinate::FromLeptons(0xFF0D, 0x0095), game::Direction(3)},
    {Coordinate::FromLeptons(0xFF10, 0x008B), game::Direction(4)},
    {Coordinate::FromLeptons(0xFF14, 0x0080),
     game::Direction(5)},  // Track entry here.
    {Coordinate::FromLeptons(0xFF18, 0x0075), game::Direction(8)},
    {Coordinate::FromLeptons(0xFF1C, 0x006D), game::Direction(12)},
    {Coordinate::FromLeptons(0xFF22, 0x0063), game::Direction(16)},
    {Coordinate::FromLeptons(0xFF25, 0x005A), game::Direction(20)},
    {Coordinate::FromLeptons(0xFF2B, 0x0052), game::Direction(23)},
    {Coordinate::FromLeptons(0xFF32, 0x0048), game::Direction(27)},
    {Coordinate::FromLeptons(0xFF37, 0x0040), game::Direction(32)},
    {Coordinate::FromLeptons(0xFF3D, 0x0038), game::Direction(36)},
    {Coordinate::FromLeptons(0xFF46, 0x0030), game::Direction(39)},
    {Coordinate::FromLeptons(0xFF4F, 0x002B), game::Direction(43)},
    {Coordinate::FromLeptons(0xFF58, 0x0024), game::Direction(47)},
    {Coordinate::FromLeptons(0xFF60, 0x0020), game::Direction(51)},
    {Coordinate::FromLeptons(0xFF6D, 0x001B), game::Direction(54)},
    {Coordinate::FromLeptons(0xFF79, 0x0017), game::Direction(57)},
    {Coordinate::FromLeptons(0xFF82, 0x0014),
     game::Direction(60)},  // Track jump here.
    {Coordinate::FromLeptons(0xFF8F, 0x0011), game::Direction(62)},
    {Coordinate::FromLeptons(0xFF98, 0x000D), game::Direction(63)},
    {Coordinate::FromLeptons(0xFFA2, 0x0009), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFAC, 0x0006), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFB5, 0x0004), game::Direction(66)},
    {Coordinate::FromLeptons(0xFFC0, 0x0003), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFCB, 0x0002), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x0001), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFE0, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFEB, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0000), game::Direction(64)},
    {Coordinate{}, game::Direction(64)}};

const DriveClass::TrackType DriveClass::Track5[] = {
    {Coordinate::FromLeptons(0xFE08, 0xFFF8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE10, 0xFFF0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE18, 0xFFE8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE20, 0xFFE0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE28, 0xFFD8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE30, 0xFFD0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE38, 0xFFC8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE40, 0xFFC0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE48, 0xFFB8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE50, 0xFFB0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE58, 0xFFA8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE60, 0xFFA0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE68, 0xFF98), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE70, 0xFF90), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE78, 0xFF88), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE80, 0xFF80),
     game::Direction(32)},  // Track entry here.
    {Coordinate::FromLeptons(0xFE88, 0xFF78), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE90, 0xFF71), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE97, 0xFF6A), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE9F, 0xFF62), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEA8, 0xFF5A), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEB0, 0xFF53), game::Direction(35)},
    {Coordinate::FromLeptons(0xFEB7, 0xFF4B), game::Direction(38)},
    {Coordinate::FromLeptons(0xFEBE, 0xFF44), game::Direction(41)},
    {Coordinate::FromLeptons(0xFEC4, 0xFF3E), game::Direction(44)},
    {Coordinate::FromLeptons(0xFECE, 0xFF39), game::Direction(47)},
    {Coordinate::FromLeptons(0xFED8, 0xFF34), game::Direction(50)},
    {Coordinate::FromLeptons(0xFEE0, 0xFF30), game::Direction(53)},
    {Coordinate::FromLeptons(0xFEEB, 0xFF2D), game::Direction(56)},
    {Coordinate::FromLeptons(0xFEF5, 0xFF2C), game::Direction(59)},
    {Coordinate::FromLeptons(0xFF00, 0xFF2B), game::Direction(62)},
    {Coordinate::FromLeptons(0xFF0B, 0xFF2C), game::Direction(66)},
    {Coordinate::FromLeptons(0xFF15, 0xFF2D), game::Direction(69)},
    {Coordinate::FromLeptons(0xFF1F, 0xFF30), game::Direction(72)},
    {Coordinate::FromLeptons(0xFF28, 0xFF34), game::Direction(75)},
    {Coordinate::FromLeptons(0xFF30, 0xFF39), game::Direction(78)},
    {Coordinate::FromLeptons(0xFF3A, 0xFF3E), game::Direction(81)},
    {Coordinate::FromLeptons(0xFF44, 0xFF44), game::Direction(84)},
    {Coordinate::FromLeptons(0xFF4B, 0xFF4B), game::Direction(87)},
    {Coordinate::FromLeptons(0xFF50, 0xFF53), game::Direction(90)},
    {Coordinate::FromLeptons(0xFF58, 0xFF5A), game::Direction(93)},
    {Coordinate::FromLeptons(0xFF60, 0xFF62), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF68, 0xFF6A), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF70, 0xFF71), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF78, 0xFF78), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF80, 0xFF80),
     game::Direction(96)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF88, 0xFF88), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF90, 0xFF90), game::Direction(96)},
    {Coordinate::FromLeptons(0xFF98, 0xFF98), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFA0, 0xFFA0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFA8, 0xFFA8), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFB0, 0xFFB0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFB8, 0xFFB8), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFC0, 0xFFC0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFC8, 0xFFC8), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFD0, 0xFFD0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFD8, 0xFFD8), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFE0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFE8, 0xFFE8), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFF0, 0xFFF0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFF8, 0xFFF8), game::Direction(96)},
    {Coordinate{}, game::Direction(96)}};

const DriveClass::TrackType DriveClass::Track6[] = {
    {Coordinate::FromLeptons(0xFE00, 0x0100), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE08, 0x00F8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE10, 0x00F0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE18, 0x00E8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE20, 0x00E0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE28, 0x00D8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE30, 0x00D0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE38, 0x00C8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE40, 0x00C0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE48, 0x00B8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE50, 0x00B0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE58, 0x00A8), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE60, 0x00A0), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE68, 0x0098), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE70, 0x0090), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE78, 0x0088), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE80, 0x0080),
     game::Direction(32)},  // Jump entry point here.
    {Coordinate::FromLeptons(0xFE88, 0x0078), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE90, 0x0070), game::Direction(32)},
    {Coordinate::FromLeptons(0xFE98, 0x0068), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEA0, 0x0060), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEA8, 0x0058), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEAE, 0x0055), game::Direction(32)},
    {Coordinate::FromLeptons(0xFEB8, 0x004E), game::Direction(35)},
    {Coordinate::FromLeptons(0xFEC0, 0x0048), game::Direction(37)},
    {Coordinate::FromLeptons(0xFEC9, 0x0042), game::Direction(40)},
    {Coordinate::FromLeptons(0xFED2, 0x003B), game::Direction(43)},
    {Coordinate::FromLeptons(0xFEDA, 0x0037), game::Direction(45)},
    {Coordinate::FromLeptons(0xFEE3, 0x0032), game::Direction(48)},
    {Coordinate::FromLeptons(0xFEEB, 0x002B), game::Direction(51)},
    {Coordinate::FromLeptons(0xFEF5, 0x0026), game::Direction(53)},
    {Coordinate::FromLeptons(0xFEFE, 0x0022), game::Direction(56)},
    {Coordinate::FromLeptons(0xFF08, 0x001C), game::Direction(59)},
    {Coordinate::FromLeptons(0xFF12, 0x0019), game::Direction(61)},
    {Coordinate::FromLeptons(0xFF1B, 0x0015), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF26, 0x0011), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF30, 0x000E), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF39, 0x000B), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF43, 0x0009), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF4E, 0x0007), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF57, 0x0005), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF62, 0x0003), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF6D, 0x0001), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF77, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF80, 0x0000),
     game::Direction(64)},  // Track jump check here.
    {Coordinate::FromLeptons(0xFF8B, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFF95, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFA0, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFAB, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFB5, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFC0, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFCB, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFE0, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFEB, 0x0000), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0000), game::Direction(64)},
    {Coordinate{}, game::Direction(64)}};

const DriveClass::TrackType DriveClass::Track7[] = {
    {Coordinate::FromLeptons(0xFFFF, 0x0006), game::Direction(0)},
    {Coordinate::FromLeptons(0xFFFE, 0x000C), game::Direction(4)},
    {Coordinate::FromLeptons(0xFFFC, 0x0011), game::Direction(8)},
    {Coordinate::FromLeptons(0xFFFA, 0x0018), game::Direction(12)},
    {Coordinate::FromLeptons(0xFFF6, 0x001F), game::Direction(16)},
    {Coordinate::FromLeptons(0xFFF3, 0x0024), game::Direction(19)},
    {Coordinate::FromLeptons(0xFFF0, 0x002B), game::Direction(22)},
    {Coordinate::FromLeptons(0xFFFD, 0x0030), game::Direction(23)},
    {Coordinate::FromLeptons(0xFFEB, 0x0035), game::Direction(24)},
    {Coordinate::FromLeptons(0xFFE8, 0x0038), game::Direction(25)},
    {Coordinate::FromLeptons(0xFFE6, 0x003C), game::Direction(26)},
    {Coordinate::FromLeptons(0xFFE3, 0x0040), game::Direction(27)},
    {Coordinate::FromLeptons(0xFFE0, 0x0043), game::Direction(28)},
    {Coordinate::FromLeptons(0xFFDD, 0x0046), game::Direction(29)},
    {Coordinate::FromLeptons(0xFFDF, 0x0043), game::Direction(30)},
    {Coordinate::FromLeptons(0xFFE1, 0x0040), game::Direction(30)},
    {Coordinate::FromLeptons(0xFFE3, 0x003C), game::Direction(30)},
    {Coordinate::FromLeptons(0xFFE5, 0x0038), game::Direction(30)},
    {Coordinate::FromLeptons(0xFFE7, 0x0035), game::Direction(31)},
    {Coordinate::FromLeptons(0xFFE9, 0x0030), game::Direction(31)},
    {Coordinate::FromLeptons(0xFFEB, 0x002B), game::Direction(31)},
    {Coordinate::FromLeptons(0xFFED, 0x0024), game::Direction(31)},
    {Coordinate::FromLeptons(0xFFF1, 0x001F), game::Direction(31)},
    {Coordinate::FromLeptons(0xFFF4, 0x0018), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF7, 0x0011), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFFA, 0x000C), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFFD, 0x0006), game::Direction(32)},
    {Coordinate{}, game::Direction(32)}};

const DriveClass::TrackType DriveClass::Track8[] = {
    {Coordinate::FromLeptons(0xFFFC, 0x0003), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFF7, 0x0006), game::Direction(36)},
    {Coordinate::FromLeptons(0xFFF1, 0x000A), game::Direction(40)},
    {Coordinate::FromLeptons(0xFFEB, 0x000C), game::Direction(44)},
    {Coordinate::FromLeptons(0xFFE4, 0x000D), game::Direction(46)},
    {Coordinate::FromLeptons(0xFFDC, 0x000E), game::Direction(48)},
    {Coordinate::FromLeptons(0xFFD5, 0x000F), game::Direction(50)},
    {Coordinate::FromLeptons(0xFFD0, 0x0010), game::Direction(52)},
    {Coordinate::FromLeptons(0xFFC9, 0x0011), game::Direction(54)},
    {Coordinate::FromLeptons(0xFFC2, 0x0012), game::Direction(56)},
    {Coordinate::FromLeptons(0xFFC0, 0x0011), game::Direction(58)},
    {Coordinate::FromLeptons(0xFFC2, 0x0010), game::Direction(60)},
    {Coordinate::FromLeptons(0xFFC9, 0x000E), game::Direction(62)},
    {Coordinate::FromLeptons(0xFFCF, 0x000C), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFD5, 0x000A), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFDA, 0x0008), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFE2, 0x0006), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFE9, 0x0004), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFEF, 0x0002), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFF5, 0x0001), game::Direction(64)},
    {Coordinate::FromLeptons(0xFFF9, 0x0000), game::Direction(64)},
    {Coordinate{}, game::Direction(64)}};

const DriveClass::TrackType DriveClass::Track9[] = {
    {Coordinate::FromLeptons(0x0002, 0xFFF5), game::Direction(0)},
    {Coordinate::FromLeptons(0x0004, 0xFFEB), game::Direction(2)},
    {Coordinate::FromLeptons(0x0006, 0xFFE0), game::Direction(4)},
    {Coordinate::FromLeptons(0x0009, 0xFFD5), game::Direction(6)},
    {Coordinate::FromLeptons(0x000C, 0xFFCE), game::Direction(9)},
    {Coordinate::FromLeptons(0x000F, 0xFFC8), game::Direction(11)},
    {Coordinate::FromLeptons(0x0012, 0xFFC0), game::Direction(13)},
    {Coordinate::FromLeptons(0x0015, 0xFFB8), game::Direction(16)},
    {Coordinate::FromLeptons(0x0012, 0xFFC0), game::Direction(18)},
    {Coordinate::FromLeptons(0x000E, 0xFFC8), game::Direction(20)},
    {Coordinate::FromLeptons(0x000A, 0xFFCE), game::Direction(22)},
    {Coordinate::FromLeptons(0x0004, 0xFFD5), game::Direction(24)},
    {Coordinate::FromLeptons(0x0000, 0xFFDE), game::Direction(26)},
    {Coordinate::FromLeptons(0xFFF8, 0xFFE9), game::Direction(28)},
    {Coordinate::FromLeptons(0xFFF2, 0xFFEE), game::Direction(30)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFF5), game::Direction(32)},
    {Coordinate::FromLeptons(0xFFE1, 0xFFFD), game::Direction(34)},
    {Coordinate::FromLeptons(0xFFD8, 0x0002), game::Direction(36)},
    {Coordinate::FromLeptons(0xFFD2, 0x0007), game::Direction(39)},
    {Coordinate::FromLeptons(0xFFCB, 0x000B), game::Direction(41)},
    {Coordinate::FromLeptons(0xFFC5, 0x0010), game::Direction(43)},
    {Coordinate::FromLeptons(0xFFBE, 0x0013), game::Direction(45)},
    {Coordinate::FromLeptons(0xFFB7, 0x0015), game::Direction(48)},
    {Coordinate::FromLeptons(0xFFBE, 0x0013), game::Direction(50)},
    {Coordinate::FromLeptons(0xFFC5, 0x0011), game::Direction(52)},
    {Coordinate::FromLeptons(0xFFCC, 0x000B), game::Direction(54)},
    {Coordinate::FromLeptons(0xFFD4, 0x0008), game::Direction(56)},
    {Coordinate::FromLeptons(0xFFDF, 0x0005), game::Direction(58)},
    {Coordinate::FromLeptons(0xFFEB, 0x0003), game::Direction(62)},
    {Coordinate::FromLeptons(0xFFF5, 0x0001), game::Direction(64)},
    {Coordinate{}, game::Direction(64)}};

const DriveClass::TrackType DriveClass::Track10[] = {
    {Coordinate::FromLeptons(0x000B, 0xFFF6), game::Direction(32)},
    {Coordinate::FromLeptons(0x0015, 0xFFF0), game::Direction(37)},
    {Coordinate::FromLeptons(0x0020, 0xFFEB), game::Direction(42)},
    {Coordinate::FromLeptons(0x002B, 0xFFE9), game::Direction(47)},
    {Coordinate::FromLeptons(0x0032, 0xFFE5), game::Direction(52)},
    {Coordinate::FromLeptons(0x0038, 0xFFE3), game::Direction(57)},
    {Coordinate::FromLeptons(0x0040, 0xFFE0), game::Direction(60)},
    {Coordinate::FromLeptons(0x0038, 0xFFE2), game::Direction(62)},
    {Coordinate::FromLeptons(0x0032, 0xFFE4), game::Direction(64)},
    {Coordinate::FromLeptons(0x002A, 0xFFE5), game::Direction(68)},
    {Coordinate::FromLeptons(0x001E, 0xFFE6), game::Direction(70)},
    {Coordinate::FromLeptons(0x0015, 0xFFE7), game::Direction(72)},
    {Coordinate::FromLeptons(0x000B, 0xFFE8), game::Direction(74)},
    {Coordinate::FromLeptons(0x0000, 0xFFE9), game::Direction(76)},
    {Coordinate::FromLeptons(0xFFF5, 0xFFE8), game::Direction(78)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFE7), game::Direction(80)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFE6), game::Direction(82)},
    {Coordinate::FromLeptons(0xFFD5, 0xFFE5), game::Direction(84)},
    {Coordinate::FromLeptons(0xFFCE, 0xFFE4), game::Direction(86)},
    {Coordinate::FromLeptons(0xFFC5, 0xFFE2), game::Direction(88)},
    {Coordinate::FromLeptons(0xFFC0, 0xFFE0), game::Direction(90)},
    {Coordinate::FromLeptons(0xFFC5, 0xFFE3), game::Direction(92)},
    {Coordinate::FromLeptons(0xFFCE, 0xFFE5), game::Direction(94)},
    {Coordinate::FromLeptons(0xFFD5, 0xFFE9), game::Direction(95)},
    {Coordinate::FromLeptons(0xFFE0, 0xFFEB), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFEB, 0xFFF0), game::Direction(96)},
    {Coordinate::FromLeptons(0xFFF5, 0xFFF6), game::Direction(96)},
    {Coordinate{}, game::Direction(96)}};

const DriveClass::TrackType DriveClass::Track11[] = {
    {Coordinate::FromLeptons(0x0000, 0x0100), game::Direction::kSouthWest},
    {Coordinate::FromLeptons(0x0008, 0x00F3), game::Direction::kSouthWest},
    {Coordinate::FromLeptons(0x0010, 0x00E5), kRefineryApproach},
    {Coordinate::FromLeptons(0x0018, 0x00D6), kRefineryApproach},
    {Coordinate::FromLeptons(0x0020, 0x00C8), kRefineryApproach},
    {Coordinate::FromLeptons(0x0028, 0x00B9), kRefineryApproach},
    {Coordinate::FromLeptons(0x0030, 0x00AB), kRefineryDock},
    {Coordinate::FromLeptons(0x0038, 0x009C), kRefineryDock},
    {Coordinate::FromLeptons(0x0040, 0x008D), kRefineryDock},
    {Coordinate::FromLeptons(0x0048, 0x007F), kRefineryDock},
    {Coordinate::FromLeptons(0x0050, 0x0071), kRefineryDock},
    {Coordinate::FromLeptons(0x0058, 0x0064), kRefineryDock},
    {Coordinate::FromLeptons(0x0060, 0x0055), kRefineryDock},

    {Coordinate{}, kRefineryDock}};

const DriveClass::TrackType DriveClass::Track12[] = {
    {Coordinate::FromLeptons(0x0060, 0xFF55), kRefineryDock},
    {Coordinate::FromLeptons(0x0058, 0xFF64), kRefineryDock},
    {Coordinate::FromLeptons(0x0050, 0xFF71), kRefineryDock},
    {Coordinate::FromLeptons(0x0048, 0xFF7F), kRefineryDock},
    {Coordinate::FromLeptons(0x0040, 0xFF8D), kRefineryDock},
    {Coordinate::FromLeptons(0x0038, 0xFF9C), kRefineryDock},
    {Coordinate::FromLeptons(0x0030, 0xFFAB), kRefineryDock},
    {Coordinate::FromLeptons(0x0028, 0xFFB9), kRefineryApproach},
    {Coordinate::FromLeptons(0x0020, 0xFFC8), kRefineryApproach},
    {Coordinate::FromLeptons(0x0018, 0xFFD6), kRefineryApproach},
    {Coordinate::FromLeptons(0x0010, 0xFFE5), kRefineryApproach},
    {Coordinate::FromLeptons(0x0008, 0xFFF3), game::Direction::kSouthWest},

    {Coordinate{}, game::Direction::kSouthWest}};

/*
**	Drive out of weapon's factory.
*/
const DriveClass::TrackType DriveClass::Track13[] = {
    {CoordFromPixels(0, -35), game::Direction::kSouth},
    {CoordFromPixels(0, -34), game::Direction::kSouth},
    {CoordFromPixels(0, -33), game::Direction::kSouth},
    {CoordFromPixels(0, -32), game::Direction::kSouth},
    {CoordFromPixels(0, -31), game::Direction::kSouth},
    {CoordFromPixels(0, -30), game::Direction::kSouth},
    {CoordFromPixels(0, -29), game::Direction::kSouth},
    {CoordFromPixels(0, -28), game::Direction::kSouth},
    {CoordFromPixels(0, -27), game::Direction::kSouth},
    {CoordFromPixels(0, -26), game::Direction::kSouth},
    {CoordFromPixels(0, -25), game::Direction::kSouth},
    {CoordFromPixels(0, -24), game::Direction::kSouth},
    {CoordFromPixels(0, -23), game::Direction::kSouth},
    {CoordFromPixels(0, -22), game::Direction::kSouth},
    {CoordFromPixels(0, -21), game::Direction::kSouth},
    {CoordFromPixels(0, -20), game::Direction::kSouth},
    {CoordFromPixels(0, -19), game::Direction::kSouth},
    {CoordFromPixels(0, -18), game::Direction::kSouth},
    {CoordFromPixels(0, -17), game::Direction::kSouth},
    {CoordFromPixels(0, -16), game::Direction::kSouth},
    {CoordFromPixels(0, -15), game::Direction::kSouth},
    {CoordFromPixels(0, -14), game::Direction::kSouth},
    {CoordFromPixels(0, -13), game::Direction::kSouth},
    {CoordFromPixels(0, -12), game::Direction::kSouth},
    {CoordFromPixels(0, -11), game::Direction::kSouth},
    {CoordFromPixels(0, -10), game::Direction::kSouth},
    {CoordFromPixels(0, -9), game::Direction::kSouth},
    {CoordFromPixels(0, -8), game::Direction::kSouth},
    {CoordFromPixels(0, -7), game::Direction::kSouth},
    {CoordFromPixels(0, -6), game::Direction::kSouth},
    {CoordFromPixels(0, -5), game::Direction::kSouth},
    {CoordFromPixels(0, -4), game::Direction::kSouth},
    {CoordFromPixels(0, -3), game::Direction::kSouth},
    {CoordFromPixels(0, -2), game::Direction::kSouth},
    {CoordFromPixels(0, -1), game::Direction::kSouth},

    {Coordinate{}, game::Direction::kSouth}};

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
    {1, 0, game::Direction::kNorth, F_},               //	0-0
    {3, 7, game::Direction::kNorthEast, F_D},          //	0-1 (raw chart)
    {4, 9, game::Direction::kEast, F_D},               //	0-2 (raw chart)
    {0, 0, game::Direction::kSouthEast, F_},           //	0-3 !
    {0, 0, game::Direction::kSouth, F_},               //	0-4 !
    {0, 0, game::Direction::kSouthWest, F_},           //	0-5 !
    {4, 9, game::Direction::kWest, (F_X | F_D)},       //	0-6
    {3, 7, game::Direction::kNorthWest, (F_X | F_D)},  //	0-7
    {6, 8, game::Direction::kNorth, (F_T | F_X | F_Y | F_D)},  //	1-0
    {2, 0, game::Direction::kNorthEast, F_},    //	1-1 (raw chart)
    {6, 8, game::Direction::kEast, F_D},        //	1-2 (raw chart)
    {5, 10, game::Direction::kSouthEast, F_D},  //	1-3 (raw chart)
    {0, 0, game::Direction::kSouth, F_},        //	1-4 !
    {0, 0, game::Direction::kSouthWest, F_},    //	1-5 !
    {0, 0, game::Direction::kWest, F_},         //	1-6 !
    {5, 10, game::Direction::kNorthWest, (F_T | F_X | F_Y | F_D)},  //	1-7
    {4, 9, game::Direction::kNorth, (F_T | F_X | F_Y | F_D)},       //	2-0
    {3, 7, game::Direction::kNorthEast, (F_T | F_X | F_Y | F_D)},   //	2-1
    {1, 0, game::Direction::kEast, (F_T | F_X)},                    //	2-2
    {3, 7, game::Direction::kSouthEast, (F_T | F_X | F_D)},         //	2-3
    {4, 9, game::Direction::kSouth, (F_T | F_X | F_D)},             //	2-4
    {0, 0, game::Direction::kSouthWest, F_},                        //	2-5 !
    {0, 0, game::Direction::kWest, F_},                             //	2-6 !
    {0, 0, game::Direction::kNorthWest, F_},                        //	2-7 !
    {0, 0, game::Direction::kNorth, F_},                            //	3-0 !
    {5, 10, game::Direction::kNorthEast, (F_Y | F_D)},              //	3-1
    {6, 8, game::Direction::kEast, (F_Y | F_D)},                    //	3-2
    {2, 0, game::Direction::kSouthEast, F_Y},                       //	3-3
    {6, 8, game::Direction::kSouth, (F_T | F_X | F_D)},             //	3-4
    {5, 10, game::Direction::kSouthWest, (F_T | F_X | F_D)},        //	3-5
    {0, 0, game::Direction::kWest, F_},                             //	3-6 !
    {0, 0, game::Direction::kNorthWest, F_},                        //	3-7 !
    {0, 0, game::Direction::kNorth, F_},                            //	4-0 !
    {0, 0, game::Direction::kNorthEast, F_},                        //	4-1 !
    {4, 9, game::Direction::kEast, (F_Y | F_D)},                    //	4-2
    {3, 7, game::Direction::kSouthEast, (F_Y | F_D)},               //	4-3
    {1, 0, game::Direction::kSouth, F_Y},                           //	4-4
    {3, 7, game::Direction::kSouthWest, (F_X | F_Y | F_D)},         //	4-5
    {4, 9, game::Direction::kWest, (F_X | F_Y | F_D)},              //	4-6
    {0, 0, game::Direction::kNorthWest, F_},                        //	4-7 !
    {0, 0, game::Direction::kNorth, F_},                            //	5-0 !
    {0, 0, game::Direction::kNorthEast, F_},                        //	5-1 !
    {0, 0, game::Direction::kEast, F_},                             //	5-2 !
    {5, 10, game::Direction::kSouthEast, (F_T | F_D)},              //	5-3
    {6, 8, game::Direction::kSouth, (F_T | F_D)},                   //	5-4
    {2, 0, game::Direction::kSouthWest, F_T},                       //	5-5
    {6, 8, game::Direction::kWest, (F_X | F_Y | F_D)},              //	5-6
    {5, 10, game::Direction::kNorthWest, (F_X | F_Y | F_D)},        //	5-7
    {4, 9, game::Direction::kNorth, (F_T | F_Y | F_D)},             //	6-0
    {0, 0, game::Direction::kNorthEast, F_},                        //	6-1 !
    {0, 0, game::Direction::kEast, F_},                             //	6-2 !
    {0, 0, game::Direction::kSouthEast, F_},                        //	6-3 !
    {4, 9, game::Direction::kSouth, (F_T | F_D)},                   //	6-4
    {3, 7, game::Direction::kSouthWest, (F_T | F_D)},               //	6-5
    {1, 0, game::Direction::kWest, F_T},                            //	6-6
    {3, 7, game::Direction::kNorthWest, (F_T | F_Y | F_D)},         //	6-7
    {6, 8, game::Direction::kNorth, (F_T | F_Y | F_D)},             //	7-0
    {5, 10, game::Direction::kNorthEast, (F_T | F_Y | F_D)},        //	7-1
    {0, 0, game::Direction::kEast, F_},                             //	7-2 !
    {0, 0, game::Direction::kSouthEast, F_},                        //	7-3 !
    {0, 0, game::Direction::kSouth, F_},                            //	7-4 !
    {5, 10, game::Direction::kSouthWest, (F_X | F_D)},              //	7-5
    {6, 8, game::Direction::kWest, (F_X | F_D)},                    //	7-6
    {2, 0, game::Direction::kNorthWest, F_X},                       //	7-7

    {11, 11, game::Direction::kSouthWest,
     F_},                         // Backup harvester into refinery.
    {12, 12, kRefineryDock, F_},  // Drive back into refinery.
    {13, 13, game::Direction::kSouthWest, F_}  // Drive out of weapons factory.
};

}  // namespace ra
