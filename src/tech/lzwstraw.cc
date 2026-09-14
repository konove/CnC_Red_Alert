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

/* $Header: /CounterStrike/LZWSTRAW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZWSTRAW.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/02/96 *
 *                                                                                             *
 *                  Last Update : July 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LZWStraw::Get -- Fetch data through the LZW processor. *
 *   LZWStraw::LZWStraw -- Constructor for LZW straw object. *
 *   LZWStraw::~LZWStraw -- Destructor for the LZW straw. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/lzwstraw.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "base/numeric.h"
#include "tech/buff.h"
#include "tech/codec_block.h"
#include "tech/lzw.h"
#include "tech/straw.h"

/***********************************************************************************************
 * LZWStraw::LZWStraw -- Constructor for LZW straw object. *
 *                                                                                             *
 *    This will initialize the LZW straw object. Whether the object is to
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
LZWStraw::LZWStraw(CompControl control, int blocksize)
    : Control(control),
      BlockSize(blocksize),
      // Room for an incompressible block plus the header the straw stores in
      // front of it.
      SafetyMargin(LzwWorstCaseSize(BlockSize) - BlockSize +
                   static_cast<int>(sizeof(BlockHeader))) {
  //	SafetyMargin = BlockSize/128+1;
  source_buffer_.resize(base::ToSize(BlockSize + SafetyMargin));
  if (control == COMPRESS) {
    output_buffer_.resize(base::ToSize(BlockSize + SafetyMargin));
  }
}

/***********************************************************************************************
 * LZWStraw::Get -- Fetch data through the LZW processor. *
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
int LZWStraw::Get(void* destbuf, int slen) {
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
        memmove(destbuf,
                source_buffer_.data() + (BlockHeader.UncompCount - Counter),
                base::ToSize(len));
      } else {
        memmove(destbuf,
                output_buffer_.data() +
                    (BlockHeader.CompCount +
                     static_cast<int>(sizeof(BlockHeader)) - Counter),
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

      void* ptr = source_buffer_.data() +
                  (BlockSize + SafetyMargin - BlockHeader.CompCount);
      incount = Straw::Get(ptr, BlockHeader.CompCount);
      if (std::cmp_not_equal(incount, BlockHeader.CompCount)) {
        break;
      }

      // Sized buffers stop a corrupt code stream from reading or writing
      // past source_buffer_.
      const int produced = LZW_Uncompress(
          Buffer(ptr, BlockHeader.CompCount),
          Buffer(source_buffer_.data(), BlockSize + SafetyMargin));
      if (std::cmp_not_equal(produced, BlockHeader.UncompCount)) {
        corrupt_ = true;
        break;
      }
      Counter = BlockHeader.UncompCount;
    } else {
      // Compress
      BlockHeader.UncompCount =
          static_cast<uint16_t>(Straw::Get(source_buffer_.data(), BlockSize));
      if (BlockHeader.UncompCount == 0) {
        break;
      }
      BlockHeader.CompCount = static_cast<uint16_t>(
          LZW_Compress(Buffer(source_buffer_.data(), BlockHeader.UncompCount),
                       Buffer(output_buffer_.data() + sizeof(BlockHeader),
                              BlockSize + SafetyMargin -
                                  static_cast<int>(sizeof(BlockHeader)))));
      memmove(output_buffer_.data(), &BlockHeader, sizeof(BlockHeader));
      Counter = static_cast<int>(BlockHeader.CompCount + sizeof(BlockHeader));
    }
  }

  return total;
}
