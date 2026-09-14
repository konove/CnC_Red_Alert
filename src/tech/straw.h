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

/* $Header: /CounterStrike/STRAW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : STRAW.H *
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

#ifndef CNC_RED_ALERT_TECH_STRAW_H_
#define CNC_RED_ALERT_TECH_STRAW_H_

#include <cstddef>
#include <span>
#include <type_traits>

#include "base/numeric.h"
#include "base/types.h"

/*
**	This is a demand driven data carrier. It will retrieve the byte request
*by passing *	the request down the chain (possibly processing on the way) in
*order to fulfill the *	data request. Without being derived, this class merely
*passes the data through. Derived *	versions are presumed to modify the data
*in some useful way or monitor the data *	flow.
*/
class Straw {
 public:
  Straw() = default;
  virtual ~Straw() = default;

  Straw(const Straw&) = delete;
  Straw& operator=(const Straw&) = delete;
  Straw(Straw&&) = delete;
  Straw& operator=(Straw&&) = delete;

  void SetSource(Straw* source) { source_ = source; }
  void SetSource(Straw& source) { source_ = &source; }
  // Pulls up to buffer.size() bytes through the chain into buffer and returns
  // how many were stored. The count is short only at the end of the data or
  // after a failure, which ok() tells apart.
  virtual base::ssize Get(std::span<std::byte> buffer);

  // Returns false once this link or any link before it has failed: a read
  // error, or data that cannot be decoded.
  [[nodiscard]] bool ok() const {
    return ok_ && (source_ == nullptr || source_->ok());
  }

  // Pulls one trivially copyable value. Returns false on a short read, in
  // which case value is partially written.
  template <typename T>
    requires std::is_trivially_copyable_v<T>
  bool ReadObject(T& value) {
    return Get(std::as_writable_bytes(std::span(&value, 1))) ==
           base::ToSigned(sizeof(T));
  }

 protected:
  // The straw we pull data from. Caller must ensure source outlives this straw.
  Straw* source_ = nullptr;

  // Marks this link as failed, which makes ok() false for good.
  void Fail() { ok_ = false; }

 private:
  bool ok_ = true;
};

#endif  // CNC_RED_ALERT_TECH_STRAW_H_
