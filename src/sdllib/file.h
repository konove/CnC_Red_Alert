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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Library - Filio header stuff.            *
 *                                                                         *
 *                    File Name : FILE.H *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : September 13, 1993                       *
 *                                                                         *
 *                  Last Update : April 11, 1994
 **
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_FILE_H_
#define CNC_RED_ALERT_SDLLIB_FILE_H_

#include <cstddef>
#include <cstdint>
#include <ctime>

// file searching
struct FindFileState {
  const char* name;
  time_t mod_time;

  // internal state
  void* data;
  size_t offset;
};

bool Find_First_File(const char* path_glob, FindFileState& state);
bool Find_Next_File(FindFileState& state);
// to clean up if not finishing the search
void End_Find_File(FindFileState& state);

uint64_t Disk_Space_Available();

#endif  // CNC_RED_ALERT_SDLLIB_FILE_H_
