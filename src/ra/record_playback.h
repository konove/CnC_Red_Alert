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
#ifndef CNC_RED_ALERT_RA_RECORD_PLAYBACK_H_
#define CNC_RED_ALERT_RA_RECORD_PLAYBACK_H_

// File: Recording and attract-mode playback of the view and selection state
// that the event stream does not carry.

// Saves or replays the parts of the game state that the event stream alone
// cannot reconstruct: where the map is scrolled to, which objects are selected,
// and any team or formation hotkey pressed this frame.
//
// The selection is written as a checksum followed by the target list. On
// playback the checksum says whether the current selection already matches; if
// it does, the objects are read but not selected again, which stops the unit
// acknowledgement voices from firing again on every frame.
void Do_Record_Playback();

// Notes, for the next Do_Record_Playback() while recording, that the player
// used a unit-group hotkey. team and action are Handle_Team()'s arguments.
void RecordTeamEvent(int team, int action);

// Notes, for the next Do_Record_Playback() while recording, that the player
// toggled formation.
void RecordFormationEvent();

// Discards any team or formation event not yet written, as when a new game
// starts.
void ResetRecordedEvents();

#endif  // CNC_RED_ALERT_RA_RECORD_PLAYBACK_H_
