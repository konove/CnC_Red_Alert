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

/* $Header: /CounterStrike/B64STRAW.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : B64STRAW.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/02/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * Base64Source::Get -- Fetch data and convert it to/from base 64
 *encoding.                   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/base64_source.h"

#include <cstddef>
#include <cstring>
#include <span>

#include "base/numeric.h"
#include "base/types.h"
#include "tech/base64.h"
#include "tech/byte_source.h"
#include "tech/byte_view.h"

/***********************************************************************************************
 * Base64Source::Get -- Fetch data and convert it to/from base 64 encoding. *
 *                                                                                             *
 *    This routine will fetch the number of bytes requested and perform any
 *conversion as      * necessary upon the data. The nature of Base 64 encoding
 *means that the data will         * increase in size by 30% when encoding and
 *decrease in like manner when decoding.         *
 *                                                                                             *
 * INPUT:   source   -- The buffer to hold the processed data. *
 *                                                                                             *
 *          length   -- The number of bytes requested. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes stored into the buffer. If the
 *number is less     * than requested, then this indicates that the data stream
 *has been exhausted.       *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
base::ssize Base64Source::Read(std::span<std::byte> buffer) {
  void* source = buffer.data();
  int slen = static_cast<int>(buffer.size());
  base::ssize total = 0;

  char* from;
  int fromsize;
  char* to;
  int tosize;

  if (Control == Base64Mode::kEncode) {
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

  /*
  **	Process the byte request in code blocks until there are either
  **	no more source bytes available or the request has been fulfilled.
  */
  while (slen > 0) {
    /*
    **	Transfer any processed bytes available to the request buffer.
    */
    if (Counter > 0) {
      const int len = slen < Counter ? slen : Counter;
      memmove(source, &to[tosize - Counter], base::ToSize(len));
      Counter -= len;
      slen -= len;
      source = static_cast<char*>(source) + len;
      total += len;
    }
    if (slen == 0) {
      break;
    }

    /*
    **	More bytes are needed, so fetch and process another base 64 block.
    */
    const int incount =
        static_cast<int>(ChainedSource::Read(WritableByteView(from, fromsize)));
    if (Control == Base64Mode::kEncode) {
      Counter = Base64_Encode(from, incount, to, tosize);
    } else {
      Counter = Base64_Decode(from, incount, to, tosize);
    }
    if (Counter == 0) {
      break;
    }
    // Pending bytes are drained from the end of the scratch buffer above.
    // Decoding a padded final group produces only one or two bytes at its
    // start.
    if (Counter < tosize) {
      memmove(to + tosize - Counter, to, base::ToSize(Counter));
    }
  }

  return total;
}
