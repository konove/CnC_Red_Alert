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

/* $Header: /CounterStrike/LZOPIPE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZWPIPE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : July 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * LZWPipe::Flush -- Flushes any partially accumulated block. *
 *   LZWPipe::LZWPipe -- Constructor for the LZO processor pipe. * LZWPipe::Put
 *-- Send some data through the LZO processor pipe.                            *
 *   LZWPipe::~LZWPipe -- Deconstructor for the LZO pipe object. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/lzopipe.h"

#include <cstdint>
#include <cstring>
#include <utility>

#include "base/numeric.h"
#include "lzo/lzo.h"
#include "lzo/lzo1x.h"
#include "lzo/lzoconf.h"
#include "tech/codec_block.h"
#include "tech/pipe.h"

/***********************************************************************************************
 * LZOPipe::LZOPipe -- Constructor for the LZO processor pipe. *
 *                                                                                             *
 *    This will initialize the LZOPipe object so that it is prepared for
 *compression or        * decompression as indicated. *
 *                                                                                             *
 * INPUT:   decrypt  -- Should decompression be performed? *
 *                                                                                             *
 *          blocksize-- The size of the data blocks to process. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
LZOPipe::LZOPipe(CompControl control, int blocksize)
    : Control(control), BlockSize(blocksize), SafetyMargin(BlockSize) {
  Buffer.resize(base::ToSize(BlockSize + SafetyMargin));
  Buffer2.resize(base::ToSize(BlockSize + SafetyMargin));
  if (control == COMPRESS) {
    work_.resize(LZO1X_MEM_COMPRESS);
  }
}

/***********************************************************************************************
 * LZOPipe::Put -- Send some data through the LZO processor pipe. *
 *                                                                                             *
 *    This routine will take the data requested and process it (decompression or
 *compression). * It does this by accumulating the necessary bytes to make a
 *whole block. Then the block   * is processed and the entire contents are
 *flushed to the next pipe segment in the chain.  *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the data to be fed to this LZO processor. *
 *                                                                                             *
 *          length   -- The number of bytes received. *
 *                                                                                             *
 * OUTPUT:  Returns with the actual number of bytes output at the far distant
 *final link in    * the pipe chain. *
 *                                                                                             *
 * WARNINGS:   The compression process may be slow as well as consuming two
 *buffers.           *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
int LZOPipe::Put(const void* source, int slen) {
  if (source == nullptr || slen < 1) {
    return Pipe::Put(source, slen);
  }

  int total = 0;

  /*
  **	Copy as much as can fit into the buffer from the source data supplied.
  */
  if (Control == DECOMPRESS) {
    while (slen > 0 && !corrupt_) {
      /*
      **	First check to see if we are in the block header accumulation
      *phase. *	When a whole block header has been accumulated, only then will
      *the regular *	data processing begin for the block.
      */
      if (BlockHeader.CompCount == 0xFFFF) {
        const int needed =
            static_cast<int>(sizeof(BlockHeader)) - Counter;
        const int len = slen < needed ? slen : needed;
        memmove(Buffer.data() + Counter, source, base::ToSize(len));
        source = (char*)source + len;
        slen -= len;
        Counter += len;

        /*
        **	A whole block header has been accumulated. Store it for
        *safekeeping.
        */
        if (Counter == sizeof(BlockHeader)) {
          memmove(&BlockHeader, Buffer.data(), sizeof(BlockHeader));
          Counter = 0;
          // A corrupt header must not size writes past the staging buffers.
          if (!BlockHeaderFits(BlockHeader.CompCount, BlockHeader.UncompCount,
                               BlockSize + SafetyMargin)) {
            corrupt_ = true;
            break;
          }
        }
      }

      /*
      **	Fill the buffer with compressed data until there is enough to
      *make a whole *	data block.
      */
      if (slen > 0) {
        const int len = slen < BlockHeader.CompCount - Counter
                            ? slen
                            : BlockHeader.CompCount - Counter;

        memmove(Buffer.data() + Counter, source, base::ToSize(len));
        slen -= len;
        source = (char*)source + len;
        Counter += len;

        /*
        **	If an entire block has been accumulated, then uncompress it and
        *feed it *	through the pipe.
        */
        if (std::cmp_equal(Counter, BlockHeader.CompCount)) {
          auto length = static_cast<lzo_uint>(BlockSize + SafetyMargin);
          // The checked decoder keeps a corrupt payload inside both buffers.
          if (lzo1x_decompress_safe(Buffer.data(), BlockHeader.CompCount,
                                    Buffer2.data(), &length,
                                    nullptr) != LZO_E_OK ||
              std::cmp_not_equal(length, BlockHeader.UncompCount)) {
            Counter = 0;
            corrupt_ = true;
            break;
          }
          total += Pipe::Put(Buffer2.data(), BlockHeader.UncompCount);
          Counter = 0;
          BlockHeader.CompCount = 0xFFFF;
        }
      }
    }

  } else {
    /*
    **	If the buffer already contains some data, then any new data must be
    *stored *	into the staging buffer until a full set has been accumulated.
    */
    if (Counter > 0) {
      const int tocopy =
          slen < BlockSize - Counter ? slen : BlockSize - Counter;
      memmove(Buffer.data() + Counter, source, base::ToSize(tocopy));
      source = (char*)source + tocopy;
      slen -= tocopy;
      Counter += tocopy;

      if (Counter == BlockSize) {
        auto len = static_cast<lzo_uint>(BlockSize + SafetyMargin);
        lzo1x_1_compress(Buffer.data(), static_cast<lzo_uint>(BlockSize),
                         Buffer2.data(), &len, work_.data());
        BlockHeader.CompCount = static_cast<uint16_t>(len);
        BlockHeader.UncompCount = static_cast<uint16_t>(BlockSize);
        total += Pipe::Put(&BlockHeader, sizeof(BlockHeader));
        total += Pipe::Put(Buffer2.data(), static_cast<int>(len));
        Counter = 0;
      }
    }

    /*
    **	Process the source data in whole block chunks until there is
    *insufficient *	source data left for a whole data block.
    */
    while (slen >= BlockSize) {
      auto len = static_cast<lzo_uint>(BlockSize + SafetyMargin);
      lzo1x_1_compress(static_cast<const unsigned char*>(source),
                       static_cast<lzo_uint>(BlockSize), Buffer2.data(), &len,
                       work_.data());
      source = (char*)source + BlockSize;
      slen -= BlockSize;

      BlockHeader.CompCount = static_cast<uint16_t>(len);
      BlockHeader.UncompCount = static_cast<uint16_t>(BlockSize);
      total += Pipe::Put(&BlockHeader, sizeof(BlockHeader));
      total += Pipe::Put(Buffer2.data(), static_cast<int>(len));
    }

    /*
    **	If there is any remaining data, then it is stored into the buffer
    **	until a full data block has been accumulated.
    */
    if (slen > 0) {
      memmove(Buffer.data(), source, base::ToSize(slen));
      Counter = slen;
    }
  }

  return total;
}

/***********************************************************************************************
 * LZOPipe::Flush -- Flushes any partially accumulated block. *
 *                                                                                             *
 *    This routine is called when any buffered data must be flushed out the
 *pipe. For the      * compression process, this will generate the sub-sized
 *compressed block. For              * decompression, this routine should not
 *have any data in the buffer. In such a case, it   * means that the data source
 *was prematurely truncated. In such a case, just dump the      * accumulated
 *data through the pipe.                                                       *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the actual number of data bytes output to the distant
 *final link in   * the pipe chain. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/04/1996 JLB : Created. *
 *=============================================================================================*/
int LZOPipe::Flush() {
  int total = 0;

  /*
  **	If there is accumulated data, then it must processed.
  */
  if (Counter > 0) {
    if (Control == DECOMPRESS) {
      /*
      **	If the accumulated data is insufficient to make a block header,
      *then *	this means the data has been truncated. Just dump the data
      *through *	as if were already decompressed.
      */
      if (BlockHeader.CompCount == 0xFFFF) {
        total += Pipe::Put(Buffer.data(), Counter);
        Counter = 0;
      }

      /*
      **	There appears to be a partial block accumulated in the buffer.
      *It would *	be disastrous to try to decompress the data since there
      *wouldn't be *	the special end of data code that LZO decompression
      *needs. In this *	case, dump the data out as if it were already
      *decompressed.
      */
      if (Counter > 0) {
        total += Pipe::Put(&BlockHeader, sizeof(BlockHeader));
        total += Pipe::Put(Buffer.data(), Counter);
        Counter = 0;
        BlockHeader.CompCount = 0xFFFF;
      }

    } else {
      /*
      **	A partial block in the compression process is a normal
      *occurrence. Just *	compress the partial block and output normally.
      */
      auto len = static_cast<lzo_uint>(BlockSize + SafetyMargin);
      lzo1x_1_compress(Buffer.data(), static_cast<lzo_uint>(Counter),
                       Buffer2.data(), &len, work_.data());
      BlockHeader.CompCount = static_cast<uint16_t>(len);
      BlockHeader.UncompCount = static_cast<uint16_t>(Counter);
      total += Pipe::Put(&BlockHeader, sizeof(BlockHeader));
      total += Pipe::Put(Buffer2.data(), static_cast<int>(len));
      Counter = 0;
    }
  }

  total += Pipe::Flush();
  return total;
}
