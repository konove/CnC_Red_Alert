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

#include "tech/lcw.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"

int LcwUncompBounded(std::span<const std::byte> source,
                     std::span<std::byte> dest) {
  const base::ssize in_size = std::ssize(source);
  const base::ssize out_size = std::ssize(dest);
  const auto in_bytes = source.begin();
  const auto out_bytes = dest.begin();
  base::ssize in = 0;
  base::ssize out = 0;

  const auto byte_at = [&](base::ssize offset) {
    return std::to_integer<uint8_t>(in_bytes[in + offset]);
  };
  // Copies forward one byte at a time so a reference that overlaps the bytes
  // it is producing repeats a pattern, as the encoder intends.
  const auto copy_back = [&](base::ssize from, base::ssize count) {
    if (from < 0 || from >= out || count > out_size - out) {
      return false;
    }
    for (base::ssize i = 0; i < count; ++i) {
      out_bytes[out + i] = out_bytes[from + i];
    }
    out += count;
    return true;
  };

  while (in < in_size) {
    const uint8_t op_code = byte_at(0);
    ++in;

    if ((op_code & 0x80) == 0) {
      // Short copy, relative to the write position.
      if (in_size - in < 1) {
        return -1;
      }
      const base::ssize offset = ((op_code & 0x0f) * 256) + byte_at(0);
      in += 1;
      if (!copy_back(out - offset, (op_code >> 4) + 3)) {
        return -1;
      }
    } else if ((op_code & 0x40) == 0) {
      if (op_code == 0x80) {
        return static_cast<int>(out);
      }
      // Literal bytes from the source. Byte by byte, because the in-place
      // straw decodes with source and dest in the same buffer.
      const base::ssize count = op_code & 0x3f;
      if (count > in_size - in || count > out_size - out) {
        return -1;
      }
      for (base::ssize i = 0; i < count; ++i) {
        out_bytes[out + i] = in_bytes[in + i];
      }
      in += count;
      out += count;
    } else if (op_code == 0xfe) {
      // Long run of one byte value.
      if (in_size - in < 3) {
        return -1;
      }
      const base::ssize count = byte_at(0) + (byte_at(1) * 256);
      if (count > out_size - out) {
        return -1;
      }
      std::fill_n(out_bytes + out, count, in_bytes[in + 2]);
      in += 3;
      out += count;
    } else if (op_code == 0xff) {
      // Long copy from an absolute output position.
      if (in_size - in < 4) {
        return -1;
      }
      const base::ssize count = byte_at(0) + (byte_at(1) * 256);
      const base::ssize from = byte_at(2) + (byte_at(3) * 256);
      in += 4;
      if (!copy_back(from, count)) {
        return -1;
      }
    } else {
      // Medium copy from an absolute output position.
      if (in_size - in < 2) {
        return -1;
      }
      const base::ssize from = byte_at(0) + (byte_at(1) * 256);
      in += 2;
      if (!copy_back(from, (op_code & 0x3f) + 3)) {
        return -1;
      }
    }
  }

  // The source ran out before the end marker.
  return -1;
}

namespace {

constexpr int kMaxLiteralRun = 63;         // 10xxxxxx holds 1..63 bytes.
constexpr int kMaxShortCount = 10;         // 0xxx: 3..10 bytes.
constexpr int kMaxShortDistance = 0xfff;   // 12-bit relative distance.
constexpr int kMaxMediumCount = 64;        // 11xxxxxx below 0xfe: 3..64.
constexpr int kMaxLongCount = 0xffff;      // 16-bit count for 0xfe and 0xff.
constexpr int kMaxAbsolutePosition = 0xffff;
constexpr int kHashSize = 1 << 12;
constexpr int kMaxChainSteps = 256;

// One way to encode the bytes at the current position.
struct Choice {
  enum class Kind { kLiteral, kShort, kMedium, kLong, kFill };
  using enum Kind;
  Kind kind = kLiteral;
  int count = 1;    // Input bytes covered.
  int from = 0;     // Distance (short) or absolute position (medium, long).
  int savings = 0;  // Input bytes covered minus encoded bytes.
};

// Returns the cheapest back-reference for a match of `length` bytes starting
// `distance` bytes back at absolute `position`.
Choice BestReference(int length, int distance, int position) {
  Choice best;
  if (distance <= kMaxShortDistance) {
    const int count = std::min(length, kMaxShortCount);
    best = {Choice::kShort, count, distance, count - 2};
  }
  if (position <= kMaxAbsolutePosition) {
    const int medium = std::min(length, kMaxMediumCount);
    if (medium - 3 > best.savings) {
      best = {Choice::kMedium, medium, position, medium - 3};
    }
    const int long_count = std::min(length, kMaxLongCount);
    if (long_count - 5 > best.savings) {
      best = {Choice::kLong, long_count, position, long_count - 5};
    }
  }
  return best;
}

int HashAt(std::span<const std::byte> in, int pos) {
  const uint32_t hash =
      (std::to_integer<uint32_t>(in[base::ToSize(pos)]) << 8) ^
      (std::to_integer<uint32_t>(in[base::ToSize(pos + 1)]) << 4) ^
      std::to_integer<uint32_t>(in[base::ToSize(pos + 2)]);
  return static_cast<int>(hash % kHashSize);
}

}  // namespace

// Greedy encoder. A back-reference or fill is only used when it saves at least
// one byte over storing the bytes as literals, which also pays for any literal
// opcode it forces afterwards; so the output never exceeds LcwWorstCaseSize().
int LCW_Comp(std::span<const std::byte> in, std::span<std::byte> out) {
  const int length = static_cast<int>(in.size());
  if (out.size() < static_cast<std::size_t>(LcwWorstCaseSize(length))) {
    return -1;
  }
  int written = 0;
  int literal_opcode = -1;  // Index of the open literal run's opcode, or -1.

  // Hash chains over 3-byte prefixes find earlier occurrences quickly.
  std::vector<int> head(kHashSize, -1);
  std::vector<int> previous(base::ToSize(std::max(length, 0)), -1);
  const auto remember = [&](int pos) {
    if (pos + 2 < length) {
      const auto bucket = base::ToSize(HashAt(in, pos));
      previous[base::ToSize(pos)] = head[bucket];
      head[bucket] = pos;
    }
  };

  const auto put = [&](int value) {
    out[base::ToSize(written++)] = static_cast<std::byte>(value);
  };

  int pos = 0;
  while (pos < length) {
    const int limit = std::min(length - pos, kMaxLongCount);
    Choice best;

    int run = 1;
    while (run < limit &&
           in[base::ToSize(pos + run)] == in[base::ToSize(pos)]) {
      ++run;
    }
    if (run - 4 > best.savings) {
      best = {Choice::kFill, run, 0, run - 4};
    }

    if (pos + 2 < length) {
      int steps = 0;
      for (int candidate = head[base::ToSize(HashAt(in, pos))];
           candidate >= 0 && steps < kMaxChainSteps;
           candidate = previous[base::ToSize(candidate)], ++steps) {
        int match = 0;
        // Overlapping matches are fine: the decoder copies byte by byte.
        while (match < limit && in[base::ToSize(candidate + match)] ==
                                    in[base::ToSize(pos + match)]) {
          ++match;
        }
        if (match < 3) {
          continue;
        }
        const Choice reference = BestReference(match, pos - candidate, candidate);
        if (reference.savings > best.savings) {
          best = reference;
        }
      }
    }

    if (best.savings < 1) {
      if (literal_opcode < 0 ||
          out[base::ToSize(literal_opcode)] ==
              static_cast<std::byte>(0x80 + kMaxLiteralRun)) {
        literal_opcode = written;
        put(0x80);
      }
      out[base::ToSize(literal_opcode)] = static_cast<std::byte>(
          std::to_integer<int>(out[base::ToSize(literal_opcode)]) + 1);
      put(std::to_integer<int>(in[base::ToSize(pos)]));
      remember(pos);
      ++pos;
      continue;
    }

    switch (best.kind) {
      case Choice::kShort:
        put(((best.count - 3) * 16) + (best.from / 256));
        put(best.from % 256);
        break;
      case Choice::kMedium:
        put(0xc0 + (best.count - 3));
        put(best.from % 256);
        put(best.from / 256);
        break;
      case Choice::kLong:
        put(0xff);
        put(best.count % 256);
        put(best.count / 256);
        put(best.from % 256);
        put(best.from / 256);
        break;
      case Choice::kFill:
        put(0xfe);
        put(best.count % 256);
        put(best.count / 256);
        put(std::to_integer<int>(in[base::ToSize(pos)]));
        break;
      case Choice::kLiteral:
      default:
        break;
    }
    for (int i = 0; i < best.count; ++i) {
      remember(pos + i);
    }
    pos += best.count;
    literal_opcode = -1;
  }

  put(0x80);
  return written;
}
