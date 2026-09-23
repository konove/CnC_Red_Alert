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

#ifndef CNC_RED_ALERT_WINVQ_VQM32_MONO_H_
#define CNC_RED_ALERT_WINVQ_VQM32_MONO_H_
/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * FILE
 *     mono.h
 *
 * DESCRIPTION
 *     Mono screen definitions. (32-Bit protected mode)
 *
 * PROGRAMMER
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     Feburary 8, 1995
 *
 ****************************************************************************/

/* Prototypes */

void Mono_Enable();
void Mono_Disable();
void Mono_Set_Cursor(long x, long y);
void Mono_Clear_Screen();
void Mono_Scroll(long lines);
void Mono_Put_Char(long character, long attrib);
void Mono_Draw_Rect(long x, long y, long w, long h, long attrib, long thick);

void Mono_Text_Print(const void* text, long x, long y, long attrib);
void Mono_Print(const void* text);
short Mono_View_Page(long page);
short Mono_X();
short Mono_Y();

#endif  // CNC_RED_ALERT_WINVQ_VQM32_MONO_H_
