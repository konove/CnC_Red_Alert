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

/* $Header: /CounterStrike/SHASTRAW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHASTRAW.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/02/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * SHAStraw::Get -- Fetch data from the straw and process the SHA
 *with the data.             * digest.                                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/shastraw.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/types.h"
#include "tech/straw.h"

/***********************************************************************************************
 * SHAStraw::Get -- Fetch data from the straw and process the SHA with the data.
 **
 *                                                                                             *
 *    This routine will fetch the requested data and as it passes through this
 *straw it will   * submit it to the SHA processor. The data that passes through
 *is unmodified by this       * straw segment. *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the buffer that will hold the requested data.
 **
 *                                                                                             *
 *          length   -- The length of the data requested. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes stored in the buffer. If this
 *number is less      * than the number requested, then this indicates that the
 *data stream has been       * exhausted. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
base::ssize SHAStraw::Get(std::span<std::byte> buffer) {
  if (buffer.empty()) {
    return 0;
  }

  const base::ssize counter = ChainedStraw::Get(buffer);
  SHA.Hash(buffer.data(), static_cast<int32_t>(counter));
  return counter;
}

