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
;**   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
;***************************************************************************
;*                                                                         *
;*                 Project Name : iff                                      *
;*                                                                         *
;*                    File Name : FILEPCX.H                                *
;*                                                                         *
;*                   Programmer : Julio R. Jerez                           *
;*                                                                         *
;*                   Start Date : May 2, 1995                              *
;*                                                                         *
;*                  Last Update : May 2, 1995   [JRJ]                      *
;*                                                                         *
;*-------------------------------------------------------------------------*
;* Functions:                                                              *
;* GraphicBufferClass* Read_PCX_File (char* name, BYTE* palette,void *buff, long
size);
;* GraphicBufferClass* Read_PCX_File (char* name, BYTE* palette, BufferClass&
Buff);
;* int Write_PCX_File (char* name, GraphicViewPortClass& pic, BYTE* palette );*
;*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
#ifndef CNC_RED_ALERT_RA_FILEPCX_H_
#define CNC_RED_ALERT_RA_FILEPCX_H_

#include <cstdint>

#include "ra/palette.h"
#include "sdllib/buffer.h"
#include "sdllib/gbuffer.h"
#include "tech/wwfile.h"

typedef struct {
  unsigned char red;
  unsigned char green;
  unsigned char blue;
} RGB;

typedef struct {
  char id;
  char version;
  char encoding;
  char pixelsize;
  int16_t x;
  int16_t y;
  int16_t width;
  int16_t height;
  int16_t xres;
  int16_t yres;
  RGB ega_palette[16];
  char nothing;
  char color_planes;
  int16_t byte_per_line;
  int16_t palette_type;
  char filler[58];
} PCX_HEADER;

GraphicBufferClass* Read_PCX_File(const char* name, char* palette, void* buff,
                                  int32_t size);
GraphicBufferClass* Read_PCX_File(const char* name, BufferClass& Buff,
                                  char* palette = nullptr);

int Write_PCX_File(FileClass& file, GraphicBufferClass& pic,
                   PaletteClass* palette);

#endif  // CNC_RED_ALERT_RA_FILEPCX_H_
