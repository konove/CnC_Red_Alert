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

/* $Header: /CounterStrike/BLOWPIPE.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : BLOWPIPE.H *
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

#ifndef CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_
#define CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_

#include <array>
#include <cstddef>
#include <optional>
#include <span>

#include "base/types.h"
#include "tech/blowfish.h"
#include "tech/byte_sink.h"

/*
**	Performs Blowfish encryption/decryption on the data stream that is piped
**	through this class.
*/
class BlowfishSink : public ChainedSink {
 public:
  BlowfishSink(CipherMode control, ByteSink& next)
      : ChainedSink(next), Control(control) {}
  ~BlowfishSink() override = default;
  bool Flush() override;

  bool Write(std::span<const std::byte> bytes) override;

  // Submit key for blowfish engine.
  void Key(const void* key, int length);

 protected:
  /*
  **	The Blowfish engine used for encryption/decryption. If it is empty,
  **	then this indicates that the blowfish engine is not active and no
  **	key has been submitted. All data would pass through this pipe unchanged
  **	in that case.
  */
  std::optional<BlowfishEngine> BF;

 private:
  static constexpr int kBlockSize = 8;
  std::array<char, kBlockSize> Buffer{};
  int Counter = 0;
  CipherMode Control;

 public:
  BlowfishSink(const BlowfishSink&) = delete;
  BlowfishSink& operator=(const BlowfishSink&) = delete;
  BlowfishSink(BlowfishSink&&) = delete;
  BlowfishSink& operator=(BlowfishSink&&) = delete;
};

#endif  // CNC_RED_ALERT_TECH_BLOWFISH_SINK_H_
