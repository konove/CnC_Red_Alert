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
 * Functions: * DriveClass::AI -- Processes unit movement and rotation. *
 *   DriveClass::Approach_Target -- Handles approaching the target in order to
 *attack it.      * DriveClass::Assign_Destination -- Set the unit's NavCom. *
 *   DriveClass::Class_Of -- Fetches a reference to the class type for this
 *object.            * DriveClass::Debug_Dump -- Displays status information to
 *monochrome screen.               * DriveClass::Do_Turn -- Tries to turn the
 *vehicle to the specified direction.              * DriveClass::DriveClass --
 *Constructor for drive class object.                             *
 *   DriveClass::Exit_Map -- Give the unit a movement order to exit the map. *
 *   DriveClass::Fixup_Path -- Adds smooth start path to normal movement path. *
 *   DriveClass::Force_Track -- Forces the unit to use the indicated track. *
 *   DriveClass::Lay_Track -- Handles track laying logic for the unit. *
 *   DriveClass::Offload_Tiberium_Bail -- Offloads one Tiberium quantum from the
 *object.       * DriveClass::Ok_To_Move -- Checks to see if this object can
 *begin moving.                  * DriveClass::Overrun_Square -- Handles vehicle
 *overrun of a cell.                          * DriveClass::Per_Cell_Process --
 *Handles when unit finishes movement into a cell.          *
 *   DriveClass::Smooth_Turn -- Handles the low level coord calc for smooth turn
 *logic.        * DriveClass::Start_Of_Move -- Tries to get a unit to advance
 *toward cell.                  * DriveClass::Tiberium_Load -- Determine the
 *Tiberium load as a percentage.                 * DriveClass::While_Moving --
 *Processes unit movement.                                      *
 *   DriveClass::Mark_Track -- Marks the midpoint of the track as occupied. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/drive.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "td/audio.h"
#include "td/building.h"
#include "td/cell.h"
#include "td/compat.h"
#include "td/config.h"
#include "td/const.h"
#include "td/defines.h"
#include "td/display_constants.h"
#include "td/externs.h"
#include "td/facing.h"
#include "td/foot.h"
#include "td/ftimer.h"
#include "td/globals.h"
#include "td/house.h"
#include "td/inline.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/overlay.h"
#include "td/special.h"
#include "td/target.h"
#include "td/techno.h"
#include "td/type.h"
#include "td/unit.h"
#include "td/vector.h"

DriveClass::DriveClass() : Class(nullptr) {};

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
void DriveClass::Do_Turn(DirType dir) {
  if (dir != PrimaryFacing) {
    /*
    **	Special rotation track is needed for units that
    **	cannot rotate in place.
    */
    if (Special.IsThreePoint && TrackNumber == -1 &&
        Class->Speed == SPEED_WHEEL) {
      int facediff;     // Signed difference between current and desired facing.
      FacingType face;  // Current facing (ordinal value).

      facediff = PrimaryFacing.Difference(dir) >> 5;
      facediff = Bound(facediff, -2, 2);
      if (facediff) {
        face = Dir_Facing(PrimaryFacing);

        IsOnShortTrack = true;
        Force_Track(face * FACING_COUNT + (face + facediff), Coord);

        Path[0] = FACING_NONE;
        Set_Speed(0xFF);  // Full speed.
      }
    } else {
      PrimaryFacing.Set_Desired(dir);
      if (Special.IsJurassic && AreThingiesEnabled &&
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
void DriveClass::Force_Track(int track, COORDINATE coord) {
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
    return Cardinal_To_Fixed(UnitTypeClass::STEP_COUNT, Tiberium);
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
  if (!House->IsHuman && Target_Legal(TarCom) && !Target_Legal(NavCom)) {
    /*
    **	Special case:
    **	If this is for a unit that can crush infantry, and the target is
    **	infantry, AND the infantry is pretty darn close, then just try
    **	to drive over the infantry instead of firing on it.
    */
    TechnoClass* target = As_Techno(TarCom);
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
void DriveClass::Overrun_Square(CELL cell, bool threaten) {
  CellClass* cellptr = &Map[cell];

  if (Class->IsCrusher) {
    if (threaten) {
      /*
      **	If the cell contains infantry, then they will panic when a
      *vehicle tries *	drive over them. Have the infantry run away instead.
      */
      if (cellptr->Flag.Composite & 0x1F) {
        /*
        **	Scattering is controlled by the game difficulty level.
        */
        if ((Special.IsDifficult || Special.IsScatter || Scenario > 8) &&
            !Special.IsEasy) {
          cellptr->Incoming(0, true);
        }
      }
    } else {
      ObjectClass* object = cellptr->Cell_Occupier();
      int crushed = false;
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
          delete object;
          new OverlayClass(OVERLAY_SQUISH, Coord_Cell(Coord));

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
    : Class(&UnitTypeClass::As_Reference(classid)), FootClass(house) {
  /*
  **	For two shooters, clear out the second shot flag -- it will be set the
  *first time *	the object fires. For non two shooters, set the flag since it
  *will never be cleared *	and the second shot flag tells the system that
  *normal rearm times apply -- this is *	what is desired for non two
  *shooters.
  */
  IsSecondShot = Class->IsTwoShooter == 0;
  IsHarvesting = false;
  IsTurretLockedDown = false;
  IsOnShortTrack = false;
  IsReturning = false;
  TrackNumber = -1;
  TrackIndex = 0;
  SpeedAccum = 0;
  Tiberium = 0;
  Strength = Class->MaxStrength;
}

/***********************************************************************************************
 * DriveClass::Debug_Dump -- Displays status information to monochrome screen. *
 *                                                                                             *
 *    This debug utility function will display the status of the drive class to
 *the mono       * screen. It is through this information that bugs can be
 *tracked down.                    *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 05/31/1994 JLB : Created. *
 *=============================================================================================*/
void DriveClass::Debug_Dump(MonoClass* mono) const {
  if constexpr (config::kCheatKeysEnabled) {
    mono->Set_Cursor(33, 7);
    mono->Printf("%2d:%2d", TrackNumber, TrackIndex);
    mono->Text_Print("X", 16 + (IsTurretLockedDown ? 2 : 0), 10);
    //	mono->Text_Print("X", 16 + (IsOnShortTrack?2:0), 11);
    mono->Set_Cursor(41, 7);
    mono->Printf("%d", Fixed_To_Cardinal(100, Tiberium_Load()));
    FootClass::Debug_Dump(mono);
  }
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
  CELL cell;  // Map exit cell number.

  if (*this == UNIT_HOVER && !Target_Legal(NavCom)) {
    /*
    **	Scan a swath of cells from current position to the edge of the map and
    *if *	there is any blocking object, just wait so to try again later.
    */
    Mark(MARK_UP);
    for (int x = Cell_X(Coord_Cell(Center_Coord())) - 1;
         x <= Cell_X(Coord_Cell(Center_Coord())) + 1; x++) {
      for (int y = Cell_Y(Coord_Cell(Center_Coord())) + 1;
           y < Map.MapCellY + Map.MapCellHeight; y++) {
        cell = XY_Cell(x, y);
        if (Map[cell].Cell_Techno()) {
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
    cell = XY_Cell(Cell_X(Coord_Cell(Coord)), Map.MapCellY + Map.MapCellHeight);
    IsReturning = true;
    Assign_Destination(::As_Target(cell));
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
COORDINATE DriveClass::Smooth_Turn(COORDINATE adj, DirType* dir) {
  DirType workdir = *dir;
  int x, y;
  int temp;
  TrackControlType flags = TrackControl[TrackNumber].Flag;

  x = Coord_X(adj);
  y = Coord_Y(adj);

  if (flags & F_T) {
    temp = x;
    x = y;
    y = temp;
    workdir = static_cast<DirType>(DIR_W - workdir);
  }

  if (flags & F_X) {
    x = -x;
    workdir = static_cast<DirType>(-workdir);
  }

  if (flags & F_Y) {
    y = -y;
    workdir = static_cast<DirType>(DIR_S - workdir);
  }

  *dir = workdir;

  return XY_Coord(Coord_X(Head_To_Coord()) + x, Coord_Y(Head_To_Coord()) + y);
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
void DriveClass::Assign_Destination(TARGET target) {
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
  if (Is_Target_Cell(target)) {
    cell = As_Cell(target);

    tunit = Map[cell].Cell_Unit();
    if (tunit) {
      /*
      **	Prevent targeting of itself.
      */
      if (tunit != this) {
        target = tunit->As_Target();
      }
    } else {
      tbuilding = Map[cell].Cell_Building();
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
  BuildingClass* b = As_Building(target);

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
      target = 0;
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
        CELL cell = Coord_Cell(b->Center_Coord()) + (MAP_CELL_W - 1);
        if (Ground[Map[cell].Land_Type()].Cost[Class->Speed]) {
          if (Transmit_Message(RADIO_DOCKING) == RADIO_ROGER) {
            FootClass::Assign_Destination(target);
            Path[0] = FACING_NONE;
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
  Path[0] = FACING_NONE;  // Force recalculation of path.
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
  int actual;  // Working movement addition value.

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
    actual = SpeedAccum + Fixed_To_Cardinal(Class->MaxSpeed / 2, Speed);
  } else {
    actual = SpeedAccum + Fixed_To_Cardinal(Class->MaxSpeed, Speed);
  }

  if (actual > PIXEL_LEPTON_W) {
    const TurnTrackType* track;  // Track control pointer.
    const TrackType* ptr;        // Pointer to coord offset values.
    int tracknum;                // The track number being processed.
    FacingType nextface;         // Next facing queued in path.
    bool adj;                    // Is a turn coming up?

    track = &TrackControl[TrackNumber];
    if (IsOnShortTrack) {
      tracknum = track->StartTrack;
    } else {
      tracknum = track->Track;
    }
    ptr = RawTracks[tracknum - 1].Track;
    nextface = Path[0];

    /*
    **	Determine if there is a turn coming up. If there is
    **	a turn, then track jumping might occur.
    */
    adj = false;
    if (nextface != FACING_NONE && Dir_Facing(track->Facing) != nextface) {
      adj = true;
    }

    /*
    **	Skip ahead the number of track steps required (limited only
    **	by track length). Set the unit to the new position and
    **	flag the unit accordingly.
    */
    Mark(MARK_UP);
    while (actual > PIXEL_LEPTON_W) {
      COORDINATE offset;
      DirType dir;

      actual -= PIXEL_LEPTON_W;

      offset = ptr[TrackIndex].Offset;
      if (offset || !TrackIndex) {
        dir = ptr[TrackIndex].Facing;
        Coord = Smooth_Turn(offset, &dir);

        PrimaryFacing.Set(dir);

        /*
        **	See if "per cell" processing is necessary.
        */
        if (TrackIndex && RawTracks[tracknum - 1].Cell == TrackIndex) {
          Per_Cell_Process(false);
          if (!IsActive) {
            return false;
          }
        }

        /*
        **	The unit could "jump tracks". Check to see if the unit should
        **	do so.
        */
        if (*this != UNIT_GUNBOAT && nextface != FACING_NONE && adj &&
            RawTracks[tracknum - 1].Jump == TrackIndex && TrackIndex) {
          const TurnTrackType* newtrack;  // Proposed jump-to track.
          int tnum;

          tnum = Dir_Facing(track->Facing) * FACING_COUNT + nextface;
          newtrack = &TrackControl[tnum];
          if (newtrack->Track && RawTracks[newtrack->Track - 1].Entry) {
            COORDINATE c = Head_To_Coord();
            int oldspeed = Speed;

            c = Adjacent_Cell(c, nextface);

            switch (Can_Enter_Cell(Coord_Cell(c), nextface)) {
              case MOVE_OK:
                IsOnShortTrack = false;  // Shouldn't be necessary, but...
                TrackNumber = tnum;
                track = newtrack;

                //			Mono_Printf("**Jumping from track %d to
                // track %d. **\n", tracknum, track->Track);Keyboard::Get();

                tracknum = track->Track;
                TrackIndex =
                    RawTracks[tracknum - 1].Entry - 1;  // Anticipate increment.
                ptr = RawTracks[tracknum - 1].Track;
                adj = false;

                Stop_Driver();
                Per_Cell_Process(true);
                if (Start_Driver(c)) {
                  Set_Speed(oldspeed);
                  memmove(&Path[0], &Path[1], CONQUER_PATH_MAX - 1);
                  Path[CONQUER_PATH_MAX - 1] = FACING_NONE;
                } else {
                  Path[0] = FACING_NONE;
                  TrackNumber = -1;
                  actual = 0;
                }
                break;

              case MOVE_CLOAK:
                Map[Coord_Cell(c)].Shimmer();
                break;

              case MOVE_TEMP:
                if (*this == UNIT_HARVESTER || !House->IsHuman) {
                  bool old = Special.IsScatter;
                  Special.IsScatter = true;
                  Map[Coord_Cell(c)].Incoming(0, true);
                  Special.IsScatter = old;
                }
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
  CELL cell = Coord_Cell(Coord);

  /*
  **	Check to see if it has reached its destination. If so, then clear the
  *NavCom *	regardless of the remaining path list.
  */
  if (center && As_Cell(NavCom) == cell) {
    IsTurretLockedDown = false;
    NavCom = kTargetNone;
    Path[0] = FACING_NONE;
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
  FacingType facing;  // Direction movement will commence.
  DirType dir;        // Desired actual facing toward destination.
  int facediff;       // Difference between current and desired facing.
  int speed;          // Speed of unit.
  CELL destcell;      // Cell of destination.
  LandType ground;    // Ground unit is entering.
  COORDINATE dest;    // Destination coordinate.

  facing = Path[0];

  if (!Target_Legal(NavCom) && facing == FACING_NONE) {
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
  if (facing != FACING_NONE) {
    int dist;

    if (Is_Target_Unit(NavCom) || Is_Target_Infantry(NavCom)) {
      dist = Lepton_To_Cell(Distance(NavCom));

      //			if (dist > CELL_LEPTON_W ||
      //				!As_Techno(NavCom)->Techno_Type_Class()->IsCrushable
      //|| 				!Class->IsCrusher) {

      if (dist < CONQUER_PATH_MAX) {
        Path[dist] = FACING_NONE;
        facing = Path[0];  // Maybe needed.
      }
      //			}
    }
  }

  /*
  **	If the path is invalid at this point, then generate one. If
  **	generating a new path fails, then abort NavCom.
  */
  if (facing == FACING_NONE) {
    /*
    **	If after a path search, there is still no valid path, then set the
    **	NavCom to null and let the script take care of assigning a new
    **	navigation target.
    */
    if (!PathDelay.Expired()) {
      return false;
    }
    if (!Basic_Path()) {
      if (Distance(NavCom) < 0x0280 &&
          (Mission == MISSION_MOVE || Mission == MISSION_GUARD_AREA)) {
        Assign_Destination(kTargetNone);
      } else {
        /*
        **	If a basic path could be found, but the immediate move
        *destination is *	blocked by a friendly temporary blockage, then
        *cause that blockage *	to scatter.
        */
        CELL cell =
            Adjacent_Cell(Coord_Cell(Center_Coord()), PrimaryFacing.Current());
        if (Map.In_Radar(cell)) {
          if (Can_Enter_Cell(cell) == MOVE_TEMP) {
            CellClass* cellptr = &Map[cell];
            TechnoClass* blockage = cellptr->Cell_Techno();
            if (blockage && House->Is_Ally(blockage)) {
              bool old = Special.IsScatter;
              Special.IsScatter = true;
              cellptr->Incoming(0, true);
              Special.IsScatter = old;
            }
          }
        }

        if (TryTryAgain) {
          TryTryAgain--;
        } else {
          Assign_Destination(kTargetNone);
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
    CELL cell = Adjacent_Cell(Coord_Cell(Center_Coord()), Path[0]);
    if (Map.In_Radar(cell)) {
      if (Can_Enter_Cell(cell) == MOVE_TEMP) {
        CellClass* cellptr = &Map[cell];
        TechnoClass* blockage = cellptr->Cell_Techno();
        if (blockage && House->Is_Ally(blockage)) {
          bool old = Special.IsScatter;
          Special.IsScatter = true;
          cellptr->Incoming(0, true);
          Special.IsScatter = old;
        }
      }
    }

    TryTryAgain = PATH_RETRY;
    facing = Path[0];
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
  dir = Facing_Dir(facing);

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
  destcell = Coord_Cell(dest);
  Mark(MARK_UP);
  MoveType cando = Can_Enter_Cell(destcell, facing);
  Mark(MARK_DOWN);

  if (cando != MOVE_OK) {
    if (Mission == MISSION_MOVE && House->IsHuman &&
        Distance(NavCom) < 0x0200) {
      Assign_Destination(kTargetNone);
    }

    /*
    **	If a temporary friendly object is blocking the path, then cause
    *it to *	get out of the way.
    */
    if (cando == MOVE_TEMP) {
      bool old = Special.IsScatter;
      Special.IsScatter = true;
      Map[destcell].Incoming(0, true);
      Special.IsScatter = old;
    }

    /*
    **	If a cloaked object is blocking, then shimmer the cell.
    */
    if (cando == MOVE_CLOAK) {
      Map[destcell].Shimmer();
    }

    Stop_Driver();
    if (cando != MOVE_MOVING_BLOCK) {
      Path[0] = FACING_NONE;  // Path is blocked!
    }

    /*
    ** If blocked by a moving block then just exit start of move and
    ** try again next tick.
    */
    if (cando == MOVE_DESTROYABLE) {
      if (Map[destcell].Cell_Object()) {
        if (!House->Is_Ally(Map[destcell].Cell_Object())) {
          Override_Mission(MISSION_ATTACK,
                           Map[destcell].Cell_Object()->As_Target(),
                           kTargetNone);
        }
      } else {
        if (Map[destcell].Overlay != OVERLAY_NONE &&
            OverlayTypeClass::As_Reference(Map[destcell].Overlay).IsWall) {
          Override_Mission(MISSION_ATTACK, ::As_Target(destcell), kTargetNone);
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
  ground = Map[destcell].Land_Type();
  speed = Ground[ground].Cost[Class->Speed];
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
  if (Class->MaxStrength >> 1 > Strength) {
    speed -= speed >> 2;  // Three quarters speed.
  }
  if (speed != Speed /* || !SpeedAdd*/) {
    Set_Speed(speed);  // Full speed.
  }

  /*
  **	Reserve the destination cell so that it won't become
  **	occupied AS this unit is moving into it.
  */
  if (cando != MOVE_OK) {
    Path[0] = FACING_NONE;  // Path is blocked!
    TrackNumber = -1;
    dest = 0;
  } else {
    Overrun_Square(Coord_Cell(dest), true);

    /*
    **	Determine which track to use (based on recorded path).
    */
    FacingType nextface = Path[1];
    if (nextface == FACING_NONE) {
      nextface = facing;
    }

    IsOnShortTrack = false;
    TrackNumber = facing * FACING_COUNT + nextface;
    if (TrackControl[TrackNumber].Track == 0) {
      Path[0] = FACING_NONE;
      TrackNumber = -1;
      return true;
    }
    if (TrackControl[TrackNumber].Flag & F_D) {
      /*
      **	If the middle cell of a two cell track contains a crate,
      **	the check for goodies before movement starts.
      */
      if (!Map[destcell].Goodie_Check(this)) {
        cando = MOVE_NO;
      } else {
        dest = Adjacent_Cell(dest, nextface);
        destcell = Coord_Cell(dest);
        cando = Can_Enter_Cell(destcell);
      }

      if (cando != MOVE_OK) {
        /*
        **	If a temporary friendly object is blocking the path, then cause
        *it to *	get out of the way.
        */
        if (cando == MOVE_TEMP) {
          bool old = Special.IsScatter;
          Special.IsScatter = true;
          Map[destcell].Incoming(0, true);
          Special.IsScatter = old;
        }

        /*
        **	If a cloaked object is blocking, then shimmer the cell.
        */
        if (cando == MOVE_CLOAK) {
          Map[destcell].Shimmer();
        }

        Path[0] = FACING_NONE;  // Path is blocked!
        TrackNumber = -1;
        dest = 0;
        if (cando == MOVE_DESTROYABLE) {
          if (Map[destcell].Cell_Object()) {
            if (!House->Is_Ally(Map[destcell].Cell_Object())) {
              Override_Mission(MISSION_ATTACK,
                               Map[destcell].Cell_Object()->As_Target(),
                               kTargetNone);
            }
          } else {
            if (Map[destcell].Overlay != OVERLAY_NONE &&
                OverlayTypeClass::As_Reference(Map[destcell].Overlay).IsWall) {
              Override_Mission(MISSION_ATTACK, ::As_Target(destcell),
                               kTargetNone);
            }
          }
          IsNewNavCom = false;
          TrackIndex = 0;
          return true;
        }
      } else {
        memmove(&Path[0], &Path[2], CONQUER_PATH_MAX - 2);
        Path[CONQUER_PATH_MAX - 2] = FACING_NONE;
        IsPlanningToLook = true;
      }
    } else {
      memmove(&Path[0], &Path[1], CONQUER_PATH_MAX - 1);
    }
    Path[CONQUER_PATH_MAX - 1] = FACING_NONE;
  }

  IsNewNavCom = false;
  TrackIndex = 0;
  if (!Start_Driver(dest)) {
    TrackNumber = -1;
    Path[0] = FACING_NONE;
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
    if (TrackNumber == -1 && (Target_Legal(NavCom) || Path[0] != FACING_NONE)) {
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
         (Class->Speed == SPEED_WHEEL && !Special.IsThreePoint)) &&
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
      if (Mission != MISSION_GUARD || NavCom != kTargetNone) {
        if (Target_Legal(NavCom) || Path[0] != FACING_NONE) {
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
  FacingType stage[6] = {FACING_N, FACING_N, FACING_N, FACING_N,
                         FACING_N, FACING_N};  // Prefix path elements.
  int facediff;  // The facing difference value (0..4 | 0..-4).
  static FacingType _path[4][6] = {
      {static_cast<FacingType>(2), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(0),
       static_cast<FacingType>(0), static_cast<FacingType>(0)},
      {static_cast<FacingType>(3), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(0), static_cast<FacingType>(0)},
      {static_cast<FacingType>(4), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(0), static_cast<FacingType>(0)},
      {static_cast<FacingType>(4), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(1), static_cast<FacingType>(0)}};
  static FacingType _dpath[4][6] = {
      {static_cast<FacingType>(0), static_cast<FacingType>(0),
       static_cast<FacingType>(0), static_cast<FacingType>(0),
       static_cast<FacingType>(0), static_cast<FacingType>(0)},
      {static_cast<FacingType>(3), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(0), static_cast<FacingType>(0)},
      {static_cast<FacingType>(4), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(1), static_cast<FacingType>(0)},
      {static_cast<FacingType>(5), static_cast<FacingType>(0),
       static_cast<FacingType>(2), static_cast<FacingType>(2),
       static_cast<FacingType>(1), static_cast<FacingType>(0)}};

  int index;
  int counter;          // Path addition
  FacingType* ptr;      // Path list pointer.
  FacingType* ptr2;     // Copy of new path list pointer.
  FacingType nextpath;  // Next path value.
  CELL cell;            // Working cell value.
  bool ok;

  /*
  **	Verify that the unit is valid and there is a path problem to resolve.
  */
  if (!path || path->Command[0] == FACING_NONE) {
    return;
  }

  /*
  **	Only wheeled vehicles need a path fixup -- to avoid 3 point turns.
  */
  if (!Special.IsThreePoint || Class->Speed != SPEED_WHEEL) {
    return;
  }

  /*
  **	If the original path starts in the same direction as the unit, then
  **	there is no problem to resolve -- abort.
  */
  facediff =
      PrimaryFacing.Difference(static_cast<DirType>(path->Command[0] << 5)) >>
      5;

  if (!facediff) {
    return;
  }

  if (Dir_Facing(PrimaryFacing) & FACING_NE) {
    ptr = &_dpath[static_cast<FacingType>(std::abs(facediff)) - FACING_NE]
                 [1];  // Pointer to path adjust list.
    counter =
        static_cast<int>(_dpath[(FacingType)std::abs(facediff) - FACING_NE]
                               [0]);  // Number of path adjusts.
  } else {
    ptr = &_path[static_cast<FacingType>(std::abs(facediff)) - FACING_NE]
                [1];  // Pointer to path adjust list.
    counter = static_cast<int>(_path[(FacingType)std::abs(facediff) - FACING_NE]
                                    [0]);  // Number of path adjusts.
  }
  ptr2 = ptr;

  ok = true;                             // Presume adjustment is all ok.
  cell = Coord_Cell(Coord);              // Starting cell.
  nextpath = Dir_Facing(PrimaryFacing);  // Starting path.
  for (index = 0; index < counter; index++) {
    /*
    **	Determine next path element and add it to the
    **	working path list.
    */
    if (facediff > 0) {
      nextpath = nextpath + *ptr++;
    } else {
      nextpath = nextpath - *ptr++;
    }
    stage[index] = nextpath;
    cell = Adjacent_Cell(cell, nextpath);
    // cell = Coord_Cell(Adjacent_Cell(Cell_Coord(cell), nextpath));

    /*
    **	If it can't enter this cell, then abort the path
    **	building operation without adjusting the unit's
    **	path.
    */
    if (Can_Enter_Cell(cell, nextpath) != MOVE_OK) {
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
    cell = Coord_Cell(Coord);              // Starting cell.
    nextpath = Dir_Facing(PrimaryFacing);  // Starting path.
    for (index = 0; index < counter; index++) {
      /*
      **	Determine next path element and add it to the
      **	working path list.
      */
      if (facediff > 0) {
        nextpath = nextpath + *ptr++;
      } else {
        nextpath = nextpath - *ptr++;
      }
      stage[index] = nextpath;
      cell = Coord_Cell(Adjacent_Cell(Cell_Coord(cell), nextpath));

      /*
      **	If it can't enter this cell, then abort the path
      **	building operation without adjusting the unit's
      **	path.
      */
      if (Can_Enter_Cell(cell, nextpath) != MOVE_OK) {
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
      memmove(path->Command, &stage[0], std::max(counter, 1));
      path->Length = counter;
    } else {
      /*
      **	Optimize the transition path step from the smooth turn
      **	first part as it joins with the rest of the normal
      **	path. The normal prefix path steps are NOT to be optimized.
      */
      if (counter) {
        counter--;
        path->Command[0] = stage[counter];
        Optimize_Moves(path, MOVE_OK);
      }

      /*
      **	If there is more than one prefix path element, then
      **	insert the rest now.
      */
      if (counter) {
        memmove(&path->Command[counter], &path->Command[0], 40 - counter);
        memmove(&path->Command[0], &stage[0], counter);
        path->Length += counter;
      }
    }
    path->Command[path->Length] = FACING_NONE;
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

  Icon_Install(Coord_Cell(Coord), _trackdirs[Facing_To_8(BodyFacing)]);
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
void DriveClass::Mark_Track(COORDINATE headto, MarkType type) {
  int value;

  value = type != MARK_UP;

  if (headto) {
    if (!IsOnShortTrack && TrackNumber != -1) {
      /*
      ** If we have not passed the per cell process point we need
      ** to deal with it.
      */
      int tracknum = TrackControl[TrackNumber].Track;
      if (tracknum) {
        const TrackType* ptr = RawTracks[tracknum - 1].Track;
        int cellidx = RawTracks[tracknum - 1].Cell;
        if (cellidx > -1) {
          DirType dir = ptr[cellidx].Facing;

          if (TrackIndex < cellidx && cellidx != -1) {
            COORDINATE offset = Smooth_Turn(ptr[cellidx].Offset, &dir);
            Map[Coord_Cell(offset)].Flag.Occupy.Vehicle = value;
          }
        }
      }
    }
    Map[Coord_Cell(headto)].Flag.Occupy.Vehicle = value;
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
      return UnitTypeClass::FULL_LOAD_CREDITS / UnitTypeClass::STEP_COUNT;
    }
    return UnitTypeClass::FULL_LOAD_CREDITS +
           UnitTypeClass::FULL_LOAD_CREDITS / 3 / UnitTypeClass::STEP_COUNT;
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
bool DriveClass::Ok_To_Move(DirType) const { return true; }

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
    {0x00F50000L, static_cast<DirType>(0)},
    {0x00EA0000L, static_cast<DirType>(0)},
    {0x00DF0000L, static_cast<DirType>(0)},
    {0x00D40000L, static_cast<DirType>(0)},
    {0x00C90000L, static_cast<DirType>(0)},
    {0x00BE0000L, static_cast<DirType>(0)},
    {0x00B30000L, static_cast<DirType>(0)},
    {0x00A80000L, static_cast<DirType>(0)},
    {0x009D0000L, static_cast<DirType>(0)},
    {0x00920000L, static_cast<DirType>(0)},
    {0x00870000L, static_cast<DirType>(0)},
    {0x007C0000L, static_cast<DirType>(0)},  // Track jump check here.
    {0x00710000L, static_cast<DirType>(0)},
    {0x00660000L, static_cast<DirType>(0)},
    {0x005B0000L, static_cast<DirType>(0)},
    {0x00500000L, static_cast<DirType>(0)},
    {0x00450000L, static_cast<DirType>(0)},
    {0x003A0000L, static_cast<DirType>(0)},
    {0x002F0000L, static_cast<DirType>(0)},
    {0x00240000L, static_cast<DirType>(0)},
    {0x00190000L, static_cast<DirType>(0)},
    {0x000E0000L, static_cast<DirType>(0)},
    {0x00030000L, static_cast<DirType>(0)},
    {0x00000000L, static_cast<DirType>(0)}};

const DriveClass::TrackType DriveClass::Track2[] = {
    {0x00F8FF08L, static_cast<DirType>(32)},
    {0x00F0FF10L, static_cast<DirType>(32)},
    {0x00E8FF18L, static_cast<DirType>(32)},
    {0x00E0FF20L, static_cast<DirType>(32)},
    {0x00D8FF28L, static_cast<DirType>(32)},
    {0x00D0FF30L, static_cast<DirType>(32)},
    {0x00C8FF38L, static_cast<DirType>(32)},
    {0x00C0FF40L, static_cast<DirType>(32)},
    {0x00B8FF48L, static_cast<DirType>(32)},
    {0x00B0FF50L, static_cast<DirType>(32)},
    {0x00A8FF58L, static_cast<DirType>(32)},
    {0x00A0FF60L, static_cast<DirType>(32)},
    {0x0098FF68L, static_cast<DirType>(32)},
    {0x0090FF70L, static_cast<DirType>(32)},
    {0x0088FF78L, static_cast<DirType>(32)},
    {0x0080FF80L, static_cast<DirType>(32)},  // Track jump check here.
    {0x0078FF88L, static_cast<DirType>(32)},
    {0x0070FF90L, static_cast<DirType>(32)},
    {0x0068FF98L, static_cast<DirType>(32)},
    {0x0060FFA0L, static_cast<DirType>(32)},
    {0x0058FFA8L, static_cast<DirType>(32)},
    {0x0050FFB0L, static_cast<DirType>(32)},
    {0x0048FFB8L, static_cast<DirType>(32)},
    {0x0040FFC0L, static_cast<DirType>(32)},
    {0x0038FFC8L, static_cast<DirType>(32)},
    {0x0030FFD0L, static_cast<DirType>(32)},
    {0x0028FFD8L, static_cast<DirType>(32)},
    {0x0020FFE0L, static_cast<DirType>(32)},
    {0x0018FFE8L, static_cast<DirType>(32)},
    {0x0010FFF0L, static_cast<DirType>(32)},
    {0x0008FFF8L, static_cast<DirType>(32)},
    {0x00000000L, static_cast<DirType>(32)}};

const DriveClass::TrackType DriveClass::Track3[] = {
    {0x01F5FF00L, static_cast<DirType>(0)},
    {0x01EAFF00L, static_cast<DirType>(0)},
    {0x01DFFF00L, static_cast<DirType>(0)},
    {0x01D4FF00L, static_cast<DirType>(0)},
    {0x01C9FF00L, static_cast<DirType>(0)},
    {0x01BEFF00L, static_cast<DirType>(0)},
    {0x01B3FF00L, static_cast<DirType>(0)},
    {0x01A8FF00L, static_cast<DirType>(0)},
    {0x019DFF00L, static_cast<DirType>(0)},
    {0x0192FF00L, static_cast<DirType>(0)},
    {0x0187FF00L, static_cast<DirType>(0)},
    {0x0180FF00L, static_cast<DirType>(0)},
    {0x0175FF00L, static_cast<DirType>(0)},  // Jump entry point here.
    {0x016BFF00L, static_cast<DirType>(0)},
    {0x0160FF02L, static_cast<DirType>(1)},
    {0x0155FF04L, static_cast<DirType>(3)},
    {0x014CFF06L, static_cast<DirType>(4)},
    {0x0141FF08L, static_cast<DirType>(5)},
    {0x0137FF0BL, static_cast<DirType>(7)},
    {0x012EFF0FL, static_cast<DirType>(8)},
    {0x0124FF13L, static_cast<DirType>(9)},
    {0x011AFF17L, static_cast<DirType>(11)},
    {0x0110FF1BL, static_cast<DirType>(12)},
    {0x0107FF1FL, static_cast<DirType>(13)},  // Center cell processing here.
    {0x00FCFF24L, static_cast<DirType>(15)},
    {0x00F3FF28L, static_cast<DirType>(16)},
    {0x00ECFF2CL, static_cast<DirType>(17)},
    {0x00E0FF32L, static_cast<DirType>(19)},
    {0x00D7FF36L, static_cast<DirType>(20)},
    {0x00CFFF3DL, static_cast<DirType>(21)},
    {0x00C6FF42L, static_cast<DirType>(23)},
    {0x00BAFF49L, static_cast<DirType>(24)},
    {0x00B0FF4DL, static_cast<DirType>(25)},
    {0x00A8FF58L, static_cast<DirType>(27)},
    {0x00A0FF60L, static_cast<DirType>(28)},
    {0x0098FF68L, static_cast<DirType>(29)},
    {0x0090FF70L, static_cast<DirType>(31)},
    {0x0088FF78L, static_cast<DirType>(32)},
    {0x0080FF80L, static_cast<DirType>(32)},  // Track jump check here.
    {0x0078FF88L, static_cast<DirType>(32)},
    {0x0070FF90L, static_cast<DirType>(32)},
    {0x0068FF98L, static_cast<DirType>(32)},
    {0x0060FFA0L, static_cast<DirType>(32)},
    {0x0058FFA8L, static_cast<DirType>(32)},
    {0x0050FFB0L, static_cast<DirType>(32)},
    {0x0048FFB8L, static_cast<DirType>(32)},
    {0x0040FFC0L, static_cast<DirType>(32)},
    {0x0038FFC8L, static_cast<DirType>(32)},
    {0x0030FFD0L, static_cast<DirType>(32)},
    {0x0028FFD8L, static_cast<DirType>(32)},
    {0x0020FFE0L, static_cast<DirType>(32)},
    {0x0018FFE8L, static_cast<DirType>(32)},
    {0x0010FFF0L, static_cast<DirType>(32)},
    {0x0008FFF8L, static_cast<DirType>(32)},
    {0x00000000L, static_cast<DirType>(32)}};

const DriveClass::TrackType DriveClass::Track4[] = {
    {0x00F5FF00L, static_cast<DirType>(0)},
    {0x00EBFF00L, static_cast<DirType>(0)},
    {0x00E0FF00L, static_cast<DirType>(0)},
    {0x00D5FF00L, static_cast<DirType>(0)},
    {0x00CBFF01L, static_cast<DirType>(0)},
    {0x00C0FF03L, static_cast<DirType>(0)},
    {0x00B5FF05L, static_cast<DirType>(1)},
    {0x00ABFF07L, static_cast<DirType>(1)},
    {0x00A0FF0AL, static_cast<DirType>(2)},
    {0x0095FF0DL, static_cast<DirType>(3)},
    {0x008BFF10L, static_cast<DirType>(4)},
    {0x0080FF14L, static_cast<DirType>(5)},  // Track entry here.
    {0x0075FF18L, static_cast<DirType>(8)},
    {0x006DFF1CL, static_cast<DirType>(12)},
    {0x0063FF22L, static_cast<DirType>(16)},
    {0x005AFF25L, static_cast<DirType>(20)},
    {0x0052FF2BL, static_cast<DirType>(23)},
    {0x0048FF32L, static_cast<DirType>(27)},
    {0x0040FF37L, static_cast<DirType>(32)},
    {0x0038FF3DL, static_cast<DirType>(36)},
    {0x0030FF46L, static_cast<DirType>(39)},
    {0x002BFF4FL, static_cast<DirType>(43)},
    {0x0024FF58L, static_cast<DirType>(47)},
    {0x0020FF60L, static_cast<DirType>(51)},
    {0x001BFF6DL, static_cast<DirType>(54)},
    {0x0017FF79L, static_cast<DirType>(57)},
    {0x0014FF82L, static_cast<DirType>(60)},  // Track jump here.
    {0x0011FF8FL, static_cast<DirType>(62)},
    {0x000DFF98L, static_cast<DirType>(63)},
    {0x0009FFA2L, static_cast<DirType>(64)},
    {0x0006FFACL, static_cast<DirType>(64)},
    {0x0004FFB5L, static_cast<DirType>(66)},
    {0x0003FFC0L, static_cast<DirType>(64)},
    {0x0002FFCBL, static_cast<DirType>(64)},
    {0x0001FFD5L, static_cast<DirType>(64)},
    {0x0000FFE0L, static_cast<DirType>(64)},
    {0x0000FFEBL, static_cast<DirType>(64)},
    {0x0000FFF5L, static_cast<DirType>(64)},
    {0x00000000L, static_cast<DirType>(64)}};

const DriveClass::TrackType DriveClass::Track5[] = {
    {0xFFF8FE08L, static_cast<DirType>(32)},
    {0xFFF0FE10L, static_cast<DirType>(32)},
    {0xFFE8FE18L, static_cast<DirType>(32)},
    {0xFFE0FE20L, static_cast<DirType>(32)},
    {0xFFD8FE28L, static_cast<DirType>(32)},
    {0xFFD0FE30L, static_cast<DirType>(32)},
    {0xFFC8FE38L, static_cast<DirType>(32)},
    {0xFFC0FE40L, static_cast<DirType>(32)},
    {0xFFB8FE48L, static_cast<DirType>(32)},
    {0xFFB0FE50L, static_cast<DirType>(32)},
    {0xFFA8FE58L, static_cast<DirType>(32)},
    {0xFFA0FE60L, static_cast<DirType>(32)},
    {0xFF98FE68L, static_cast<DirType>(32)},
    {0xFF90FE70L, static_cast<DirType>(32)},
    {0xFF88FE78L, static_cast<DirType>(32)},
    {0xFF80FE80L, static_cast<DirType>(32)},  // Track entry here.
    {0xFF78FE88L, static_cast<DirType>(32)},
    {0xFF71FE90L, static_cast<DirType>(32)},
    {0xFF6AFE97L, static_cast<DirType>(32)},
    {0xFF62FE9FL, static_cast<DirType>(32)},
    {0xFF5AFEA8L, static_cast<DirType>(32)},
    {0xFF53FEB0L, static_cast<DirType>(35)},
    {0xFF4BFEB7L, static_cast<DirType>(38)},
    {0xFF44FEBEL, static_cast<DirType>(41)},
    {0xFF3EFEC4L, static_cast<DirType>(44)},
    {0xFF39FECEL, static_cast<DirType>(47)},
    {0xFF34FED8L, static_cast<DirType>(50)},
    {0xFF30FEE0L, static_cast<DirType>(53)},
    {0xFF2DFEEBL, static_cast<DirType>(56)},
    {0xFF2CFEF5L, static_cast<DirType>(59)},
    {0xFF2BFF00L, static_cast<DirType>(62)},
    {0xFF2CFF0BL, static_cast<DirType>(66)},
    {0xFF2DFF15L, static_cast<DirType>(69)},
    {0xFF30FF1FL, static_cast<DirType>(72)},
    {0xFF34FF28L, static_cast<DirType>(75)},
    {0xFF39FF30L, static_cast<DirType>(78)},
    {0xFF3EFF3AL, static_cast<DirType>(81)},
    {0xFF44FF44L, static_cast<DirType>(84)},
    {0xFF4BFF4BL, static_cast<DirType>(87)},
    {0xFF53FF50L, static_cast<DirType>(90)},
    {0xFF5AFF58L, static_cast<DirType>(93)},
    {0xFF62FF60L, static_cast<DirType>(96)},
    {0xFF6AFF68L, static_cast<DirType>(96)},
    {0xFF71FF70L, static_cast<DirType>(96)},
    {0xFF78FF78L, static_cast<DirType>(96)},
    {0xFF80FF80L, static_cast<DirType>(96)},  // Track jump check here.
    {0xFF88FF88L, static_cast<DirType>(96)},
    {0xFF90FF90L, static_cast<DirType>(96)},
    {0xFF98FF98L, static_cast<DirType>(96)},
    {0xFFA0FFA0L, static_cast<DirType>(96)},
    {0xFFA8FFA8L, static_cast<DirType>(96)},
    {0xFFB0FFB0L, static_cast<DirType>(96)},
    {0xFFB8FFB8L, static_cast<DirType>(96)},
    {0xFFC0FFC0L, static_cast<DirType>(96)},
    {0xFFC8FFC8L, static_cast<DirType>(96)},
    {0xFFD0FFD0L, static_cast<DirType>(96)},
    {0xFFD8FFD8L, static_cast<DirType>(96)},
    {0xFFE0FFE0L, static_cast<DirType>(96)},
    {0xFFE8FFE8L, static_cast<DirType>(96)},
    {0xFFF0FFF0L, static_cast<DirType>(96)},
    {0xFFF8FFF8L, static_cast<DirType>(96)},
    {0x00000000L, static_cast<DirType>(96)}};

const DriveClass::TrackType DriveClass::Track6[] = {
    {0x0100FE00L, static_cast<DirType>(32)},
    {0x00F8FE08L, static_cast<DirType>(32)},
    {0x00F0FE10L, static_cast<DirType>(32)},
    {0x00E8FE18L, static_cast<DirType>(32)},
    {0x00E0FE20L, static_cast<DirType>(32)},
    {0x00D8FE28L, static_cast<DirType>(32)},
    {0x00D0FE30L, static_cast<DirType>(32)},
    {0x00C8FE38L, static_cast<DirType>(32)},
    {0x00C0FE40L, static_cast<DirType>(32)},
    {0x00B8FE48L, static_cast<DirType>(32)},
    {0x00B0FE50L, static_cast<DirType>(32)},
    {0x00A8FE58L, static_cast<DirType>(32)},
    {0x00A0FE60L, static_cast<DirType>(32)},
    {0x0098FE68L, static_cast<DirType>(32)},
    {0x0090FE70L, static_cast<DirType>(32)},
    {0x0088FE78L, static_cast<DirType>(32)},
    {0x0080FE80L, static_cast<DirType>(32)},  // Jump entry point here.
    {0x0078FE88L, static_cast<DirType>(32)},
    {0x0070FE90L, static_cast<DirType>(32)},
    {0x0068FE98L, static_cast<DirType>(32)},
    {0x0060FEA0L, static_cast<DirType>(32)},
    {0x0058FEA8L, static_cast<DirType>(32)},
    {0x0055FEAEL, static_cast<DirType>(32)},
    {0x004EFEB8L, static_cast<DirType>(35)},
    {0x0048FEC0L, static_cast<DirType>(37)},
    {0x0042FEC9L, static_cast<DirType>(40)},
    {0x003BFED2L, static_cast<DirType>(43)},
    {0x0037FEDAL, static_cast<DirType>(45)},
    {0x0032FEE3L, static_cast<DirType>(48)},
    {0x002BFEEBL, static_cast<DirType>(51)},
    {0x0026FEF5L, static_cast<DirType>(53)},
    {0x0022FEFEL, static_cast<DirType>(56)},
    {0x001CFF08L, static_cast<DirType>(59)},
    {0x0019FF12L, static_cast<DirType>(61)},
    {0x0015FF1BL, static_cast<DirType>(64)},
    {0x0011FF26L, static_cast<DirType>(64)},
    {0x000EFF30L, static_cast<DirType>(64)},
    {0x000BFF39L, static_cast<DirType>(64)},
    {0x0009FF43L, static_cast<DirType>(64)},
    {0x0007FF4EL, static_cast<DirType>(64)},
    {0x0005FF57L, static_cast<DirType>(64)},
    {0x0003FF62L, static_cast<DirType>(64)},
    {0x0001FF6DL, static_cast<DirType>(64)},
    {0x0000FF77L, static_cast<DirType>(64)},
    {0x0000FF80L, static_cast<DirType>(64)},  // Track jump check here.
    {0x0000FF8BL, static_cast<DirType>(64)},
    {0x0000FF95L, static_cast<DirType>(64)},
    {0x0000FFA0L, static_cast<DirType>(64)},
    {0x0000FFABL, static_cast<DirType>(64)},
    {0x0000FFB5L, static_cast<DirType>(64)},
    {0x0000FFC0L, static_cast<DirType>(64)},
    {0x0000FFCBL, static_cast<DirType>(64)},
    {0x0000FFD5L, static_cast<DirType>(64)},
    {0x0000FFE0L, static_cast<DirType>(64)},
    {0x0000FFEBL, static_cast<DirType>(64)},
    {0x0000FFF5L, static_cast<DirType>(64)},
    {0x00000000L, static_cast<DirType>(64)}};

const DriveClass::TrackType DriveClass::Track7[] = {
    {0x0006FFFFL, static_cast<DirType>(0)},
    {0x000CFFFEL, static_cast<DirType>(4)},
    {0x0011FFFCL, static_cast<DirType>(8)},
    {0x0018FFFAL, static_cast<DirType>(12)},
    {0x001FFFF6L, static_cast<DirType>(16)},
    {0x0024FFF3L, static_cast<DirType>(19)},
    {0x002BFFF0L, static_cast<DirType>(22)},
    {0x0030FFFDL, static_cast<DirType>(23)},
    {0x0035FFEBL, static_cast<DirType>(24)},
    {0x0038FFE8L, static_cast<DirType>(25)},
    {0x003CFFE6L, static_cast<DirType>(26)},
    {0x0040FFE3L, static_cast<DirType>(27)},
    {0x0043FFE0L, static_cast<DirType>(28)},
    {0x0046FFDDL, static_cast<DirType>(29)},
    {0x0043FFDFL, static_cast<DirType>(30)},
    {0x0040FFE1L, static_cast<DirType>(30)},
    {0x003CFFE3L, static_cast<DirType>(30)},
    {0x0038FFE5L, static_cast<DirType>(30)},
    {0x0035FFE7L, static_cast<DirType>(31)},
    {0x0030FFE9L, static_cast<DirType>(31)},
    {0x002BFFEBL, static_cast<DirType>(31)},
    {0x0024FFEDL, static_cast<DirType>(31)},
    {0x001FFFF1L, static_cast<DirType>(31)},
    {0x0018FFF4L, static_cast<DirType>(32)},
    {0x0011FFF7L, static_cast<DirType>(32)},
    {0x000CFFFAL, static_cast<DirType>(32)},
    {0x0006FFFDL, static_cast<DirType>(32)},
    {0x00000000L, static_cast<DirType>(32)}};

const DriveClass::TrackType DriveClass::Track8[] = {
    {0x0003FFFCL, static_cast<DirType>(32)},
    {0x0006FFF7L, static_cast<DirType>(36)},
    {0x000AFFF1L, static_cast<DirType>(40)},
    {0x000CFFEBL, static_cast<DirType>(44)},
    {0x000DFFE4L, static_cast<DirType>(46)},
    {0x000EFFDCL, static_cast<DirType>(48)},
    {0x000FFFD5L, static_cast<DirType>(50)},
    {0x0010FFD0L, static_cast<DirType>(52)},
    {0x0011FFC9L, static_cast<DirType>(54)},
    {0x0012FFC2L, static_cast<DirType>(56)},
    {0x0011FFC0L, static_cast<DirType>(58)},
    {0x0010FFC2L, static_cast<DirType>(60)},
    {0x000EFFC9L, static_cast<DirType>(62)},
    {0x000CFFCFL, static_cast<DirType>(64)},
    {0x000AFFD5L, static_cast<DirType>(64)},
    {0x0008FFDAL, static_cast<DirType>(64)},
    {0x0006FFE2L, static_cast<DirType>(64)},
    {0x0004FFE9L, static_cast<DirType>(64)},
    {0x0002FFEFL, static_cast<DirType>(64)},
    {0x0001FFF5L, static_cast<DirType>(64)},
    {0x0000FFF9L, static_cast<DirType>(64)},
    {0x00000000L, static_cast<DirType>(64)}};

const DriveClass::TrackType DriveClass::Track9[] = {
    {0xFFF50002L, static_cast<DirType>(0)},
    {0xFFEB0004L, static_cast<DirType>(2)},
    {0xFFE00006L, static_cast<DirType>(4)},
    {0xFFD50009L, static_cast<DirType>(6)},
    {0xFFCE000CL, static_cast<DirType>(9)},
    {0xFFC8000FL, static_cast<DirType>(11)},
    {0xFFC00012L, static_cast<DirType>(13)},
    {0xFFB80015L, static_cast<DirType>(16)},
    {0xFFC00012L, static_cast<DirType>(18)},
    {0xFFC8000EL, static_cast<DirType>(20)},
    {0xFFCE000AL, static_cast<DirType>(22)},
    {0xFFD50004L, static_cast<DirType>(24)},
    {0xFFDE0000L, static_cast<DirType>(26)},
    {0xFFE9FFF8L, static_cast<DirType>(28)},
    {0xFFEEFFF2L, static_cast<DirType>(30)},
    {0xFFF5FFEBL, static_cast<DirType>(32)},
    {0xFFFDFFE1L, static_cast<DirType>(34)},
    {0x0002FFD8L, static_cast<DirType>(36)},
    {0x0007FFD2L, static_cast<DirType>(39)},
    {0x000BFFCBL, static_cast<DirType>(41)},
    {0x0010FFC5L, static_cast<DirType>(43)},
    {0x0013FFBEL, static_cast<DirType>(45)},
    {0x0015FFB7L, static_cast<DirType>(48)},
    {0x0013FFBEL, static_cast<DirType>(50)},
    {0x0011FFC5L, static_cast<DirType>(52)},
    {0x000BFFCCL, static_cast<DirType>(54)},
    {0x0008FFD4L, static_cast<DirType>(56)},
    {0x0005FFDFL, static_cast<DirType>(58)},
    {0x0003FFEBL, static_cast<DirType>(62)},
    {0x0001FFF5L, static_cast<DirType>(64)},
    {0x00000000L, static_cast<DirType>(64)}};

const DriveClass::TrackType DriveClass::Track10[] = {
    {0xFFF6000BL, static_cast<DirType>(32)},
    {0xFFF00015L, static_cast<DirType>(37)},
    {0xFFEB0020L, static_cast<DirType>(42)},
    {0xFFE9002BL, static_cast<DirType>(47)},
    {0xFFE50032L, static_cast<DirType>(52)},
    {0xFFE30038L, static_cast<DirType>(57)},
    {0xFFE00040L, static_cast<DirType>(60)},
    {0xFFE20038L, static_cast<DirType>(62)},
    {0xFFE40032L, static_cast<DirType>(64)},
    {0xFFE5002AL, static_cast<DirType>(68)},
    {0xFFE6001EL, static_cast<DirType>(70)},
    {0xFFE70015L, static_cast<DirType>(72)},
    {0xFFE8000BL, static_cast<DirType>(74)},
    {0xFFE90000L, static_cast<DirType>(76)},
    {0xFFE8FFF5L, static_cast<DirType>(78)},
    {0xFFE7FFEBL, static_cast<DirType>(80)},
    {0xFFE6FFE0L, static_cast<DirType>(82)},
    {0xFFE5FFD5L, static_cast<DirType>(84)},
    {0xFFE4FFCEL, static_cast<DirType>(86)},
    {0xFFE2FFC5L, static_cast<DirType>(88)},
    {0xFFE0FFC0L, static_cast<DirType>(90)},
    {0xFFE3FFC5L, static_cast<DirType>(92)},
    {0xFFE5FFCEL, static_cast<DirType>(94)},
    {0xFFE9FFD5L, static_cast<DirType>(95)},
    {0xFFEBFFE0L, static_cast<DirType>(96)},
    {0xFFF0FFEBL, static_cast<DirType>(96)},
    {0xFFF6FFF5L, static_cast<DirType>(96)},
    {0x00000000L, static_cast<DirType>(96)}};

const DriveClass::TrackType DriveClass::Track11[] = {
    {0x01000000L, DIR_SW},    {0x00F30008L, DIR_SW},
    {0x00E50010L, DIR_SW_X1}, {0x00D60018L, DIR_SW_X1},
    {0x00C80020L, DIR_SW_X1}, {0x00B90028L, DIR_SW_X1},
    {0x00AB0030L, DIR_SW_X2}, {0x009C0038L, DIR_SW_X2},
    {0x008D0040L, DIR_SW_X2}, {0x007F0048L, DIR_SW_X2},
    {0x00710050L, DIR_SW_X2}, {0x00640058L, DIR_SW_X2},
    {0x00550060L, DIR_SW_X2},

    {0x00000000L, DIR_SW_X2}};

const DriveClass::TrackType DriveClass::Track12[] = {
    {0xFF550060L, DIR_SW_X2}, {0xFF640058L, DIR_SW_X2},
    {0xFF710050L, DIR_SW_X2}, {0xFF7F0048L, DIR_SW_X2},
    {0xFF8D0040L, DIR_SW_X2}, {0xFF9C0038L, DIR_SW_X2},
    {0xFFAB0030L, DIR_SW_X2}, {0xFFB90028L, DIR_SW_X1},
    {0xFFC80020L, DIR_SW_X1}, {0xFFD60018L, DIR_SW_X1},
    {0xFFE50010L, DIR_SW_X1}, {0xFFF30008L, DIR_SW},

    {0x00000000L, DIR_SW}};

/*
**	Drive out of weapon's factory.
*/
const DriveClass::TrackType DriveClass::Track13[] = {
    {XYP_COORD(10, -21), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(10, -21), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(10, -20), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(10, -20), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(9, -18), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(9, -18), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(9, -17), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(8, -16), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(8, -15), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(7, -14), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(7, -13), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(6, -12), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(6, -11), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(5, -10), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(5, -9), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(4, -8), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(4, -7), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(3, -6), static_cast<DirType>(DIR_SW - 10)},
    {XYP_COORD(3, -5), static_cast<DirType>(DIR_SW - 9)},
    {XYP_COORD(2, -4), static_cast<DirType>(DIR_SW - 7)},
    {XYP_COORD(2, -3), static_cast<DirType>(DIR_SW - 5)},
    {XYP_COORD(1, -2), static_cast<DirType>(DIR_SW - 3)},
    {XYP_COORD(1, -1), static_cast<DirType>(DIR_SW - 1)},

    {0x00000000L, DIR_SW}};

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
    {1, 0, DIR_N, F_},                         //	0-0
    {3, 7, DIR_NE, F_D},                       //	0-1 (raw chart)
    {4, 9, DIR_E, F_D},                        //	0-2 (raw chart)
    {0, 0, DIR_SE, F_},                        //	0-3 !
    {0, 0, DIR_S, F_},                         //	0-4 !
    {0, 0, DIR_SW, F_},                        //	0-5 !
    {4, 9, DIR_W, (F_X | F_D)},                //	0-6
    {3, 7, DIR_NW, (F_X | F_D)},               //	0-7
    {6, 8, DIR_N, (F_T | F_X | F_Y | F_D)},    //	1-0
    {2, 0, DIR_NE, F_},                        //	1-1 (raw chart)
    {6, 8, DIR_E, F_D},                        //	1-2 (raw chart)
    {5, 10, DIR_SE, F_D},                      //	1-3 (raw chart)
    {0, 0, DIR_S, F_},                         //	1-4 !
    {0, 0, DIR_SW, F_},                        //	1-5 !
    {0, 0, DIR_W, F_},                         //	1-6 !
    {5, 10, DIR_NW, (F_T | F_X | F_Y | F_D)},  //	1-7
    {4, 9, DIR_N, (F_T | F_X | F_Y | F_D)},    //	2-0
    {3, 7, DIR_NE, (F_T | F_X | F_Y | F_D)},   //	2-1
    {1, 0, DIR_E, (F_T | F_X)},                //	2-2
    {3, 7, DIR_SE, (F_T | F_X | F_D)},         //	2-3
    {4, 9, DIR_S, (F_T | F_X | F_D)},          //	2-4
    {0, 0, DIR_SW, F_},                        //	2-5 !
    {0, 0, DIR_W, F_},                         //	2-6 !
    {0, 0, DIR_NW, F_},                        //	2-7 !
    {0, 0, DIR_N, F_},                         //	3-0 !
    {5, 10, DIR_NE, (F_Y | F_D)},              //	3-1
    {6, 8, DIR_E, (F_Y | F_D)},                //	3-2
    {2, 0, DIR_SE, F_Y},                       //	3-3
    {6, 8, DIR_S, (F_T | F_X | F_D)},          //	3-4
    {5, 10, DIR_SW, (F_T | F_X | F_D)},        //	3-5
    {0, 0, DIR_W, F_},                         //	3-6 !
    {0, 0, DIR_NW, F_},                        //	3-7 !
    {0, 0, DIR_N, F_},                         //	4-0 !
    {0, 0, DIR_NE, F_},                        //	4-1 !
    {4, 9, DIR_E, (F_Y | F_D)},                //	4-2
    {3, 7, DIR_SE, (F_Y | F_D)},               //	4-3
    {1, 0, DIR_S, F_Y},                        //	4-4
    {3, 7, DIR_SW, (F_X | F_Y | F_D)},         //	4-5
    {4, 9, DIR_W, (F_X | F_Y | F_D)},          //	4-6
    {0, 0, DIR_NW, F_},                        //	4-7 !
    {0, 0, DIR_N, F_},                         //	5-0 !
    {0, 0, DIR_NE, F_},                        //	5-1 !
    {0, 0, DIR_E, F_},                         //	5-2 !
    {5, 10, DIR_SE, (F_T | F_D)},              //	5-3
    {6, 8, DIR_S, (F_T | F_D)},                //	5-4
    {2, 0, DIR_SW, F_T},                       //	5-5
    {6, 8, DIR_W, (F_X | F_Y | F_D)},          //	5-6
    {5, 10, DIR_NW, (F_X | F_Y | F_D)},        //	5-7
    {4, 9, DIR_N, (F_T | F_Y | F_D)},          //	6-0
    {0, 0, DIR_NE, F_},                        //	6-1 !
    {0, 0, DIR_E, F_},                         //	6-2 !
    {0, 0, DIR_SE, F_},                        //	6-3 !
    {4, 9, DIR_S, (F_T | F_D)},                //	6-4
    {3, 7, DIR_SW, (F_T | F_D)},               //	6-5
    {1, 0, DIR_W, F_T},                        //	6-6
    {3, 7, DIR_NW, (F_T | F_Y | F_D)},         //	6-7
    {6, 8, DIR_N, (F_T | F_Y | F_D)},          //	7-0
    {5, 10, DIR_NE, (F_T | F_Y | F_D)},        //	7-1
    {0, 0, DIR_E, F_},                         //	7-2 !
    {0, 0, DIR_SE, F_},                        //	7-3 !
    {0, 0, DIR_S, F_},                         //	7-4 !
    {5, 10, DIR_SW, (F_X | F_D)},              //	7-5
    {6, 8, DIR_W, (F_X | F_D)},                //	7-6
    {2, 0, DIR_NW, F_X},                       //	7-7

    {11, 11, DIR_SW, F_},     // Backup harvester into refinery.
    {12, 12, DIR_SW_X2, F_},  // Drive back into refinery.
    {13, 13, DIR_SW, F_}      // Drive out of weapons factory.
};
