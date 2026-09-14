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

  void SetSink(Pipe* sink) { sink_ = sink; }
  void SetSink(Pipe& sink) { sink_ = &sink; }

  // Accepts bytes, buffering them or passing them down the chain. Returns
  // true if every byte was accepted. Once any link in the chain has failed,
  // ok() is false and every later Put returns false.
  virtual bool Put(std::span<const std::byte> bytes);

  // Pushes everything buffered (a partial compression block, a Blowfish
  // tail, Base64 padding) all the way to the end of the chain. The chain
  // stays usable, and a Flush with nothing buffered emits nothing. Returns
  // ok().
  virtual bool Flush();

  // Flushes, then lets every link release what it holds: a file pipe closes
  // a file it opened. The last call made on a chain. Returns ok().
  virtual bool Finish();

  // Returns false once this link or any link after it has failed.
  [[nodiscard]] bool ok() const {
    return ok_ && (sink_ == nullptr || sink_->ok());
  }

  // Puts one trivially copyable value; returns what Put returns.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool WriteObject(const T& value) {
    return Put(std::as_bytes(std::span(&value, 1)));
  }

 protected:
  // Marks this link as failed, which makes ok() false for good.
  void Fail() { ok_ = false; }

  // The pipe we push data to. Caller must ensure sink outlives this pipe.
  Pipe* sink_ = nullptr;

 private:
  bool ok_ = true;
};

#endif  // CNC_RED_ALERT_TECH_PIPE_H_
