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

#ifndef CNC_RED_ALERT_WINVQ_VQM32_VIDEO_H_
#define CNC_RED_ALERT_WINVQ_VQM32_VIDEO_H_
/****************************************************************************
 *
 *        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
 *
 *----------------------------------------------------------------------------
 *
 * FILE
 *     Video.h (32-Bit protected mode)
 *
 * DESCRIPTION
 *     Video manager definitions.
 *
 * PROGRAMMER
 *     Denzil E. Long, Jr.
 *
 * DATE
 *     Febuary 3, 1995
 *
 ****************************************************************************/

#include <cstdint>

#include "winvq/vqm32/vesavid.h"

/*---------------------------------------------------------------------------
 * VGA video modes
 *-------------------------------------------------------------------------*/

#define TEXT_VIDEO 0x02
#define MCGA 0x13
#define XMODE_320X200 0x50
#define XMODE_320X240 0x51
#define XMODE_320X400 0x52
#define XMODE_320X480 0x53
#define XMODE_360X400 0x54
#define XMODE_360X480 0x55

#define XMODE_MIN 0x50
#define XMODE_MAX 0x55

/*---------------------------------------------------------------------------
 * Structure definitions
 *-------------------------------------------------------------------------*/

/* DisplayInfo - Information about the current display.
 *
 * Mode     - Mode identification.
 * XRes     - X resolution of mode.
 * YRes     - Y resolution of mode.
 * VBIbit   - Polarity of vertical blank bit.
 * Extended - Pointer to mode specific data structure.
 */
typedef struct DisplayInfo {
  int32_t Mode;
  int32_t XRes;
  int32_t YRes;
  int32_t VBIbit;
  void* Extended;
} DisplayInfo;

/*---------------------------------------------------------------------------
 * Function prototypes
 *-------------------------------------------------------------------------*/

DisplayInfo* SetVideoMode(int32_t mode);
DisplayInfo* GetDisplayInfo();
int32_t TestVBIBit();
int32_t GetVBIBit();

void SetupXPaging();
void FlipXPage();
unsigned char* GetXHidPage();
unsigned char* GetXSeenPage();
void DisplayXPage(int32_t page);

#ifdef __cplusplus
extern "C" {
#endif

void __cdecl WaitNoVB(int16_t vbibit);
void __cdecl WaitVB(int16_t vbibit);
void __cdecl ClearVRAM();
int32_t __cdecl SetXMode(int32_t mode);
void __cdecl ClearXMode();
void __cdecl ShowXPage(uint32_t StartOffset);
void __cdecl Xmode_BufferCopy_320x200(void* buff, void* screen);
void __cdecl Xmode_Blit(void* buffer, void* screen, int32_t imgwidth,
                        int32_t imgheight);
void __cdecl MCGA_BufferCopy(unsigned char* buffer, unsigned char* dummy);
void __cdecl MCGA_Blit(unsigned char* buffer, unsigned char* screen,
                       int32_t imgwidth, int32_t imgheight);

#ifdef __cplusplus
}
#endif

#endif  // CNC_RED_ALERT_WINVQ_VQM32_VIDEO_H_
