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

/* $Header: /CounterStrike/LCWUNCMP.CPP 1     3/03/97 10:25a Joe_bostic $ */
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

#include "engine/codec/lcw_uncompress.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/log/check.h"
#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The decoders are engine_rs's lcw::uncompress and lcw::uncompress_in_place
// (src/rust/src/lcw/decode.rs).

std::vector<uint8_t> LcwUncompress(const std::span<const std::byte> source,
                                   const base::ssize max_size) {
  // Rust fills the vector this returns; one it returned would need copying.
  std::vector<uint8_t> decoded(base::ToSize(max_size));
  decoded.resize(engine_rs::lcw_uncompress(
      AsRustSlice(source),
      AsRustSlice(std::as_writable_bytes(std::span(decoded)))));
  return decoded;
}

int32_t LcwUncompressInPlace(const std::span<const std::byte> source,
                             const std::span<std::byte> dest) {
  if (!Overlaps(source, dest)) {
    return static_cast<int32_t>(
        engine_rs::lcw_uncompress(AsRustSlice(source), AsRustSlice(dest)));
  }
  // Rust may not see one buffer as a shared and a mutable slice at once, so
  // the whole of it crosses once.
  const auto dest_begin = std::bit_cast<uintptr_t>(dest.data());
  const auto source_begin = std::bit_cast<uintptr_t>(source.data());
  CHECK_GE(source_begin, dest_begin)
      << "LCW stream overlaps the front of its destination";
  const std::size_t source_start = source_begin - dest_begin;
  const std::size_t buffer_size =
      std::max(dest.size(), source_start + source.size());
  // Both spans lie in one array, which runs at least this far.
  // NOLINTNEXTLINE(clang-diagnostic-unsafe-buffer-usage-in-container)
  const std::span<std::byte> buffer(dest.data(), buffer_size);
  return static_cast<int32_t>(engine_rs::lcw_uncompress_in_place(
      AsRustSlice(buffer), source_start, source.size(), dest.size()));
}

int32_t LcwUncompressInPlace(const std::span<const unsigned char> source,
                             const std::span<unsigned char> dest) {
  return LcwUncompressInPlace(std::as_bytes(source),
                              std::as_writable_bytes(dest));
}
