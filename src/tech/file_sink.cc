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

/* $Header: /CounterStrike/XPIPE.CPP 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : XPIPE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/04/96 *
 *                                                                                             *
 *                  Last Update : July 5, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * SpanSink::Put -- Submit data to the buffered pipe segment. *
 *   FileSink::Put -- Submit a block of data to the pipe. * FileSink::Finish --
 * Finish the file pipe handler.                                               *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/file_sink.h"

#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>

#include "sdllib/file_access.h"

//---------------------------------------------------------------------------------------------------------
// FileSink
//---------------------------------------------------------------------------------------------------------

FileSink::~FileSink() {
  if (Valid_File() && HasOpened) {
    HasOpened = false;
    file_->Close();
    file_ = nullptr;
  }
}

/***********************************************************************************************
 * FileSink::Finish -- Finish the file pipe handler. *
 *                                                                                             *
 *    This routine is called when there will be no more data sent through the
 *pipe. It is      * responsible for cleaning up anything it needs to. This is
 *not handled by the             * destructor, although it serves a similar
 *purpose, because pipe are linked together and   * the destructor order is not
 *easily controlled. If the destructors for a pipe chain were  * called out of
 *order, the result might be less than pleasant.                             *
 *                                                                                             *
 * INPUT:   none *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes flushed out the final end of the
 *pipe as a        * consequence of this routine. *
 *                                                                                             *
 * WARNINGS:   Don't send any more data through the pipe after this routine is
 *called.         *
 *                                                                                             *
 * HISTORY: * 07/05/1996 JLB : Created. *
 *=============================================================================================*/
bool FileSink::Finish() {
  const bool result = ok();
  if (Valid_File() && HasOpened) {
    HasOpened = false;
    file_->Close();
  }
  return result;
}

/***********************************************************************************************
 * FileSink::Put -- Submit a block of data to the pipe. *
 *                                                                                             *
 *    Takes the data block submitted and writes it to the file. If the file was
 *not already    * open, this routine will open it for write. *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the data to submit to the file. *
 *                                                                                             *
 *          length   -- The number of bytes to write to the file. *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes written to the file. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool FileSink::Write(std::span<const std::byte> bytes) {
  if (!ok()) {
    return false;
  }
  if (bytes.empty()) {
    return true;
  }
  if (!Valid_File()) {
    Fail();
    return false;
  }
  if (!file_->IsOpen()) {
    HasOpened = true;
    if (!file_->Open(FileAccess::kWrite)) {
      Fail();
      return false;
    }
  }
  if (file_->Write(bytes) != std::ssize(bytes)) {
    Fail();
  }
  return ok();
}
