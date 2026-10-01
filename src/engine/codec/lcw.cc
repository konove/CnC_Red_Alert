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
#include <span>

#include "absl/log/check.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The kernels are engine_rs's lcw module (src/rust/src/lcw/).

int LcwUncompBounded(std::span<const std::byte> source,
                     std::span<std::byte> dest) {
  CHECK(!Overlaps(source, dest)) << "LCW needs separate buffers";
  return engine_rs::lcw_uncompress_bounded(AsRustSlice(source),
                                           AsRustSlice(dest));
}

int LCW_Comp(std::span<const std::byte> in, std::span<std::byte> out) {
  CHECK(!Overlaps(in, out)) << "LCW needs separate buffers";
  return engine_rs::lcw_compress(AsRustSlice(in), AsRustSlice(out));
}
