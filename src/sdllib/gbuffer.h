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
 *                 Project Name : Westwood 32 Bit Library
 **
 *                                                                         *
 *                    File Name : GBUFFER.H                                *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 26, 1994                             *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *	This module contains the definition for the graphic buffer class.  The
 ** primary functionality of the graphic buffer class is handled by inline
 ** functions that make a call through function pointers to the correct *
 * routine.  This has two benefits:
 **
 *																									*
 *																									*
 *		1) C++ name mangling is not a big deal since the function
 *pointers	* point to functions in standard C format.
 ** 2) The function pointers can be changed when we set a different * graphic
 *mode.  This allows us to have both supervga and mcga		* routines
 *present in memory at once.
 **
 *																									*
 * In the basic library, these functions point to stub routines which just
 ** return.  This makes a product that just uses a graphic buffer take the
 ** minimum amount of code space.  For programs that require MCGA or VESA
 ** support, all that is necessary to do is link either the MCGA or VESA
 ** specific libraries in, previous to WWLIB32.  The linker will then * overide
 *the the necessary stub functions automatically.
 **
 *																									*
 * In addition, there are helpful inline function calls for parameter *
 * ellimination.  This header file gives the defintion for all
 ** GraphicViewPort and GraphicBuffer classes.
 **
 *																									*
 * Terminology:
 **
 *																									*
 *	Buffer Class - A class which consists of a pointer to an allocated
 ** buffer and the size of the buffer that was allocated.
 **
 *																									*
 *	Graphic ViewPort - The Graphic ViewPort defines a window into a
 ** Graphic Buffer.  This means that although a Graphic Buffer
 ** represents linear memory, this may not be true with a Graphic
 ** Viewport.  All low level functions that act directly on a graphic * viewport
 *are included within this class.  This includes but is not	* limited to
 *most of the functions which can act on a Video Viewport	* Video Buffer.
 **
 *																									*
 * Graphic Buffer - A Graphic Buffer is an instance of an allocated buffer
 ** used to represent a rectangular region of graphics memory.
 ** The HidBuff	and BackBuff are excellent examples of a Graphic Buffer.
 **
 *																									*
 * Below is a tree which shows the relationship of the VideoBuffer and * Buffer
 *classes to the GraphicBuffer class:
 **
 *																									*
 *	  BUFFER.H				 GBUFFER.H
 *BUFFER.H				 VBUFFER.H	*
 *  ----------          ----------         ----------          ----------
 ** |  Buffer  |        | Graphic  |       |  Buffer  |        |  Video   |
 ** |  Class   |        | ViewPort |       |  Class   |        | ViewPort |
 **
 *  ----------          ----------         ----------          ----------
 ** \        /                             \        / * \      / \      /
 **
 *            ----------                             ----------
 ** |  Graphic |                           |  Video   | * |  Buffer  | |  Buffer
 *|				*
 *            ----------                             ----------
 ** GBUFFER.H			                       VBUFFER.H
 **
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   GBC::GraphicBufferClass -- inline constructor for GraphicBufferClass  *
 *   GVPC::Remap -- Short form to remap an entire graphic view port        *
 *   GVPC::Get_XPos -- Returns x offset for a graphic viewport class       *
 *   GVPC::Get_Ypos -- Return y offset in a GraphicViewPortClass           *
 *   VVPC::Get_XPos -- Get the x pos of the VP on the Video                *
 *   VVPC::Get_YPos -- Get the y pos of the VP on the video                *
 *   GBC::Get_Graphic_Buffer -- Get the graphic buffer of the VP.          *
 *   GVPC::Draw_Line -- Stub function to draw line in Graphic Viewport Class*
 *   GVPC::Fill_Rect -- Stub function to fill rectangle in a GVPC          *
 *   GVPC::Remap -- Stub function to remap a GVPC                          *
 *   GVPC::Print -- stub func to print a text string                       *
 *   GVPC::Print -- Stub function to print an integer                      *
 *   GVPC::Print -- Stub function to print a short to a graphic viewport   *
 *   GVPC::Print -- stub function to print a long on a graphic view port   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_GBUFFER_H_
#define CNC_RED_ALERT_SDLLIB_GBUFFER_H_

#include "base/numeric.h"

#include <cstddef>

#include <cstdint>
#include <span>

#include "absl/base/attributes.h"
#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/attributes.h"
#include "base/flags.h"
#include "base/types.h"
#include "sdllib/bitmap.h"
#include "sdllib/buffer.h"
#include "sdllib/drawbuff.h"
#include "sdllib/ww_win.h"

//////////////////////////////////////////////////////////////////////////
//
// Defines for direct draw
//
//

enum class CNC_FLAG_ENUM GBC_Enum {
  GBC_NONE = 0,
  GBC_VIDEOMEM = 1,
  GBC_VISIBLE = 2,
};
using enum GBC_Enum;
template <>
inline constexpr bool base::kIsFlagEnum<GBC_Enum> = true;

#define NOT_LOCKED NULL

/*=========================================================================*/
/* Define the screen width and height to make portability to other modules
 */
/*		easier.
 */
/*=========================================================================*/
inline constexpr int kDefaultScreenWidth = 320;
constexpr int DEFAULT_SCREEN_HEIGHT = 200;

/*=========================================================================*/
/* Let the compiler know that a GraphicBufferClass exists so that it can
 */
/*		keep a pointer to it in a VideoViewPortClass.
 */
/*=========================================================================*/
class GraphicBufferClass;

GraphicViewPortClass* Set_Logic_Page(
    GraphicViewPortClass* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);
GraphicViewPortClass* Set_Logic_Page(
    GraphicViewPortClass& ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);

/*=========================================================================*/
/* GraphicViewPortClass - Holds viewport information on a viewport which
 */
/*		has been attached to a GraphicBuffer.  A viewport is effectively
 * a	*/
/*		rectangular subset of the full buffer which is used for clipping
 * and	*/
/*		the like.
 */
/*																									*/
/*			char	  	*Buffer	-		is the offset to
 * view port buffer			*/
/*			int	  	Width		-		is the
 * width of view port						*/
/*			int	  	Height	-		is the height of
 * view port						*/
/*			int	  	XAdd		-		is add
 * value to go from the end of a line	*/
/*											to
 * the beginning of the next line			*/
/*			int		XPos;		- 		x offset
 * into its associated VideoBuffer	*/
/*			int		YPos;		-		y offset
 * into its associated VideoBuffer	*/
/*=========================================================================*/
class GraphicViewPortClass {
 public:
  /*===================================================================*/
  /* Define the base constructor and destructors for the class */
  /*===================================================================*/
  GraphicViewPortClass(GraphicBufferClass* graphic_buff, int x, int y, int w,
                       int h);
  GraphicViewPortClass() = default;
  ~GraphicViewPortClass() = default;
  GraphicViewPortClass(const GraphicViewPortClass&) = delete;
  GraphicViewPortClass& operator=(const GraphicViewPortClass&) = delete;
  GraphicViewPortClass(GraphicViewPortClass&&) = delete;
  GraphicViewPortClass& operator=(GraphicViewPortClass&&) = delete;

  /*===================================================================*/
  /* define functions to get at the private data members
   */
  /*===================================================================*/
  std::uint8_t* Get_Offset();
  std::span<uint8_t> Get_Pixels();
  [[nodiscard]] int Get_Height() const;
  [[nodiscard]] int Get_Width() const;
  [[nodiscard]] int Get_XAdd() const;
  [[nodiscard]] int Get_XPos() const;
  [[nodiscard]] int Get_YPos() const;
  [[nodiscard]] int Get_Pitch() const;
  inline bool Get_IsDirectDraw();
  GraphicBufferClass* Get_Graphic_Buffer();

  /*===================================================================*/
  /* Define a function which allows us to change a video viewport on	*/
  /*		the fly.
   */
  /*===================================================================*/
  bool Change(int x, int y, int w, int h);

  /*===================================================================*/
  /* Define the set of common graphic functions that are supported by	*/
  /*		both Graphic ViewPorts and VideoViewPorts.
   */
  /*===================================================================*/

  void Put_Pixel(int x, int y, unsigned char color);
  void Buffer_Put_Pixel(int x, int y, unsigned char color);
  int Get_Pixel(int x, int y);
  void Clear(unsigned char color = 0);
  int32_t To_Buffer(int x, int y, int w, int h, std::span<uint8_t> buff,
                    int32_t size);
  int32_t To_Buffer(int x, int y, int w, int h, BufferClass* buff);
  int32_t To_Buffer(BufferClass* buff);
  bool Blit(GraphicViewPortClass& dest, int x_pixel, int y_pixel, int dx_pixel,
            int dy_pixel, int pixel_width, int pixel_height,
            bool trans = false);
  bool Blit(GraphicViewPortClass& dest, int dx, int dy, bool trans = false);
  bool Blit(GraphicViewPortClass& dest, bool trans = false);

  bool Scale(GraphicViewPortClass& dest, int src_x, int src_y, int dst_x,
             int dst_y, int src_w, int src_h, int dst_w, int dst_h,
             bool trans = false, std::span<const uint8_t> remap = {});
  bool Scale(GraphicViewPortClass& dest, int src_x, int src_y, int dst_x,
             int dst_y, int src_w, int src_h, int dst_w, int dst_h,
             std::span<const uint8_t> remap);
  bool Scale(GraphicViewPortClass& dest, bool trans = false,
             std::span<const uint8_t> remap = {});
  bool Scale(GraphicViewPortClass& dest, std::span<const uint8_t> remap);

  void Print(const char* string, int x_pixel, int y_pixel, int fcolor,
             int bcolor);
  void Print(int num, int x_pixel, int y_pixel, int fcolor, int bcolor);

  /*===================================================================*/
  /* Define the list of graphic functions which work only with a */
  /*		graphic buffer.
   */
  /*===================================================================*/
  void Draw_Line(int sx, int sy, int dx, int dy, unsigned char color);
  void Draw_Rect(int sx, int sy, int dx, int dy, unsigned char color);
  void Fill_Rect(int sx, int sy, int dx, int dy, unsigned char color);

  void Remap(int sx, int sy, int width, int height,
             std::span<const uint8_t> remap);
  void Remap(std::span<const uint8_t> remap);

  void Draw_Stamp(std::span<const std::byte> icondata, int icon, int x_pixel,
                  int y_pixel, std::span<const uint8_t> remap, int clip_window);

  //
  // New members to lock and unlock the direct draw video memory
  //
  inline bool Lock();
  inline bool Unlock();
  [[nodiscard]] inline int Get_LockCount() const;

  /*===================================================================*/
  /* Define functions to attach the viewport to a graphicbuffer */
  /*===================================================================*/
  void Attach(GraphicBufferClass* graphic_buff, int x, int y, int w, int h);

 protected:
  /*===================================================================*/
  /* Define the data used by a GraphicViewPortClass
   */
  /*===================================================================*/
  std::uint8_t* Offset = nullptr;            // offset to graphic page
  int Width = 0;                            // width of graphic page
  int Height = 0;                           // height of graphic page
  int XAdd = 0;                             // xadd for graphic page (0)
  int XPos = 0;                             // x offset in relation to graphicbuff
  int YPos = 0;                             // y offset in relation to graphicbuff
  int32_t Pitch = 0;  // Distance from one line to the next
  GraphicBufferClass* GraphicBuff = nullptr;  // related graphic buff
  int LockCount = 0;  // Count for stacking locks if non-zero the buffer
};

/*=========================================================================*/
/* GraphicBufferClass - A GraphicBuffer refers to an actual instance of an
 */
/*		allocated buffer.  The GraphicBuffer may be drawn to directly
 */
/*		becuase it inherits a ViewPort which represents its physcial
 * size.	*/
/*																									*/
/*			BYTE	  	*Buffer	-		is the offset to
 * graphic buffer				*/
/*			int	  	Width		-		is the
 * width of graphic buffer				*/
/*			int	  	Height	-		is the height of
 * graphic buffer				*/
/*			int	  	XAdd		-		is the
 * xadd of graphic buffer					*/
/*			int		XPos;		- 		will be
 * 0 because it is graphicbuff			*/
/*			int		YPos;		-		will be
 * 0 because it is graphicbuff			*/
/*			long	Pitch		-		modulo of buffer
 * for reading and writing
 */
/*			bool	IsDirectDraw - 		flag if its a direct
 * draw surface
 */
/*=========================================================================*/
class GraphicBufferClass : public GraphicViewPortClass, public BufferClass {
 public:
  GraphicBufferClass(int w, int h, std::span<uint8_t> buffer, int32_t size);
  GraphicBufferClass(int w, int h, std::span<uint8_t> buffer = {});
  GraphicBufferClass();
  ~GraphicBufferClass();

  GraphicBufferClass(const GraphicBufferClass&) = delete;
  GraphicBufferClass& operator=(const GraphicBufferClass&) = delete;
  GraphicBufferClass(GraphicBufferClass&&) = delete;
  GraphicBufferClass& operator=(GraphicBufferClass&&) = delete;

  void Init(int w, int h, std::span<uint8_t> buffer, int32_t size,
            GBC_Enum flags);
  void Un_Init();

  // Locks and unlocks the underlying SDL surface. Callers normally use the
  // inherited GraphicViewPortClass::Lock/Unlock, which also reattach the
  // viewport to the freshly locked pixels.
  bool Lock_Surface();
  bool Unlock_Surface();

  // Draws `bmp` scaled and rotated onto this buffer, centered at `pt`.
  // `scale` is 24.8 fixed point (0x100 = 1.0). `angle` is 0-255 (full circle).
  // Zero pixels are treated as transparent. No clipping is performed.
  void Scale_Rotate(const BitmapClass& bmp, const TPoint2D& pt, int32_t scale,
                    uint8_t angle);

  [[nodiscard]] bool Is_Window_Surface() const {
    return WindowTexture != nullptr;
  }
  void Update_Window_Surface(bool end_frame);
  void Update_Palette(std::span<const uint8_t> palette);
  [[nodiscard]] const void* Get_Palette() const;

  // Render paletted frame data with SDL texture scaling (for VQA movies, etc.)
  // Uses the palette already set via Update_Palette.
  void Render_Scaled_Frame(std::span<const uint8_t> paletted_data, int width,
                           int height);
  void Destroy_VQA_Texture();

 protected:
  void Init_Display_Surface();
  void* WindowTexture = nullptr;
  void* PaletteSurface = nullptr;
  int RedrawTimer = 0;
  void* VQATexture = nullptr;  // SDL_Texture* for low-res content scaling
  int VQATextureWidth = 0;
  int VQATextureHeight = 0;
};

extern GraphicBufferClass* WindowBuffer;

void Do_Set_Palette(std::span<const uint8_t> palette);

inline int GraphicViewPortClass::Get_LockCount() const { return LockCount; }

/***********************************************************************************************
 * GVPC::Get_IsDirectDraw -- provide read access to the IsDirectDraw flag *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   IsDirectDraw *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 11/29/95 1:02PM ST : Created *
 *=============================================================================================*/
inline bool GraphicViewPortClass::Get_IsDirectDraw() {
  // this flag is used as "do we need to lock" in a few places
  return GraphicBuff != nullptr && GraphicBuff->Is_Window_Surface();
}

/***********************************************************************************************
 * GVPC::Lock -- lock the graphics buffer for reading or writing *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully locked *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 09-19-95 12:33pm ST : Created * 10/09/1995     : Moved actually
 *functionality to GraphicBuffer                            *
 *=============================================================================================*/
inline bool GraphicViewPortClass::Lock() {
  const bool lock = GraphicBuff->Lock_Surface();
  if (!lock) {
    return false;
  }

  if (this != GraphicBuff) {
    Attach(GraphicBuff, XPos, YPos, Width, Height);
  }
  return true;
}

/***********************************************************************************************
 * GVPC::Unlock -- unlock the video buffer *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully unlocked *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 09-19-95 02:20pm ST : Created * 10/09/1995     : Moved actually
 *functionality to GraphicBuffer                            *
 *=============================================================================================*/
inline bool GraphicViewPortClass::Unlock() {
  return GraphicBuff->Unlock_Surface();
}

/***************************************************************************
 * GVPC::GET_OFFSET -- Get offset for virtual view port class instance     *
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     long the offset for the virtual viewport instance           *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline std::uint8_t* GraphicViewPortClass::Get_Offset() { return Offset; }
inline std::span<uint8_t> GraphicViewPortClass::Get_Pixels() {
  if (GraphicBuff == nullptr) {
    return {};
  }
  const auto pixels = GraphicBuff->Get_Bytes();
  if (this == GraphicBuff) {
    return GraphicBuff->Get_Bytes();
  }
  return pixels.subspan(base::ToSize((YPos * (Width + XAdd + Pitch)) + XPos));
}

/***************************************************************************
 * GVPC::GET_HEIGHT -- Gets the height of a virtual viewport instance      *
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     WORD the height of the virtual viewport instance            *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int GraphicViewPortClass::Get_Height() const { return Height; }

/***************************************************************************
 * GVPC::GET_WIDTH -- Get the width of a virtual viewport instance
 **
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     WORD the width of the virtual viewport instance             *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int GraphicViewPortClass::Get_Width() const { return Width; }

/***************************************************************************
 * GVPC::GET_XADD -- Get the X add offset for virtual viewport instance    *
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     WORD the xadd for a virtual viewport instance               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int GraphicViewPortClass::Get_XAdd() const { return XAdd; }
/***************************************************************************
 * GVPC::GET_XPOS -- Get the x pos of the VP on the Video                  *
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     WORD the x offset to VideoBufferClass
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline int GraphicViewPortClass::Get_XPos() const { return XPos; }

/***************************************************************************
 * GVPC::GET_YPOS -- Get the y pos of the VP on the video                  *
 *                                                                         *
 * INPUT:		none *
 *                                                                         *
 * OUTPUT:     WORD the x offset to VideoBufferClass
 **
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline int GraphicViewPortClass::Get_YPos() const { return YPos; }

/***************************************************************************
 * GVPC::GET_GRAPHIC_BUFFER -- Get the graphic buffer of the VP.            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline GraphicBufferClass* GraphicViewPortClass::Get_Graphic_Buffer() {
  return GraphicBuff;
}

inline void GraphicViewPortClass::Put_Pixel(int x, int y, unsigned char color) {
  if (!Lock()) {
    return;
  }

  this->Buffer_Put_Pixel(x, y, color);

  Unlock();
}

inline void GraphicViewPortClass::Buffer_Put_Pixel(const int x, const int y,
                                                   const unsigned char color) {
  if (x >= 0 && y >= 0 && x < Get_Width() && y < Get_Height()) {
    const base::ssize pitch = Get_XAdd() + Get_Width() + Get_Pitch();
    base::At(Get_Pixels(), base::ToSize(x + (y * pitch))) = color;
  }
}

inline int GraphicViewPortClass::Get_Pixel(int x, int y) {
  int return_code = 0;

  if (Lock()) {
    return_code = Buffer_Get_Pixel(this, x, y);
  }
  Unlock();
  return return_code;
}

inline void GraphicViewPortClass::Clear(unsigned char color) {
  if (Lock()) {
    Buffer_Clear(this, color);
  }
  Unlock();
}

inline int32_t GraphicViewPortClass::To_Buffer(int x, int y, int w, int h,
                                               std::span<uint8_t> buff,
                                               int32_t size) {
  int32_t return_code = 0;
  if (Lock()) {
    return_code = Buffer_To_Buffer(this, x, y, w, h, buff, size);
  }
  Unlock();
  return return_code;
}

inline int32_t GraphicViewPortClass::To_Buffer(int x, int y, int w, int h,
                                               BufferClass* buff) {
  return To_Buffer(x, y, w, h, buff->Get_Bytes(), buff->Get_Size());
}

inline int32_t GraphicViewPortClass::To_Buffer(BufferClass* buff) {
  return To_Buffer(0, 0, Width, Height, buff->Get_Bytes(), buff->Get_Size());
}

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, int x_pixel,
                                       int y_pixel, int dx_pixel, int dy_pixel,
                                       int pixel_width, int pixel_height,
                                       bool trans) {
  bool return_code = false;

  if (Lock()) {
    if (dest.Lock()) {
      return_code =
          Linear_Blit_To_Linear(this, &dest, x_pixel, y_pixel, dx_pixel,
                                dy_pixel, pixel_width, pixel_height, trans);
    }
    dest.Unlock();
  }
  Unlock();

  return return_code;
}

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, int dx,
                                       int dy, bool trans) {
  return Blit(dest, 0, 0, dx, dy, Width, Height, trans);
}

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, bool trans) {
  return Blit(dest, 0, 0, 0, 0, Width, Height, trans);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, int src_x,
                                        int src_y, int dst_x, int dst_y,
                                        int src_w, int src_h, int dst_w,
                                        int dst_h, bool trans,
                                        std::span<const uint8_t> remap) {
  bool return_code = false;
  if (Lock()) {
    if (dest.Lock()) {
      return_code =
          Linear_Scale_To_Linear(this, &dest, src_x, src_y, dst_x, dst_y, src_w,
                                 src_h, dst_w, dst_h, trans, remap);
    }
    dest.Unlock();
  }
  Unlock();
  return return_code;
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, int src_x,
                                        int src_y, int dst_x, int dst_y,
                                        int src_w, int src_h, int dst_w,
                                        int dst_h,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h,
               false, remap);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, bool trans,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(),
               dest.Get_Height(), trans, remap);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(),
               dest.Get_Height(), false, remap);
}

inline void GraphicViewPortClass::Print(const char* string, int x_pixel,
                                        int y_pixel, int fcolor, int bcolor) {
  if (!Lock()) {
    return;
  }
  Buffer_Print(this, string, x_pixel, y_pixel, fcolor, bcolor);
  Unlock();
}

inline void GraphicViewPortClass::Print(int num, int x_pixel, int y_pixel,
                                        int fcolor, int bcolor) {
  Print(absl::StrCat(num).c_str(), x_pixel, y_pixel, fcolor, bcolor);
}

inline void GraphicViewPortClass::Draw_Stamp(
    std::span<const std::byte> icondata, int icon, int x_pixel, int y_pixel,
    const std::span<const uint8_t> remap, int clip_window) {
  if (Lock()) {
#ifdef TD
    Buffer_Draw_Stamp_Clip(
        this, icondata, icon, x_pixel, y_pixel, remap,
        base::At(base::At(WindowList, clip_window), kWindowX) * 8,
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth) * 8,
        base::At(base::At(WindowList, clip_window), kWindowHeight));
#else
    Buffer_Draw_Stamp_Clip(
        this, icondata, icon, x_pixel, y_pixel, remap,
        base::At(base::At(WindowList, clip_window), kWindowX),
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth),
        base::At(base::At(WindowList, clip_window), kWindowHeight));
#endif
  }
  Unlock();
}

inline void GraphicViewPortClass::Draw_Line(int sx, int sy, int dx, int dy,
                                            unsigned char color) {
  if (Lock()) {
    Buffer_Draw_Line(this, sx, sy, dx, dy, color);
  }
  Unlock();
}

inline void GraphicViewPortClass::Fill_Rect(int sx, int sy, int dx, int dy,
                                            unsigned char color) {
  if (Lock()) {
    Buffer_Fill_Rect(this, sx, sy, dx, dy, color);
    Unlock();
  }
}

inline void GraphicViewPortClass::Remap(int sx, int sy, int width, int height,
                                        std::span<const uint8_t> remap) {
  if (Lock()) {
    Buffer_Remap(this, sx, sy, width, height, remap);
  }
  Unlock();
}

inline void GraphicViewPortClass::Remap(std::span<const uint8_t> remap) {
  Remap(0, 0, Width, Height, remap);
}

inline int GraphicViewPortClass::Get_Pitch() const {
  return static_cast<int>(Pitch);
}
/*=========================================================================*/
/* The following BufferClass functions are defined here because they act
 */
/*		on graphic viewports.
 */
/*=========================================================================*/

/***************************************************************************
 * BUFFER_TO_PAGE -- Generic 'c' callable form of Buffer_To_Page           *
 *                                                                         *
 * INPUT:
 **
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t Buffer_To_Page(int x, int y, int w, int h,
                              std::span<const uint8_t> Buffer,
                              GraphicViewPortClass& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, w, h, Buffer, &view);
  }
  view.Unlock();
  return return_code;
}

/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h *
 *                                                                         *
 * INPUT:		int	width		- the width of copy region
 ** int	height	- the height of copy region
 ** GVPC&	dest		- virtual viewport to copy to
 **
 *                                                                         *
 * OUTPUT:		none *
 *																									*
 * WARNINGS:	x and y position are the upper left corner of the dest *
 *						viewport
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(int w, int h, GraphicViewPortClass& view) {
  return To_Page(0, 0, w, h, view);
}
/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h *
 *                                                                         *
 * INPUT:		GVPC&	dest		- virtual viewport to copy to
 **
 *                                                                         *
 * OUTPUT:		none *
 *																									*
 * WARNINGS:	x and y position are the upper left corner of the dest *
 *						viewport.  width and height are
 *assumed to be the			* viewport's width and height.
 **
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(GraphicViewPortClass& view) {
  return To_Page(0, 0, view.Get_Width(), view.Get_Height(), view);
}
/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable x, y, w, h *
 *                                                                         *
 * INPUT:	int	x			- x pixel on viewport to copy
 *from					* int	y			- y
 *pixel on viewport to copy from					* int
 *width		- the width of copy region
 ** int	height	- the height of copy region
 ** GVPC&	dest		- virtual viewport to copy to
 **
 *                                                                         *
 * OUTPUT:	none                                                           *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(int x, int y, int w, int h,
                                    GraphicViewPortClass& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, w, h, Get_Bytes(), &view);
  }
  view.Unlock();
  return return_code;
}

#endif  // CNC_RED_ALERT_SDLLIB_GBUFFER_H_
