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

/* $Header: /CounterStrike/LCW.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***************************************************************************
 **    C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *               Project Name : WESTWOOD LIBRARY (PSX)                     *
 *                                                                         *
 *                 File Name : LCWUNCMP.CPP                                *
 *                                                                         *
 *                Programmer : Ian M. Leslie                               *
 *                                                                         *
 *                Start Date : May 17, 1995                                *
 *                                                                         *
 *               Last Update : May 17, 1995    [IML]                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "engine/codec/lcw.h"

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The kernels are engine_rs's lcw module (src/rust/src/lcw/).

std::optional<std::vector<std::byte>> LcwUncompBounded(
    const std::span<const std::byte> source, const base::ssize max_size) {
  // Rust fills the vector this returns; one it returned would need copying.
  std::vector<std::byte> decoded(base::ToSize(max_size));
  const int size = engine_rs::lcw_uncompress_bounded(
      AsRustSlice(source), AsRustSlice(std::span(decoded)));
  if (size < 0) {
    return std::nullopt;
  }
  decoded.resize(base::ToSize(size));
  return decoded;
}

std::vector<std::byte> LCW_Comp(const std::span<const std::byte> in) {
  return ToByteVector(engine_rs::lcw_compress(AsRustSlice(in)));
}
