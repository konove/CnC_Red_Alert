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
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : wwstd.h                                  *
 *                                                                         *
 *                    File Name : WWLIB.H                                  *
 *                                                                         *
 *                   Programmer : Jeff Wilson                              *
 *                                                                         *
 *                   Start Date : March 1, 1994                            *
 *                                                                         *
 *                  Last Update : March 1, 1994   []                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_WWSTD_H_
#define CNC_RED_ALERT_SDLLIB_WWSTD_H_

inline constexpr int kInvalidHandle = -1;

// The sixteen standard palette colors, as `int` palette indices.
inline constexpr int kTBlack = 0;
inline constexpr int kPurple = 1;
inline constexpr int kCyan = 2;
inline constexpr int kGreen = 3;
inline constexpr int kLtGreen = 4;
inline constexpr int kYellow = 5;
inline constexpr int kPink = 6;
inline constexpr int kBrown = 7;
inline constexpr int kRed = 8;
inline constexpr int kLtCyan = 9;
inline constexpr int kLtBlue = 10;
inline constexpr int kBlue = 11;
inline constexpr int kBlack = 12;
inline constexpr int kGrey = 13;
inline constexpr int kLtGrey = 14;
inline constexpr int kWhite = 15;

#endif  // CNC_RED_ALERT_SDLLIB_WWSTD_H_
