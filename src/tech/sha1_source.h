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

/* $Header: /CounterStrike/SHASTRAW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHASTRAW.H *
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

#ifndef CNC_RED_ALERT_TECH_SHA1_SOURCE_H_
#define CNC_RED_ALERT_TECH_SHA1_SOURCE_H_

#include <cstddef>
#include <span>

#include "base/types.h"
#include "tech/byte_source.h"
#include "tech/sha.h"

/*
**	This class serves as a straw that generates a Secure Hash from the data
*stream that flows *	through it. It doesn't modify the data stream in any
*fashion.
*/
class Sha1Source : public ChainedSource {
 public:
  explicit Sha1Source(ByteSource& source) : ChainedSource(source) {}
  ~Sha1Source() override = default;

  Sha1Source(const Sha1Source&) = delete;
  Sha1Source& operator=(const Sha1Source&) = delete;
  Sha1Source(Sha1Source&&) = delete;
  Sha1Source& operator=(Sha1Source&&) = delete;

  base::ssize Read(std::span<std::byte> buffer) override;

  // Returns the digest of the data that has passed this link so far.
  [[nodiscard]] Sha1Digest digest() const { return SHA.Digest(); }

 protected:
  SHAEngine SHA;
};

#endif  // CNC_RED_ALERT_TECH_SHA1_SOURCE_H_
