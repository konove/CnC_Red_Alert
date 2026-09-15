/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\keyframe.cpv   2.14   16 Oct 1995
 * 16:48:54   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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

#include "td/keyframe.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "base/types.h"
#include "sdllib/iff.h"
#include "sdllib/memflag.h"
#include "sdllib/wsa.h"
#include "td/defines.h"
#include "td/externs.h"
#include "tech/2keyfbuf.h"

#define SUBFRAMEOFFS 7  // 3 1/2 frame offsets loaded (2 offsets/frame)

#define Apply_Delta(buffer, delta) \
  Apply_XOR_Delta(static_cast<char*>(buffer), static_cast<const char*>(delta))

typedef struct {
  uint16_t frames;
  uint16_t x;
  uint16_t y;
  uint16_t width;
  uint16_t height;
  uint16_t largest_frame_size;
  int16_t flags;
} KeyFrameHeaderType;

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

// Only the uncompressed-shape cache recorded a frame length, so this stays 0.
static int Length;

int Get_Last_Frame_Length() { return Length; }

void* Build_Frame(const void* dataptr, uint16_t framenumber, void* buffptr) {
  const char* ptr;
  uint32_t offset[SUBFRAMEOFFS];
  // Offsets into the 24-bit frame data, so int32_t never overflows.
  int32_t offcurr;
  int32_t offdiff;
  uint16_t buffsize;
  uint16_t currframe = 0;
  uint16_t subframe;
  char frameflags;

  //
  // valid pointer??
  //
  Length = 0;
  if (!dataptr || !buffptr) {
    return nullptr;
  }

  //
  // look at header then check that frame to build is not greater
  // than total frames
  //
  const auto* keyfr = static_cast<const KeyFrameHeaderType*>(dataptr);

  if (framenumber >= keyfr->frames) {
    return nullptr;
  }

  // calc buff size
  buffsize = keyfr->width * keyfr->height;

  // get offset into data
  ptr = static_cast<const char*>(Add_Long_To_Pointer(
      dataptr, (int32_t{framenumber} << 3) + kKeyFrameHeaderSize));
  Mem_Copy(ptr, &offset[0], 12L);
  frameflags = static_cast<char>(offset[0] >> 24);

  if (frameflags & KF_KEYFRAME) {
    ptr = static_cast<const char*>(
        Add_Long_To_Pointer(dataptr, offset[0] & 0x00FFFFFFL));

    if (keyfr->flags & 1) {
      ptr = static_cast<const char*>(Add_Long_To_Pointer(ptr, 768L));
    }
    LCW_Uncompress(ptr, buffptr, buffsize);
  } else {  // key delta or delta

    if (frameflags & KF_DELTA) {
      currframe = static_cast<uint16_t>(offset[1]);

      ptr = static_cast<const char*>(Add_Long_To_Pointer(
          dataptr, (int32_t{currframe} << 3) + kKeyFrameHeaderSize));
      Mem_Copy(ptr, &offset[0], SUBFRAMEOFFS * sizeof(uint32_t));
    }

    // key frame
    offcurr = static_cast<int32_t>(offset[1] & 0x00FFFFFF);

    // key delta
    offdiff = static_cast<int32_t>(offset[0] & 0x00FFFFFF) - offcurr;

    ptr = static_cast<const char*>(Add_Long_To_Pointer(dataptr, offcurr));

    if (keyfr->flags & 1) {
      ptr = static_cast<const char*>(Add_Long_To_Pointer(ptr, 768L));
    }

    const int32_t length = LCW_Uncompress(ptr, buffptr, buffsize);

    if (std::cmp_greater(length, buffsize)) {
      return nullptr;
    }

    // The DOS build rebased ptr whenever the next delta crossed a 64K
    // segment. ptr + offdiff is the same address either way, so a flat
    // address space needs no rebasing.
    Apply_Delta(buffptr, Add_Long_To_Pointer(ptr, offdiff));

    if (frameflags & KF_DELTA) {
      // adjust to delta after the keydelta

      currframe++;
      subframe = 2;

      while (currframe <= framenumber) {
        offdiff = static_cast<int32_t>(offset[subframe] & 0x00FFFFFF) - offcurr;

        Apply_Delta(buffptr, Add_Long_To_Pointer(ptr, offdiff));

        currframe++;
        subframe += 2;

        if (subframe >= SUBFRAMEOFFS - 1 && currframe <= framenumber) {
          Mem_Copy(Add_Long_To_Pointer(dataptr, (int32_t{currframe} << 3) +
                                                    kKeyFrameHeaderSize),
                   &offset[0], SUBFRAMEOFFS * sizeof(uint32_t));
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
  if (dataptr) {
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
    const char* ptr = static_cast<const char*>(Add_Long_To_Pointer(
        dataptr, ((static_cast<int32_t>(sizeof(uint32_t)) << 1) *
                  static_cast<const KeyFrameHeaderType*>(dataptr)->frames) +
                     16 + sizeof(KeyFrameHeaderType)));

    memcpy(palette, ptr, 768L);
    return true;
  }
  return false;
}
