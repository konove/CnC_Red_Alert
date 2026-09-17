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
#ifndef CNC_RED_ALERT_RA_SELECTION_H_
#define CNC_RED_ALERT_RA_SELECTION_H_

// File: The player's selection and numbered unit groups.

// Deselects every currently selected object.
void Unselect_All();

// Handles the player's numbered unit groups ("teams" here, unrelated to
// TeamClass). team is the group number, 0-9.
// action: 0 = select this group, replacing the selection if it holds
//             non-members,
//         1 = add this group to the current selection,
//         2 = make the currently selected objects into this group,
//         3 = as 0, and also center the map on the group.
void Handle_Team(int team, int action = 0);

// Puts the player's currently selected group into formation, or takes it out of
// one if it is already in formation.
//
// A formation is stored per unit as an offset from the group's centre, so that
// the group keeps its shape as it moves. kNoFormationOffset is the "not in
// formation" sentinel; finding it on the first member is what decides whether
// this call sets the formation up or tears it down.
void Toggle_Formation();

#endif  // CNC_RED_ALERT_RA_SELECTION_H_
