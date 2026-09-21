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

#ifndef CNC_RED_ALERT_SDLLIB_DRAWBUFF_H_
#define CNC_RED_ALERT_SDLLIB_DRAWBUFF_H_

#include <cstddef>

#include <cstdint>
#include <span>

class PixelView;

/*======================================================================*/
/* Externs for all the common functions between the video buffer */
/* class and the graphic buffer class. */
/*======================================================================*/
int Buffer_Get_Pixel(void* thisptr, int x, int y);
void Buffer_Clear(void* thisptr, unsigned char color);
int32_t Buffer_To_Buffer(void* thisptr, int x, int y, int width, int height,
                         std::span<uint8_t> dest, int32_t dest_size);
int32_t Buffer_To_Page(int dst_x, int dst_y, int width, int height,
                       std::span<const uint8_t> source, void* view);
bool Linear_Blit_To_Linear(void* thisptr, void* dest, int src_x, int src_y,
                           int dst_x, int dst_y, int width, int height,
                           bool transparent);
bool Linear_Scale_To_Linear(void* /*thisptr*/, void* /*dest*/, int /*src_x*/,
                            int /*src_y*/, int /*dst_x*/, int /*dst_y*/,
                            int /*src_width*/, int /*src_height*/,
                            int /*dst_width*/, int /*dst_height*/,
                            bool /*transparent*/,
                            std::span<const uint8_t> /*remap_table*/);

// Draws text onto the view using the current global font (FontPtr).
// Wraps to a new line when text exceeds the view's width. A back_color of 0
// means a transparent background. Does nothing if text or FontPtr is null.
void Buffer_Print(void* thisptr, const char* text, int x, int y, int fore_color,
                  int back_color);

/*======================================================================*/
/* Externs for all the graphic buffer class only functions */
/*======================================================================*/
// x1,y1 and x2,y2 are the two corners, both inclusive.
void Buffer_Draw_Line(void* thisptr, int x1, int y1, int x2, int y2,
                      unsigned char color);
void Buffer_Fill_Rect(void* thisptr, int x1, int y1, int x2, int y2,
                      unsigned char color);
void Buffer_Remap(void* thisptr, int x, int y, int width, int height,
                  std::span<const uint8_t> remap_table);
void Buffer_Draw_Stamp_Clip(PixelView* view,
                            std::span<const std::byte> icon_data, int icon,
                            int x, int y, std::span<const uint8_t> remap_table,
                            int /*min_x*/, int /*min_y*/, int /*max_x*/,
                            int /*max_y*/);

extern PixelView* LogicPage;
extern bool AllowHardwareBlitFills;

#endif  // CNC_RED_ALERT_SDLLIB_DRAWBUFF_H_
