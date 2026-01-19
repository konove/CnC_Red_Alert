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
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Command & Conquer                        *
 *                                                                         *
 *                    File Name : UTRACKER.H                               *
 *                                                                         *
 *                   Programmer : Steve Tall                               *
 *                                                                         *
 *                   Start Date : June 3rd, 1996                           *
 *                                                                         *
 *                  Last Update : June 7th, 1996 [ST]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 *  The UnitTracker class exists to track the various statistics           *
 *   required for internet games.                                          *
 *                                                                         *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef CNC_RED_ALERT_RA_UTRACKER_H_
#define CNC_RED_ALERT_RA_UTRACKER_H_

/*
** UnitTracker Class
*/

class UnitTrackerClass {
 public:
  UnitTrackerClass(int unit_count);
  ~UnitTrackerClass();

  void Increment_Unit_Total(int unit_type);
  void Decrement_Unit_Total(int unit_type);
  void Clear_Unit_Total();

  int Get_Unit_Total(int unit_type);
  long* Get_All_Totals();
  int Get_Unit_Count() { return (UnitCount); }

  void To_Network_Format();
  void To_PC_Format();

 private:
  long* UnitTotals;
  int UnitCount;
  int InNetworkFormat;
};

#endif  // CNC_RED_ALERT_RA_UTRACKER_H_
