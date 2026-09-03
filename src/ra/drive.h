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

/* $Header: /CounterStrike/DRIVE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DRIVE.H *
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

#ifndef CNC_RED_ALERT_RA_DRIVE_H_
#define CNC_RED_ALERT_RA_DRIVE_H_

#include "ra/defines.h"
#include "ra/face.h"
#include "ra/foot.h"
#include "ra/jshell.h"
#include "ra/monoc.h"
#include "tech/ftimer.h"
#include "tech/noinit.h"

/****************************************************************************
**	Movable objects are handled by this class definition. Moveable objects
**	cover everything except buildings.
*/
class DriveClass : public FootClass {
 public:
  /*
  **	If this unit performing harvesting action, then this flag is true. The
  *flag *	is located here because the other bit flags here give it a free
  *place to *	reside.
  */
  unsigned IsHarvesting : 1;

  /*
  ** This flag controls whether the unit has been moebius'd into a
  ** different location, and whether the MoebiusCountDown timer should be
  ** used to take him back where he belongs.
  */
  unsigned IsMoebius : 1;

  /*
  ** This controls how long a unit can exist in its alternate location
  ** before being pulled back by the chronosphere into its normal location.
  */
  Timer<FrameTickSource> MoebiusCountDown;

  /*
  ** This is the coord the unit will be taken back to once its moebius
  ** effect wears off.
  */
  CELL MoebiusCell;

  /*
  **	Some units must have their turret locked down to face their body
  *direction. *	When this flag is set, this condition is in effect. This flag is
  *a more *	accurate check than examining the TrackNumber since the turret
  *may be *	rotating into position so that a pending track may start. During
  *this process *	the track number does not indicate anything.
  */
  unsigned IsTurretLockedDown : 1;

  /*
  **	This vehicle could be processing a "short track". A short track is one
  *that *	doesn't actually go anywhere. Kind of like turning in place.
  */
  unsigned IsOnShortTrack : 1;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  DriveClass(RTTIType rtti, int id, HousesType house);
  DriveClass(const NoInitClass& x) : FootClass(x), MoebiusCountDown(x) {}
  ~DriveClass() override {}

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  bool Teleport_To(CELL cell);
  void Response_Select() override;
  void Response_Move() override;
  void Response_Attack() override;
  void Scatter(COORDINATE threat, bool forced = false,
               bool nokidding = false) override;
  bool Limbo() override;
  void Do_Turn(DirType dir);
  virtual void Overrun_Square(CELL, bool = true) {}
  void Assign_Destination(TARGET target) override;
  void Per_Cell_Process(PCPType why) override;
  virtual bool Ok_To_Move(DirType) const;
  void AI() override;
  void Debug_Dump(MonoClass* mono) const override;
  void Force_Track(int track, COORDINATE coord);
  bool Stop_Driver() override;

  void Mark_Track(COORDINATE headto, MarkType type);

  /**********************************************************************
  **	These enumerations are used as working constants that exist only
  **	in the DriveClass namespace.
  */
  enum DriveClassEnum {
    BACKUP_INTO_REFINERY = 64,  // Track to backup into refinery.
    OUT_OF_REFINERY,            // Track to leave refinery.
    OUT_OF_WEAPON_FACTORY       // Track to leave weapons factory.
  };

  /****************************************************************************
  **	Smooth turning tracks are controlled by this structure and these
  **	processing bits.
  */
  typedef enum TrackControlType {
    F_ = 0x00,   // No translation necessary?
    F_T = 0x01,  // Transpose X and Y components?
    F_X = 0x02,  // Reverse X component sign?
    F_Y = 0x04,  // Reverse Y component sign?
    F_D = 0x08   // Two cell consumption?
  } TrackControlType;

 private:
  typedef struct {
    int Track;              // Which track to use.
    int StartTrack;         // Track when starting from stand-still.
    DirType Facing;         // Facing when track has been completed.
    TrackControlType Flag;  // List processing flag bits.
  } TurnTrackType;

  typedef struct {
    COORDINATE Offset;  // Offset to origin coordinate.
    DirType Facing;     // Facing (primary track).
  } TrackType;

  typedef struct {
    const TrackType* Track;  // Pointer to track list.
    int Jump;                // Index where track jumping is allowed.
    int Entry;               // Entry point if jumping to this track.
    int Cell;                // Per cell process should occur at this index.
  } RawTrackType;

  /*
  **	These speed values are used to accumulate movement and then
  **	convert them into pixel "steps" that are then translated through
  **	the currently running track so that the unit will move.
  */
  int SpeedAccum;

  /*
  **	This the track control logic (used for ground vehicles only). The
  *'Track' *	variable holds the track being followed (0 == not following
  *track). The *	'TrackIndex' variable holds the current index into the
  *specified track *	(starts at 0).
  */
  int TrackNumber;
  int TrackIndex;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  bool While_Moving();
  bool Start_Of_Move();
  void Lay_Track();
  COORDINATE Smooth_Turn(COORDINATE adj, DirType& dir);

  static const TurnTrackType TrackControl[67];
  static const RawTrackType RawTracks[13];
  static const TrackType Track13[];
  static const TrackType Track12[];
  static const TrackType Track11[];
  static const TrackType Track10[];
  static const TrackType Track9[];
  static const TrackType Track8[];
  static const TrackType Track7[];
  static const TrackType Track6[];
  static const TrackType Track5[];
  static const TrackType Track4[];
  static const TrackType Track3[];
  static const TrackType Track2[];
  static const TrackType Track1[24];
};

#endif  // CNC_RED_ALERT_RA_DRIVE_H_
