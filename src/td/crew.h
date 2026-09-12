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

/* $Header:   F:\projects\c&c\vcs\code\crew.h_v   2.18   16 Oct 1995 16:47:56
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CREW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : April 23, 1994 *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_CREW_H_
#define CNC_RED_ALERT_TD_CREW_H_

/****************************************************************************
**	This class handles the basic crew logic. This includes hero tracking,
**	crew bail-out, and attached object logic.
*/
class CrewClass {
 public:
  // Field-wise saved-game support.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(Kills);
  }

  /*
  **	This keeps track of the number of "kills" the unit as accumulated.
  **	When it reaches a certain point, the unit improves.
  */
  unsigned short Kills;

  /*
  **	Constructors, Destructors, and overloaded operators.
  */
  CrewClass() { Kills = 0; }

  // Increments the crew's kill tally and returns the new total.
  int Add_Kill() {
    Kills++;
    return Kills;
  }

 private:
};

#endif  // CNC_RED_ALERT_TD_CREW_H_
