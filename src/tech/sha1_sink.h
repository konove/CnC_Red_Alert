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

/* $Header: /CounterStrike/SHAPIPE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHAPIPE.H *
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

#ifndef CNC_RED_ALERT_TECH_SHA1_SINK_H_
#define CNC_RED_ALERT_TECH_SHA1_SINK_H_

#include <cstddef>
#include <span>

#include "base/types.h"
#include "tech/byte_sink.h"
#include "tech/sha.h"

/*
**	This class serves as a pipe that generates a Secure Hash from the data
*stream that flows *	through it. It doesn't modify the data stream in any
*fashion.
*/
class Sha1Sink : public ChainedSink {
 public:
  explicit Sha1Sink(ByteSink& next) : ChainedSink(next) {}
  ~Sha1Sink() override = default;

  Sha1Sink(const Sha1Sink&) = delete;
  Sha1Sink& operator=(const Sha1Sink&) = delete;
  Sha1Sink(Sha1Sink&&) = delete;
  Sha1Sink& operator=(Sha1Sink&&) = delete;

  bool Write(std::span<const std::byte> bytes) override;

  // Returns the digest of the data that has passed this link so far.
  [[nodiscard]] Sha1Digest digest() const { return SHA.Digest(); }

 protected:
  SHAEngine SHA;
};

#endif  // CNC_RED_ALERT_TECH_SHA1_SINK_H_
