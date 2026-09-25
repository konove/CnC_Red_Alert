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
 *                 Project Name : 32 bit library                           *
 *                                                                         *
 *                    File Name : MISC.H                                   *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : August 3, 1994                           *
 *                                                                         *
 *                  Last Update : August 3, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_MISC_H_
#define CNC_RED_ALERT_SDLLIB_MISC_H_

#include "sdllib/display.h"

/*========================= C++ Routines ==================================*/

/*=========================================================================*/
/* The following prototypes are for the file: DDRAW.CPP
 */
/*=========================================================================*/
// Gives the window a w x h paletted surface through
// TheDisplay().SetVideoMode(); the surface is always 8-bit, whatever
// `bits_per_pixel` says. Returns false if SDL could not create it.
bool Set_Video_Mode(int w, int h, int bits_per_pixel);
void Wait_Blit();

/*
** Pointer to function to call if we detect a focus loss
*/
extern void (*Misc_Focus_Loss_Function)();
/*
** Pointer to function to call if we detect a surface restore
*/
extern void (*Misc_Focus_Restore_Function)();

// Cleans up the library systems (audio, mouse, tick timer, ...) before the
// process exits. Each game defines it in its startup.cc.
void Prog_End();

/*=========================================================================*/
/* The following prototypes are for the file: DELAY.CPP
 */
/*=========================================================================*/
void Delay(int duration);

void Shake_Screen(int shakes);
inline void Wait_Vert_Blank() { TheDisplay().EndFrame(); }

/*=========================================================================*/

#endif  // CNC_RED_ALERT_SDLLIB_MISC_H_
