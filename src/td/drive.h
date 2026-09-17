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

/* $Header:   F:\projects\c&c\vcs\code\drive.h_v   2.19   16 Oct 1995 16:47:44
 * JOE_BOSTIC  $ */
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

#ifndef CNC_RED_ALERT_TD_DRIVE_H_
#define CNC_RED_ALERT_TD_DRIVE_H_

#include "td/defines.h"
#include "td/foot.h"
#include "td/type.h"

/****************************************************************************
**	Movable objects are handled by this class definition. Moveable objects
**	cover everything except buildings.
*/
class DriveClass : public FootClass {
 public:
  // Field-wise state and checked references; load shells have no scenario effects.
  template <class Archive>
  void Serialize(Archive& ar);

  /*
  **	This points to the static control data that gives 'this' unit its
  *characteristics.
  */
  const UnitTypeClass* Class = nullptr;

  /*
  **	This records the number of "loads" of Tiberium the unit is carrying.
  *Only *	harvesters use this field.
  */
  unsigned char Tiberium = 0;

  /*
  **	If this unit performing harvesting action, then this flag is true. The
  *flag *	is located here because the other bit flags here give it a free
  *place to *	reside.
  */
  bool IsHarvesting : 1 = false;

  /*
  **	This flags when a transport vehicle could not unload at its designated
  *location *	and is heading off the map to try again later. When this flag is
  *true, the *	transport unit is allowed to disappear when it reaches the edge
  *of the map.
  */
  bool IsReturning : 1 = false;

  /*
  **	Some units must have their turret locked down to face their body
  *direction. *	When this flag is set, this condition is in effect. This flag is
  *a more *	accurate check than examining the TrackNumber since the turret
  *may be *	rotating into position so that a pending track may start. During
  *this process *	the track number does not indicate anything.
  */
  bool IsTurretLockedDown : 1 = false;

  /*
  **	This vehicle could be processing a "short track". A short track is one
  *that *	doesn't actually go anywhere. Kind of like turning in place.
  */
  bool IsOnShortTrack : 1 = false;

  /*---------------------------------------------------------------------
  **	Constructors, Destructors, and overloaded operators.
  */
  DriveClass();
  DriveClass(UnitType classid, HousesType house);
  ~DriveClass() override = default;
  DriveClass(const DriveClass&) = delete;
  DriveClass& operator=(const DriveClass&) = delete;
  DriveClass(DriveClass&&) = delete;
  DriveClass& operator=(DriveClass&&) = delete;
  // objects compare directly against their type ID.
  // NOLINTNEXTLINE(*-explicit-constructor)
  operator UnitType() const { return Class->Type; }

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  int Offload_Tiberium_Bail() override;
  void Do_Turn(DirType dir);
  void Approach_Target() override;
  const ObjectTypeClass& Class_Of() const override;
  virtual void Overrun_Square(CELL cell, bool threaten = true);
  void Assign_Destination(TARGET target) override;
  void Per_Cell_Process(bool center) override;
  virtual bool Ok_To_Move(DirType /*unused*/);
  void Fixup_Path(PathType* path) override;
  void AI() override;
  void Force_Track(int track, COORDINATE coord);
  int Tiberium_Load() const override;

  void Exit_Map();
  void Mark_Track(COORDINATE headto, MarkType type);
  /*
  **	File I/O.
  */

  /**********************************************************************
  **	These enumerations are used as working constants that exist only
  **	in the DriveClass namespace.
  */
  static constexpr int kBackupIntoRefinery =
      64;                                    // Track to backup into refinery.
  static constexpr int kOutOfRefinery = 65;  // Track to leave refinery.
  static constexpr int kOutOfWeaponFactory =
      66;  // Track to leave weapons factory.

 private:
  /****************************************************************************
  **	Smooth turning tracks are controlled by this structure and these
  **	processing bits.
  */
  enum class TrackControlType {
    F_ = 0x00,   // No translation necessary?
    F_T = 0x01,  // Transpose X and Y components?
    F_X = 0x02,  // Reverse X component sign?
    F_Y = 0x04,  // Reverse Y component sign?
    F_D = 0x08   // Two cell consumption?
  };
  using enum TrackControlType;
  // #define	F_S	0x10	// Is this a 90 degree turn?

  struct TurnTrackType {
    int Track;              // Which track to use.
    int StartTrack;         // Track when starting from stand-still.
    DirType Facing;         // Facing when track has been completed.
    TrackControlType Flag;  // List processing flag bits.
  };

  struct TrackType {
    COORDINATE Offset;  // Offset to origin coordinate.
    DirType Facing;     // Facing (primary track).
  };

  struct RawTrackType {
    std::span<const TrackType> Track;  // Pointer to track list.
    int Jump;                // Index where track jumping is allowed.
    int Entry;               // Entry point if jumping to this track.
    int Cell;                // Per cell process should occur at this index.
  };

  /*
  **	These speed values are used to accumulate movement and then
  **	convert them into pixel "steps" that are then translated through
  **	the currently running track so that the unit will move.
  */
  unsigned char SpeedAccum = 0;

  /*
  **	This the track control logic (used for ground vehicles only). The
  *'Track' *	variable holds the track being followed (0 == not following
  *track). The *	'TrackIndex' variable holds the current index into the
  *specified track *	(starts at 0).
  */
  int TrackNumber = -1;
  int TrackIndex = 0;

  /*---------------------------------------------------------------------
  **	Member function prototypes.
  */
  bool While_Moving();
  bool Start_Of_Move();
  void Lay_Track();
  COORDINATE Smooth_Turn(COORDINATE adj, DirType* dir);

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

class ArchiveReader;
class ArchiveWriter;
extern template void DriveClass::Serialize<ArchiveWriter>(ArchiveWriter&);
extern template void DriveClass::Serialize<ArchiveReader>(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_DRIVE_H_
