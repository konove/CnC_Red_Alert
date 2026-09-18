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

/* $Header: /CounterStrike/SHA.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHA.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/03/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * SHAEngine::Digest -- Fetch the current digest. * SHAEngine::Hash
 *-- Process an arbitrarily long data block.                                *
 *   SHAEngine::Process_Partial -- Helper routine to process any partially
 *accumulated data blo*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */
#include "tech/sha.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "port/unaligned.h"
#include "tech/sha1_compress.h"

/***********************************************************************************************
 * SHAEngine::Process_Partial -- Helper routine to process any partially
 *accumulated data bloc *
 *                                                                                             *
 *    This routine will see if there is a partial block already accumulated in
 *the holding     * buffer. If so, then the data is fetched from the source such
 *that a full buffer is       * accumulated and then processed. If there is
 *insufficient data to fill the buffer, then   * it accumulates what data it can
 *and then returns so that this routine can be called      * again later. *
 *                                                                                             *
 * INPUT:   data  -- Reference to a pointer to the data. This pointer will be
 *modified if      * this routine consumes any of the data in the buffer. *
 *                                                                                             *
 *          length-- Reference to the length of the data available. If this
 *routine consumes   * any of the data, then this length value will be modified.
 **
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
void SHAEngine::Process_Partial(std::span<const std::byte>& data) {
  if (data.empty() || (PartialCount == 0 && data.size() >= SRC_BLOCK_SIZE)) {
    return;
  }
  const auto count = std::min(
      data.size(), static_cast<std::size_t>(SRC_BLOCK_SIZE - PartialCount));
  base::CopyBytes(
      base::ObjectBytes(Partial).subspan(base::ToSize(PartialCount)), data,
      count);
  data = data.subspan(count);
  PartialCount += static_cast<int>(count);
  if (PartialCount == SRC_BLOCK_SIZE) {
    tech::Sha1Compress(Acc, base::ObjectBytes(Partial));
    Length += SRC_BLOCK_SIZE;
    PartialCount = 0;
  }
}

/***********************************************************************************************
 * SHAEngine::Hash -- Process an arbitrarily long data block. *
 *                                                                                             *
 *    This is the main access routine to the SHA engine. It will take the
 *arbitrarily long     * data block and process it. The hash value is
 *accumulated with any previous calls to      * this routine. *
 *                                                                                             *
 * INPUT:   data     -- Pointer to the data block to process. *
 *                                                                                             *
 *          length   -- The number of bytes to process. *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
void SHAEngine::Hash(std::span<const std::byte> data) {
  IsCached = false;
  Process_Partial(data);
  // All whole blocks in one call, so the hardware path runs uninterrupted.
  const std::size_t whole = data.size() - (data.size() % SRC_BLOCK_SIZE);
  tech::Sha1Compress(Acc, data.first(whole));
  Length += static_cast<int32_t>(whole);
  data = data.subspan(whole);
  Process_Partial(data);
}

// Byte-swaps a 32-bit word.
static constexpr uint32_t Reverse_LONG(uint32_t a) {
  return ((a >> 24) & 0x000000FFU) | ((a >> 8) & 0x0000FF00U) |
         ((a << 8) & 0x00FF0000U) | ((a << 24) & 0xFF000000U);
}

/***********************************************************************************************
 * SHAEngine::Digest -- Fetch the current digest. *
 *                                                                                             *
 *    This routine will return the digest as it currently stands. *
 *                                                                                             *
 * INPUT:   pointer  -- Pointer to the buffer that will hold the digest -- 20
 *bytes.           *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes copied into the buffer. This will
 *always be       *
 *          20. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
Sha1Digest SHAEngine::Digest() const {
  /*
  **	If the final hash result has already been calculated for the
  **	current data state, then immediately return with the precalculated
  **	value.
  */
  if (IsCached) {
    return FinalResult;
  }

  const int32_t length = Length + PartialCount;
  int partialcount = PartialCount;
  char partial[SRC_BLOCK_SIZE];
  base::CopyBytes(base::ObjectBytes(partial), base::ObjectBytes(Partial),
                  sizeof(Partial));

  /*
  **	Cap the end of the source data stream with a 1 bit.
  */
  base::At(partial, partialcount) = static_cast<char>(0x80);

  /*
  **	Determine if there is insufficient room to append the
  **	data length number to the hash source. If not, then
  **	fill out the rest of the accumulator and flush it to
  **	the hash so that there will be room for the final
  **	count value.
  */
  Accumulator acc = Acc;
  if (SRC_BLOCK_SIZE - partialcount < 9) {
    if (partialcount + 1 < SRC_BLOCK_SIZE) {
      std::ranges::fill(base::Suffix(partial, partialcount + 1), '\0');
    }
    tech::Sha1Compress(acc, base::ObjectBytes(partial));
    partialcount = 0;
  } else {
    partialcount++;
  }

  /*
  **	Put the length of the source data as a 64 bit integer in the
  **	last 8 bytes of the pseudo-source data.
  */
  std::ranges::fill(base::Suffix(partial, partialcount), '\0');
  port::WriteUnaligned(base::ObjectBytes(partial).last(4),
                       Reverse_LONG(static_cast<uint32_t>(length * 8)));
  tech::Sha1Compress(acc, base::ObjectBytes(partial));

  // Each word is stored most significant byte first.
  for (std::size_t word = 0; word < acc.size(); ++word) {
    for (std::size_t byte = 0; byte < 4; ++byte) {
      FinalResult.at((word * 4) + byte) =
          static_cast<std::byte>(acc.at(word) >> (24U - (8U * byte)));
    }
  }
  IsCached = true;
  return FinalResult;
}
