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

/* $Header: /CounterStrike/LZOSTRAW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZOSTRAW.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/02/96 *
 *                                                                                             *
 *                  Last Update : July 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LZOStraw::Get -- Fetch data through the LZO processor. *
 *   LZOStraw::LZOStraw -- Constructor for LZO straw object. *
 *   LZOStraw::~LZOStraw -- Destructor for the LZO straw. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/lzostraw.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "base/numeric.h"
#include "lzo/lzo.h"
#include "lzo/lzo1x.h"
#include "lzo/lzoconf.h"
#include "tech/codec_block.h"
#include "tech/straw.h"

/***********************************************************************************************
 * LZOStraw::LZOStraw -- Constructor for LZO straw object. *
 *                                                                                             *
 *    This will initialize the LZO straw object. Whether the object is to
 *compress or          * decompress and the block size to use is specified. The
 *data is compressed in blocks      * that are sized to be quick to compress and
 *yet still yield good compression ratios.      *
 *                                                                                             *
 * INPUT:   decrypt  -- Should the data be decompressed? *
 *                                                                                             *
 *          blocksize-- The size of the blocks to process. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   It takes two buffers of the blocksize specified if compression is
 *to be         * performed. *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
LZOStraw::LZOStraw(CompControl control, int blocksize)
    : Control(control), BlockSize(blocksize), SafetyMargin(BlockSize) {
  Buffer = new unsigned char[base::ToSize(BlockSize + SafetyMargin)];
  if (control == COMPRESS) {
    Buffer2 = new unsigned char[base::ToSize(BlockSize + SafetyMargin)];
  }
}

/***********************************************************************************************
 * LZOStraw::~LZOStraw -- Destructor for the LZO straw. *
 *                                                                                             *
 *    The destructor will free up the allocated buffers that it allocated in the
 *constructor.  *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
LZOStraw::~LZOStraw() {
  delete[] Buffer;
  Buffer = nullptr;

  delete[] Buffer2;
  Buffer2 = nullptr;
}

/***********************************************************************************************
 * LZOStraw::Get -- Fetch data through the LZO processor. *
 *                                                                                             *
 *    This routine will fetch the data bytes specified. It does this by first
 *accumulating     * a full block of data and then compressing or decompressing
 *it as indicated. Subsequent   * requests for data will draw from this buffer
 *of processed data until it is exhausted     * and another block must be
 *fetched.                                                       *
 *                                                                                             *
 * INPUT:   destbuf  -- Pointer to the buffer to hold the data requested. *
 *                                                                                             *
 *          length   -- The number of data bytes requested. *
 *                                                                                             *
 * OUTPUT:  Returns with the actual number of bytes stored into the buffer. If
 *this number     * is less than that requested, then this indicates that the
 *data source has been     * exhausted. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
int LZOStraw::Get(void* destbuf, int slen) {
  assert(Buffer != nullptr);

  int total = 0;

  /*
  **	Verify parameters for legality.
  */
  if (destbuf == nullptr || slen < 1) {
    return 0;
  }

  while (slen > 0) {
    /*
    **	Copy as much data is requested and available into the desired
    **	destination buffer.
    */
    if (Counter) {
      const int len = slen < Counter ? slen : Counter;
      if (Control == DECOMPRESS) {
        memmove(destbuf, &Buffer[BlockHeader.UncompCount - Counter],
                base::ToSize(len));
      } else {
        memmove(destbuf,
                &Buffer2[BlockHeader.CompCount +
                         static_cast<int>(sizeof(BlockHeader)) - Counter],
                base::ToSize(len));
      }
      destbuf = static_cast<char*>(destbuf) + len;
      slen -= len;
      Counter -= len;
      total += len;
    }
    if (slen == 0) {
      break;
    }

    if (Control == DECOMPRESS) {
      if (corrupt_) {
        break;
      }
      int incount = Straw::Get(&BlockHeader, sizeof(BlockHeader));
      if (incount != sizeof(BlockHeader)) {
        break;
      }

      // A corrupt header must not size reads or writes past Buffer.
      if (!BlockHeaderFits(BlockHeader.CompCount, BlockHeader.UncompCount,
                           BlockSize + SafetyMargin)) {
        corrupt_ = true;
        break;
      }

      std::vector<unsigned char> staging(BlockHeader.CompCount);
      incount = Straw::Get(staging.data(), BlockHeader.CompCount);
      if (std::cmp_not_equal(incount, BlockHeader.CompCount)) {
        break;
      }
      // Buffer is a pointer; pass its allocated capacity, not sizeof.
      auto length = static_cast<lzo_uint>(BlockSize + SafetyMargin);
      // The checked decoder keeps a corrupt payload inside both buffers.
      if (lzo1x_decompress_safe(staging.data(), BlockHeader.CompCount, Buffer,
                                &length, nullptr) != LZO_E_OK ||
          std::cmp_not_equal(length, BlockHeader.UncompCount)) {
        corrupt_ = true;
        break;
      }
      Counter = BlockHeader.UncompCount;
    } else {
      BlockHeader.UncompCount =
          static_cast<uint16_t>(Straw::Get(Buffer, BlockSize));
      if (BlockHeader.UncompCount == 0) {
        break;
      }
      // The compressor indexes 16384 pointers, so a fixed 64K dictionary
      // overflowed on 64-bit hosts.
      char* dictionary = new char[LZO1X_MEM_COMPRESS];
      lzo_uint length = static_cast<lzo_uint>(BlockSize + SafetyMargin) -
                        lzo_uint{sizeof(BlockHeader)};
      lzo1x_1_compress(Buffer, BlockHeader.UncompCount,
                       &Buffer2[sizeof(BlockHeader)], &length,
                       dictionary);
      BlockHeader.CompCount = static_cast<uint16_t>(length);
      delete[] dictionary;
      memmove(Buffer2, &BlockHeader, sizeof(BlockHeader));
      Counter = static_cast<int>(BlockHeader.CompCount + sizeof(BlockHeader));
    }
  }

  return total;
}
