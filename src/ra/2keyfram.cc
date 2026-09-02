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

/* $Header: /CounterStrike/2KEYFRAM.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : KEYFRAME.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/25/95 *
 *                                                                                             *
 *                  Last Update : June 25, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Get_Build_Frame_Count -- Fetches the number of frames in data
 *block.                      * Get_Build_Frame_Width -- Fetches the width of
 *the shape image.                            * Get_Build_Frame_Height --
 *Fetches the height of the shape image.                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include <bit>
#include <cstdint>
#include <cstring>

#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/keyframe.h"
#include "sdllib/iff.h"
#include "sdllib/memflag.h"
#include "sdllib/wsa.h"

// 3 1/2 frame offsets loaded (2 offsets/frame).
constexpr int kSubFrameOffs = 7;

struct KeyFrameHeaderType {
  uint16_t frames;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t largest_frame_size;
  int16_t flags;
};

constexpr int kInitialBigShapeBufferSize = 8000000;
constexpr int kTheaterBigShapeBufferSize = 4000000;
constexpr uint16_t kUncompressMagicNumber = 56789;

unsigned BigShapeBufferLength = kInitialBigShapeBufferSize;
unsigned TheaterShapeBufferLength = kTheaterBigShapeBufferSize;
char* BigShapeBufferStart = nullptr;
char* TheaterShapeBufferStart = nullptr;
bool UseBigShapeBuffer = false;
bool IsTheaterShape = false;
/*
** Global required to fix the score screen crash bug by allowing disabling of
*uncompressed shapes.
*/
bool OriginalUseBigShapeBuffer = false;
char* BigShapeBufferPtr = nullptr;
int TotalBigShapes = 0;
bool ReallocShapeBufferFlag = false;

char* TheaterShapeBufferPtr = nullptr;
int TotalTheaterShapes = 0;

constexpr int kMaxSlots = 1500;
constexpr int kTheaterSlotStart = 1000;

char** KeyFrameSlots[kMaxSlots];
int TotalSlotsUsed = 0;
int TheaterSlotsUsed = kTheaterSlotStart;

struct ShapeHeaderType {
  unsigned draw_flags;
  char* shape_data;
  int shape_buffer;  // 1 if shape is in theater buffer
};

static int Length;

void* Get_Shape_Header_Data(void* ptr) {
  if (UseBigShapeBuffer) {
    const auto* header = static_cast<ShapeHeaderType*>(ptr);
    char* base =
        header->shape_buffer ? TheaterShapeBufferStart : BigShapeBufferStart;
    return header->shape_data + std::bit_cast<uintptr_t>(base);
  }
  return ptr;
}

int Get_Last_Frame_Length() { return Length; }

void Reset_Theater_Shapes() {
  /*
  ** Delete any previously allocated slots
  */
  for (int i = kTheaterSlotStart; i < TheaterSlotsUsed; i++) {
    delete[] KeyFrameSlots[i];
  }

  TheaterShapeBufferPtr = TheaterShapeBufferStart;
  TotalTheaterShapes = 0;
  TheaterSlotsUsed = kTheaterSlotStart;
}

void Reallocate_Big_Shape_Buffer() {
  if (ReallocShapeBufferFlag) {
    BigShapeBufferLength += 2000000;  // Extra 2 Mb of uncompressed shape space
    BigShapeBufferPtr -= std::bit_cast<uintptr_t>(BigShapeBufferStart);
    Memory_Error = nullptr;
    BigShapeBufferStart = static_cast<char*>(
        Resize_Alloc(BigShapeBufferStart, BigShapeBufferLength));
    Memory_Error = &Memory_Error_Handler;
    /*
    ** If we have run out of memory then disable the uncompressed shapes
    ** It may still be possible to continue with compressed shapes
    */
    if (!BigShapeBufferStart) {
      UseBigShapeBuffer = false;
      return;
    }
    BigShapeBufferPtr += std::bit_cast<uintptr_t>(BigShapeBufferStart);
    ReallocShapeBufferFlag = false;
  }
}

/***********************************************************************************************
 * Disable_Uncompressed_Shapes -- Temporarily turns off shape decompression *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 11/19/96 2:37PM ST : Created *
 *=============================================================================================*/
void Disable_Uncompressed_Shapes() { UseBigShapeBuffer = false; }

/***********************************************************************************************
 * Enable_Uncompressed_Shapes -- Restores state of shape decompression before it
 *was disabled  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 11/19/96 2:37PM ST : Created *
 *=============================================================================================*/
void Enable_Uncompressed_Shapes() {
  UseBigShapeBuffer = OriginalUseBigShapeBuffer;
}

void Check_Use_Compressed_Shapes() {
  UseBigShapeBuffer =
      false;  // haven't implemented the draw code that uses this
  /*
  ** Keep track of our original decision about whether to use cached shapes.
  ** This is needed for the score screen crash fix.
  */
  OriginalUseBigShapeBuffer = UseBigShapeBuffer;
}

void* Build_Frame(const void* dataptr, const uint16_t framenumber,
                  void* buffptr) {
  uint32_t offset[kSubFrameOffs];
  int32_t length = 0;

  // valid pointer??
  Length = 0;
  if (!dataptr || !buffptr) {
    return nullptr;
  }

  // look at header then check that frame to build is not greater
  // than total frames
  auto* keyfr = (KeyFrameHeaderType*)dataptr;

  if (framenumber >= keyfr->frames) {
    return nullptr;
  }

  if (UseBigShapeBuffer) {
    // If we haven't yet allocated memory for uncompressed shapes then do so
    // now.
    if (!BigShapeBufferStart) {
      BigShapeBufferStart = new char[BigShapeBufferLength];
      BigShapeBufferPtr = BigShapeBufferStart;

      /*
      ** Allocate memory for theater specific uncompressed shapes
      */
      TheaterShapeBufferStart = new char[TheaterShapeBufferLength];
      TheaterShapeBufferPtr = TheaterShapeBufferStart;
    }

    /*
    ** If we are running out of memory (<10k left) for uncompressed shapes
    ** then allocate some more.
    */
    if ((uintptr_t)BigShapeBufferStart + BigShapeBufferLength -
            (uintptr_t)BigShapeBufferPtr <
        128000) {
      ReallocShapeBufferFlag = true;
    }

    /*
    ** If this animation was not previously uncompressed then
    ** allocate memory to keep the pointers to the uncompressed data
    ** for these animation frames
    */
    if (keyfr->x != kUncompressMagicNumber) {
      keyfr->x = kUncompressMagicNumber;
      if (IsTheaterShape) {
        keyfr->y = TheaterSlotsUsed;
        TheaterSlotsUsed++;
      } else {
        keyfr->y = TotalSlotsUsed;
        TotalSlotsUsed++;
      }
      /*
      ** Allocate and clear the memory for the shape info
      */
      // Value initialized rather than memset: the original cleared
      // `frames * 4` bytes, which left the upper half of the slots
      // indeterminate once pointers grew to 8 bytes.
      KeyFrameSlots[keyfr->y] = new char*[keyfr->frames]();
    }

    /*
    ** If this frame was previously uncompressed then just return
    ** a pointer to the raw data
    */
    if (*(KeyFrameSlots[keyfr->y] + framenumber)) {
      if (IsTheaterShape) {
        return TheaterShapeBufferStart +
               std::bit_cast<uintptr_t>(
                   *(KeyFrameSlots[keyfr->y] + framenumber));
      }
      return BigShapeBufferStart +
             std::bit_cast<uintptr_t>(*(KeyFrameSlots[keyfr->y] + framenumber));
    }
  }

  // calc buff size
  const int buffsize = keyfr->width * keyfr->height;

  // get offset into data
  auto* ptr = static_cast<char*>(Add_Long_To_Pointer(
      dataptr,
      (static_cast<int32_t>(framenumber) << 3) + sizeof(KeyFrameHeaderType)));
  Mem_Copy(ptr, &offset[0], 12);
  const char frameflags = static_cast<char>(offset[0] >> 24);

  if (frameflags & KF_KEYFRAME) {
    ptr = static_cast<char*>(
        Add_Long_To_Pointer(dataptr, offset[0] & 0x00FFFFFF));

    if (keyfr->flags & 1) {
      ptr = static_cast<char*>(Add_Long_To_Pointer(ptr, 768));
    }
    length = static_cast<int32_t>(LCW_Uncompress(ptr, buffptr, buffsize));
  } else {
    uint16_t currframe = 0;
    // key delta or delta

    if (frameflags & KF_DELTA) {
      currframe = static_cast<uint16_t>(offset[1]);

      ptr = static_cast<char*>(Add_Long_To_Pointer(
          dataptr,
          (static_cast<int32_t>(currframe) << 3) + sizeof(KeyFrameHeaderType)));
      Mem_Copy(ptr, &offset[0], kSubFrameOffs * sizeof(uint32_t));
    }

    // key frame
    const uint32_t offcurr = offset[1] & 0x00FFFFFF;

    // key delta
    uint32_t offdiff = (offset[0] & 0x00FFFFFF) - offcurr;

    ptr = static_cast<char*>(Add_Long_To_Pointer(dataptr, offcurr));

    if (keyfr->flags & 1) {
      ptr = static_cast<char*>(Add_Long_To_Pointer(ptr, 768));
    }

    length = static_cast<int32_t>(LCW_Uncompress(ptr, buffptr, buffsize));

    if (length > buffsize) {
      return nullptr;
    }

    length = buffsize;
    Apply_XOR_Delta(static_cast<char*>(buffptr),
                    static_cast<char*>(Add_Long_To_Pointer(ptr, offdiff)));

    if (frameflags & KF_DELTA) {
      // adjust to delta after the keydelta

      currframe++;
      int subframe = 2;

      while (currframe <= framenumber) {
        offdiff = (offset[subframe] & 0x00FFFFFF) - offcurr;

        Apply_XOR_Delta(static_cast<char*>(buffptr),
                        static_cast<char*>(Add_Long_To_Pointer(ptr, offdiff)));

        currframe++;
        subframe += 2;

        if (subframe >= kSubFrameOffs - 1 && currframe <= framenumber) {
          Mem_Copy(Add_Long_To_Pointer(dataptr,
                                       (static_cast<int32_t>(currframe) << 3) +
                                           sizeof(KeyFrameHeaderType)),
                   &offset[0], kSubFrameOffs * sizeof(uint32_t));
          subframe = 0;
        }
      }
    }
  }

  if (UseBigShapeBuffer) {
    char* temp_shape_ptr;
    void* return_value;
    /*
    ** Save the uncompressed shape data so we dont have to uncompress it
    ** again next time its drawn.
    ** We keep a space free before the raw shape data so we can add line
    ** header info before the shape is drawn for the first time
    */

    if (IsTheaterShape) {
      /*
      ** Shape is a theater specific shape
      */
      return_value = TheaterShapeBufferPtr;
      temp_shape_ptr =
          TheaterShapeBufferPtr + keyfr->height + sizeof(ShapeHeaderType);
      /*
      ** align the actual shape data
      */
      if (3 & (uintptr_t)temp_shape_ptr) {
        temp_shape_ptr = (char*)((uintptr_t)(temp_shape_ptr + 3) & ~3);
      }

      memcpy(temp_shape_ptr, buffptr, length);
      ((ShapeHeaderType*)TheaterShapeBufferPtr)->draw_flags =
          -1;  // Flag that headers need to be generated
      ((ShapeHeaderType*)TheaterShapeBufferPtr)->shape_data =
          temp_shape_ptr -
          (uintptr_t)TheaterShapeBufferStart;  // pointer to old raw shape data
      ((ShapeHeaderType*)TheaterShapeBufferPtr)->shape_buffer =
          1;  // Theater buffer
      *(KeyFrameSlots[keyfr->y] + framenumber) =
          TheaterShapeBufferPtr - (uintptr_t)TheaterShapeBufferStart;
      TheaterShapeBufferPtr = (char*)(length + (uintptr_t)temp_shape_ptr);
      /*
      ** Align the next shape
      */
      if (3 & (uintptr_t)TheaterShapeBufferPtr) {
        TheaterShapeBufferPtr =
            (char*)((uintptr_t)(TheaterShapeBufferPtr + 3) & ~3);
      }
      Length = length;
      return return_value;
    }
    return_value = BigShapeBufferPtr;
    temp_shape_ptr =
        BigShapeBufferPtr + keyfr->height + sizeof(ShapeHeaderType);
    /*
    ** align the actual shape data
    */
    if (3 & (uintptr_t)temp_shape_ptr) {
      temp_shape_ptr = (char*)((uintptr_t)(temp_shape_ptr + 3) & ~3);
    }
    memcpy(temp_shape_ptr, buffptr, length);
    ((ShapeHeaderType*)BigShapeBufferPtr)->draw_flags =
        -1;  // Flag that headers need to be generated
    ((ShapeHeaderType*)BigShapeBufferPtr)->shape_data =
        temp_shape_ptr -
        (uintptr_t)BigShapeBufferStart;  // pointer to old raw shape data
    ((ShapeHeaderType*)BigShapeBufferPtr)->shape_buffer =
        0;  // Normal Big Shape Buffer
    *(KeyFrameSlots[keyfr->y] + framenumber) =
        BigShapeBufferPtr - (uintptr_t)BigShapeBufferStart;
    BigShapeBufferPtr = (char*)(length + (uintptr_t)temp_shape_ptr);
    // Align the next shape
    if (3 & (uintptr_t)BigShapeBufferPtr) {
      BigShapeBufferPtr = (char*)((uintptr_t)(BigShapeBufferPtr + 3) & ~3);
    }
    Length = length;
    return return_value;
  }
  return buffptr;
}

/***********************************************************************************************
 * Get_Build_Frame_Count -- Fetches the number of frames in data block. *
 *                                                                                             *
 *    Use this routine to determine the number of shapes within the data block.
 **
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of shapes in the data block. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Commented. *
 *=============================================================================================*/
uint16_t Get_Build_Frame_Count(const void* dataptr) {
  if (dataptr) {
    return static_cast<const KeyFrameHeaderType*>(dataptr)->frames;
  }
  return 0;
}

uint16_t Get_Build_Frame_X(const void* dataptr) {
  if (dataptr) {
    return static_cast<const KeyFrameHeaderType*>(dataptr)->x;
  }
  return 0;
}

uint16_t Get_Build_Frame_Y(const void* dataptr) {
  if (dataptr) {
    return static_cast<const KeyFrameHeaderType*>(dataptr)->y;
  }
  return 0;
}

/***********************************************************************************************
 * Get_Build_Frame_Width -- Fetches the width of the shape image. *
 *                                                                                             *
 *    Use this routine to fetch the width of the shapes within the keyframe
 *shape data block.  * All shapes within the block have the same width. *
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block. *
 *                                                                                             *
 * OUTPUT:  Returns with the width of the shapes in the block -- expressed in
 *pixels.          *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Commented *
 *=============================================================================================*/
uint16_t Get_Build_Frame_Width(const void* dataptr) {
  if (dataptr != nullptr) {
    return static_cast<const KeyFrameHeaderType*>(dataptr)->width;
  }
  return 0;
}

/***********************************************************************************************
 * Get_Build_Frame_Height -- Fetches the height of the shape image. *
 *                                                                                             *
 *    Use this routine to fetch the height of the shapes within the keyframe
 *shape data block. * All shapes within the block have the same height. *
 *                                                                                             *
 * INPUT:   dataptr  -- Pointer to the keyframe shape data block. *
 *                                                                                             *
 * OUTPUT:  Returns with the height of the shapes in the block -- expressed in
 *pixels.         *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 06/25/1995 JLB : Commented *
 *=============================================================================================*/
uint16_t Get_Build_Frame_Height(const void* dataptr) {
  if (dataptr) {
    return static_cast<const KeyFrameHeaderType*>(dataptr)->height;
  }
  return 0;
}

bool Get_Build_Frame_Palette(const void* dataptr, void* palette) {
  if (dataptr && static_cast<const KeyFrameHeaderType*>(dataptr)->flags & 1) {
    const auto* ptr = static_cast<const char*>(Add_Long_To_Pointer(
        dataptr,
        static_cast<int32_t>(sizeof(uint32_t) << 1) *
                static_cast<const KeyFrameHeaderType*>(dataptr)->frames +
            16 + sizeof(KeyFrameHeaderType)));

    memcpy(palette, ptr, 768);
    return true;
  }
  return false;
}
