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

/* $Header: /CounterStrike/LZoSTRAW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZOSTRAW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 07/02/96 *
 *                                                                                             *
 *                  Last Update : July 2, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_LZOSTRAW_H_
#define CNC_RED_ALERT_TECH_LZOSTRAW_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/types.h"
#include "tech/straw.h"

/*
**	This class handles LZO compression/decompression to the data stream that
*is drawn through *	this class. Note that for compression, two internal
*buffers are required. For decompression *	only one buffer is required.
*This changes the memory footprint of this class depending on *	the process
*desired.
*/
class LZOStraw : public ChainedStraw {
 public:
  typedef enum CompControl { COMPRESS, DECOMPRESS } CompControl;

  // source must outlive this straw.
  LZOStraw(CompControl control, Straw& source, int blocksize = 1024 * 8);
  ~LZOStraw() override = default;

  base::ssize Get(std::span<std::byte> buffer) override;

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
    uint16_t CompCount = 0;    // Size of data block (compressed).
    uint16_t UncompCount = 0;  // Bytes of uncompressed data it represents.
  } BlockHeader;

  // Set once the stream yields a block that cannot be decoded safely; all
  // later data is dropped.
  bool corrupt_ = false;

  // The LZO compressor's dictionary; empty when decompressing.
  std::vector<unsigned char> work_;

  // Holds one compressed block while it is decoded; empty when compressing.
  std::vector<unsigned char> staging_;

 public:
  LZOStraw(const LZOStraw&) = delete;
  LZOStraw& operator=(const LZOStraw&) = delete;
  LZOStraw(LZOStraw&&) = delete;
  LZOStraw& operator=(LZOStraw&&) = delete;
};

#endif  // CNC_RED_ALERT_TECH_LZOSTRAW_H_
