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
 *                 Project Name : Part of the WINDOWS Library              *
 *                                                                         *
 *                    File Name : WINDOWS.H                                *
 *                                                                         *
 *                   Programmer : Barry W. Green                           *
 *                                                                         *
 *                   Start Date : February 16, 1995                        *
 *                                                                         *
 *                  Last Update : February 16, 1995 [BWG]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_WW_WIN_H_
#define CNC_RED_ALERT_SDLLIB_WW_WIN_H_

#include <cstdint>

union SDL_Event;

/*=========================================================================*/
/* The following prototypes are for the file: WINDOWS.CPP
 */
/*=========================================================================*/
int Change_Window(int windnum);

// Function type for the game's SDL event handler.
using EventHandler = void (*)(SDL_Event* event);

// Installs the game's event handler, called once per event SDL_Event_Loop
// pumps (after Display's own redraw events are consumed and handled
// internally). With none installed, pumped events are dropped.
void SetEventHandler(EventHandler handler);

void SDL_Event_Loop();
void SDL_Send_Quit();

extern int WindowColumns;
extern int WindowLines;
extern int WindowWidth;
extern unsigned int WinB;
extern unsigned int WinC;
extern unsigned int WinCx;
extern unsigned int WinCy;
extern unsigned int WinH;
extern unsigned int WinW;

extern int MoreOn;
extern char* TXT_MoreText;

extern void (*Window_More_Ptr)(const char*, int, int, int);

#endif  // CNC_RED_ALERT_SDLLIB_WW_WIN_H_
