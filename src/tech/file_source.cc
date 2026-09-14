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

/* $Header: /CounterStrike/XSTRAW.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : XSTRAW.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/04/96 *
 *                                                                                             *
 *                  Last Update : July 4, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * SpanSource::Get -- Fetch data from the straw's buffer holding
 *tank.                      * FileSource::Get -- Fetch data from the file. *
 *   FileSource::~FileSource -- The destructor for the file straw. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/file_source.h"

#include <cstddef>
#include <cstring>
#include <span>

#include "base/types.h"
#include "sdllib/file_access.h"

//---------------------------------------------------------------------------------------------------------
// FileSource
//---------------------------------------------------------------------------------------------------------

/***********************************************************************************************
 * FileSource::Get -- Fetch data from the file. *
 *                                                                                             *
 *    This routine will read data from the file (as specified in the straw's
 *constructor) into * the buffer indicated. *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the buffer to hold the data. *
 *                                                                                             *
 *          length   -- The number of bytes requested. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes stored into the buffer. If this
 *number is less    * than the number requested, then this indicates that the
 *file is exhausted.         *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
base::ssize FileSource::Read(std::span<std::byte> buffer) {
  if (!ok() || buffer.empty()) {
    return 0;
  }
  if (!Valid_File()) {
    Fail();
    return 0;
  }
  if (!file_->IsOpen()) {
    HasOpened = true;
    if (!file_->IsAvailable() || !file_->Open(FileAccess::kRead)) {
      Fail();
      return 0;
    }
  }
  const base::ssize count = file_->Read(buffer);
  if (!file_->ok()) {
    Fail();
  }
  return count;
}

/***********************************************************************************************
 * FileSource::~FileSource -- The destructor for the file straw. *
 *                                                                                             *
 *    This destructor only needs to close the file if it was the one to open it.
 **
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  none *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
FileSource::~FileSource() {
  if (Valid_File() && HasOpened) {
    file_->Close();
    HasOpened = false;
    file_ = nullptr;
  }
}
