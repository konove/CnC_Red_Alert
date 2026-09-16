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

void SDL_Create_Main_Window(const char* title, int width, int height);
void SDL_Event_Loop();
void SDL_Event_Handler(SDL_Event* event);  // implemented in app
void SDL_Send_Quit();
void Video_End_Frame();

/*
**	The WindowList[][8] array contains the following elements.  Use these
**	defines when accessing the WindowList.
*/
// Column indices into a WindowList row.
inline constexpr int kWindowX = 0;       // X byte position of left edge.
inline constexpr int kWindowY = 1;       // Y pixel position of top edge.
inline constexpr int kWindowWidth = 2;   // Width in bytes of the window.
inline constexpr int kWindowHeight = 3;  // Height in pixels of the window.
inline constexpr int kWindowFCol = 4;    // Default foreground color.
inline constexpr int kWindowBCol = 5;    // Default background color.
inline constexpr int kWindowCursorX =
    6;  // Current cursor X position (in rows).
inline constexpr int kWindowCursorY =
    7;  // Current cursor Y position (in lines).
inline constexpr int kWindowPadding = 0x1000;

extern int WindowList[][8];
extern int WindowColumns;
extern int WindowLines;
extern int WindowWidth;
extern unsigned int WinB;
extern unsigned int WinC;
extern unsigned int WinX;
extern unsigned int WinY;
extern unsigned int WinCx;
extern unsigned int WinCy;
extern unsigned int WinH;
extern unsigned int WinW;
extern unsigned int Window;

extern int MoreOn;
extern char* TXT_MoreText;

extern void (*Window_More_Ptr)(const char*, int, int, int);

// Handle to the program's main SDL window (SDL_Window* cast to void* for
// portability). Created by SDL_Create_Main_Window() and used throughout the
// windowing system.
extern void* MainWindow;

struct SDL_Renderer;
extern SDL_Renderer* SDLRenderer;
extern uint32_t ForceRenderEventID;

#endif  // CNC_RED_ALERT_SDLLIB_WW_WIN_H_
