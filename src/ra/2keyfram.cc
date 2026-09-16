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

#include <cstdint>
#include <cstring>

#include "base/types.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/keyframe.h"
#include "sdllib/iff.h"
#include "sdllib/memflag.h"
#include "sdllib/wsa.h"
#include "tech/2keyfbuf.h"

// 3 1/2 frame offsets loaded (2 offsets/frame).
constexpr int kSubFrameOffs = 7;

struct KeyFrameHeaderType {
  uint16_t frames;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t largest_frame_size;
  uint16_t flags;
};

// Byte offset of the frame offset table, which follows the header.
constexpr base::ssize kKeyFrameHeaderSize =
    base::ssize{sizeof(KeyFrameHeaderType)};

// The uncompressed-shape cache was never finished: nothing sets
// UseBigShapeBuffer, so Build_Frame always decodes into the caller's buffer.
// tech/2keyfbuf.cc still reads these three to choose its draw path, so the
// definitions stay.
char* BigShapeBufferStart = nullptr;
char* TheaterShapeBufferStart = nullptr;
bool UseBigShapeBuffer = false;
// Set by the type classes around Build_Frame for theater-specific shapes; only
// the uncompressed-shape cache ever read it.
bool IsTheaterShape = false;

void* Build_Frame(const void* dataptr, const uint16_t framenumber,
                  void* buffptr) {
  uint32_t offset[kSubFrameOffs];

  // valid pointer??
  if (!dataptr || !buffptr) {
    return nullptr;
  }

  // look at header then check that frame to build is not greater
  // than total frames
  const auto* keyfr = static_cast<const KeyFrameHeaderType*>(dataptr);

  if (framenumber >= keyfr->frames) {
    return nullptr;
  }

  // calc buff size
  const int buffsize = keyfr->width * keyfr->height;

  // get offset into data
  const auto* ptr = static_cast<const char*>(Add_Long_To_Pointer(
      dataptr, (base::ssize{framenumber} * 8) + kKeyFrameHeaderSize));
  Mem_Copy(ptr, &offset[0], 12);
  const auto frameflags = static_cast<uint8_t>(offset[0] >> 24);

  if (frameflags & KF_KEYFRAME) {
    ptr = static_cast<const char*>(
        Add_Long_To_Pointer(dataptr, offset[0] & 0x00FFFFFF));

    if (keyfr->flags & 1) {
      ptr = static_cast<const char*>(Add_Long_To_Pointer(ptr, 768));
    }
    LCW_Uncompress(ptr, buffptr, buffsize);
  } else {
    uint16_t currframe = 0;
    // key delta or delta

    if (frameflags & KF_DELTA) {
      currframe = static_cast<uint16_t>(offset[1]);

      ptr = static_cast<const char*>(Add_Long_To_Pointer(
          dataptr, (base::ssize{currframe} * 8) + kKeyFrameHeaderSize));
      Mem_Copy(ptr, &offset[0], kSubFrameOffs * sizeof(uint32_t));
    }

    // key frame
    const uint32_t offcurr = offset[1] & 0x00FFFFFF;

    // key delta
    uint32_t offdiff = (offset[0] & 0x00FFFFFF) - offcurr;

    ptr = static_cast<const char*>(Add_Long_To_Pointer(dataptr, offcurr));

    if (keyfr->flags & 1) {
      ptr = static_cast<const char*>(Add_Long_To_Pointer(ptr, 768));
    }

    const int32_t length = LCW_Uncompress(ptr, buffptr, buffsize);

    if (length > buffsize) {
      return nullptr;
    }

    Apply_XOR_Delta(
        static_cast<char*>(buffptr),
        static_cast<const char*>(Add_Long_To_Pointer(ptr, offdiff)));

    if (frameflags & KF_DELTA) {
      // adjust to delta after the keydelta

      currframe++;
      int subframe = 2;

      while (currframe <= framenumber) {
        offdiff = (offset[subframe] & 0x00FFFFFF) - offcurr;

        Apply_XOR_Delta(
            static_cast<char*>(buffptr),
            static_cast<const char*>(Add_Long_To_Pointer(ptr, offdiff)));

        currframe++;
        subframe += 2;

        if (subframe >= kSubFrameOffs - 1 && currframe <= framenumber) {
          Mem_Copy(Add_Long_To_Pointer(dataptr, (base::ssize{currframe} * 8) +
                                                    kKeyFrameHeaderSize),
                   &offset[0], kSubFrameOffs * sizeof(uint32_t));
          subframe = 0;
        }
      }
    }
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
        dataptr, (static_cast<int32_t>(sizeof(uint32_t) << 1) *
                  static_cast<const KeyFrameHeaderType*>(dataptr)->frames) +
                     16 + sizeof(KeyFrameHeaderType)));

    memcpy(palette, ptr, 768);
    return true;
  }
  return false;
}
