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

/* $Header: /CounterStrike/PIPE.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : PIPE.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/29/96 *
 *                                                                                             *
 *                  Last Update : June 29, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_PIPE_H_
#define CNC_RED_ALERT_TECH_PIPE_H_

#include <cstddef>
#include <span>
#include <type_traits>

#include "base/types.h"

/*
**	A "push through" pipe interface abstract class used for such purposes as
*compression *	and translation of data. In STL terms, this is functionally
*similar to an output *	iterator but with a few enhancements. A pipe class
*object that is not derived into *	another useful class serves only as a
*pseudo null-pipe. It will accept data but *	just throw it away but pretend
*that it sent it somewhere.
*/
class Pipe {
 public:
  Pipe() = default;
  virtual ~Pipe() = default;
  Pipe(const Pipe&) = delete;
  Pipe& operator=(const Pipe&) = delete;
  Pipe(Pipe&&) = delete;
  Pipe& operator=(Pipe&&) = delete;

  virtual base::ssize Flush();
  virtual base::ssize End() { return Flush(); }
  void SetSink(Pipe* sink) { sink_ = sink; }
  void SetSink(Pipe& sink) { sink_ = &sink; }

  // Pushes bytes down the chain and returns how many reached its far end.
  // A link that buffers returns less than it was given; the rest follows on
  // a later Put or Flush.
  virtual base::ssize Put(std::span<const std::byte> bytes);

  // Pushes one trivially copyable value; returns what Put returns.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  base::ssize WriteObject(const T& value) {
    return Put(std::as_bytes(std::span(&value, 1)));
  }

 protected:
  // The pipe we push data to. Caller must ensure sink outlives this pipe.
  Pipe* sink_ = nullptr;
};

#endif  // CNC_RED_ALERT_TECH_PIPE_H_
