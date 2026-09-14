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

/* $Header: /CounterStrike/B64PIPE.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : B64PIPE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Base64Pipe::Put -- Processes a block of data through the pipe. *
 *   Base64Pipe::Flush -- Flushes the final pending data through the pipe. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/b64pipe.h"

#include <cstddef>
#include <cstring>
#include <span>

#include "base/numeric.h"
#include "tech/base64.h"
#include "tech/byte_view.h"
#include "tech/pipe.h"

/***********************************************************************************************
 * Base64Pipe::Put -- Processes a block of data through the pipe. *
 *                                                                                             *
 *    This will take the data submitted and either Base64 encode or decode it
 *(as specified    * in the pipe's constructor). The nature of Base64 encoding
 *means that the data will       * grow 30% in size when encoding and decrease
 *by a like amount when decoding.              *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the data to be translated. *
 *                                                                                             *
 *          length   -- The number of bytes to translate. *
 *                                                                                             *
 * OUTPUT:  Returns with the actual number of bytes output at the far distant
 *final end of     * the pipe chain. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool Base64Pipe::Put(std::span<const std::byte> bytes) {
  const void* source = bytes.data();
  int slen = static_cast<int>(bytes.size());
  if (source == nullptr || slen < 1) {
    return ChainedPipe::Put(bytes);
  }

  char* from;
  int fromsize;
  char* to;
  int tosize;

  if (Control == ENCODE) {
    from = PBuffer.data();
    fromsize = static_cast<int>(PBuffer.size());
    to = CBuffer.data();
    tosize = static_cast<int>(CBuffer.size());
  } else {
    from = CBuffer.data();
    fromsize = static_cast<int>(CBuffer.size());
    to = PBuffer.data();
    tosize = static_cast<int>(PBuffer.size());
  }

  if (Counter > 0) {
    const int len = slen < fromsize - Counter ? slen : fromsize - Counter;
    memmove(&from[Counter], source, base::ToSize(len));
    Counter += len;
    slen -= len;
    source = (char*)source + len;

    if (Counter == fromsize) {
      int outcount;
      if (Control == ENCODE) {
        outcount = Base64_Encode(from, fromsize, to, tosize);
      } else {
        outcount = Base64_Decode(from, fromsize, to, tosize);
      }
      ChainedPipe::Put(ByteView(to, outcount));
      Counter = 0;
    }
  }

  while (slen >= fromsize) {
    int outcount;
    if (Control == ENCODE) {
      outcount = Base64_Encode(source, fromsize, to, tosize);
    } else {
      outcount = Base64_Decode(source, fromsize, to, tosize);
    }
    source = (char*)source + fromsize;
    ChainedPipe::Put(ByteView(to, outcount));
    slen -= fromsize;
  }

  if (slen > 0) {
    memmove(from, source, base::ToSize(slen));
    Counter = slen;
  }

  return ok();
}

/***********************************************************************************************
 * Base64Pipe::Flush -- Flushes the final pending data through the pipe. *
 *                                                                                             *
 *    If there is any non-processed data accumulated in the holding buffer
 *(quite likely when  * encoding), then it will be processed and flushed out the
 *end of the pipe.                *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes output at the far distant final end
 *of the pipe   * chain. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool Base64Pipe::Flush() {
  if (Counter) {
    if (Control == ENCODE) {
      const int chars = Base64_Encode(PBuffer.data(), Counter, CBuffer.data(),
                                      static_cast<int>(CBuffer.size()));
      ChainedPipe::Put(ByteView(CBuffer.data(), chars));
    } else {
      const int chars = Base64_Decode(CBuffer.data(), Counter, PBuffer.data(),
                                      static_cast<int>(PBuffer.size()));
      ChainedPipe::Put(ByteView(PBuffer.data(), chars));
    }
    Counter = 0;
  }
  ChainedPipe::Flush();
  return ok();
}
