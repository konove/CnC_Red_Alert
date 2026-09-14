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

/* $Header: /CounterStrike/LZOPIPE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZOPIPE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/30/96 *
 *                                                                                             *
 *                  Last Update : June 30, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_LZOPIPE_H_
#define CNC_RED_ALERT_TECH_LZOPIPE_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"
#include "tech/pipe.h"

/*
**	Performs LZO compression/decompression on the data stream that is piped
*through this *	class. The data is compressed in blocks so of small enough size
*to be compressed *	quickly and large enough size to get decent compression
*rates.
*/
class LZOPipe : public Pipe {
 public:
  typedef enum CompControl { COMPRESS, DECOMPRESS } CompControl;

  explicit LZOPipe(CompControl /*control*/, int blocksize = 1024 * 8);
  ~LZOPipe() override = default;

  bool Flush() override;
  bool Put(std::span<const std::byte> bytes) override;

 private:
  /*
  **	This tells the pipe if it should be decompressing or compressing the
  *data stream.
  */
  CompControl Control;

  /*
  **	The number of bytes accumulated into the staging buffer.
  */
  int Counter = 0;

  /*
  **	Working buffers that compression/decompression will use.
  */
  std::vector<unsigned char> Buffer;
  std::vector<unsigned char> Buffer2;

  /*
  **	The working block size. Data will be compressed in chunks of this size.
  */
  int BlockSize;

  /*
  **	Probably dont need this anymore as LZO decompresses into a staging
  *buffer.
  */
  int SafetyMargin;

  /*
  **	Each block has a header of this format.
  */
  struct {
    uint16_t CompCount = 0xFFFF;  // Size of data block (compressed).
    uint16_t UncompCount = 0;     // Bytes of uncompressed data it represents.
  } BlockHeader;

  // Set once the stream yields a block that cannot be decoded safely; all
  // later data is dropped.
  bool corrupt_ = false;

  // The LZO compressor's dictionary; empty when decompressing.
  std::vector<unsigned char> work_;

 public:
  LZOPipe(const LZOPipe&) = delete;
  LZOPipe& operator=(const LZOPipe&) = delete;
  LZOPipe(LZOPipe&&) = delete;
  LZOPipe& operator=(LZOPipe&&) = delete;
};

#endif  // CNC_RED_ALERT_TECH_LZOPIPE_H_
