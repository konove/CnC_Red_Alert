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

/* $Header: /CounterStrike/SHA.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : SHA.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 04/26/96 *
 *                                                                                             *
 *                  Last Update : April 26, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_SHA_H_
#define CNC_RED_ALERT_TECH_SHA_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>

// A SHA-1 digest: 20 bytes, most significant first.
using Sha1Digest = std::array<std::byte, 20>;

/*
**	This implements the Secure Hash Algorithm. It is a cryptographically
**	secure hash with no known weaknesses. It generates a 160 bit hash
**	result given an arbitrary length data source.
*/
class SHAEngine {
 public:
  SHAEngine() = default;

  void Init() { new (static_cast<void*>(this)) SHAEngine; }

  // Returns the digest of everything hashed so far, as if the data stopped
  // here. Hashing may continue afterwards.
  [[nodiscard]] Sha1Digest Digest() const;

  void Hash(std::span<const std::byte> data);

 private:
  // The five 32-bit words the algorithm accumulates.
  using Accumulator = std::array<uint32_t, 5>;

  /*
  **	This holds the calculated final result. It is cached
  **	here to avoid the overhead of recalculating it over
  **	multiple sequential requests.
  */
  // Result() is const and fills this cache lazily.
  mutable bool IsCached = false;
  mutable Sha1Digest FinalResult{};

  static constexpr uint32_t kSa =
      0x67452301L;  // These are the initial seeds to the block accumulators.
  static constexpr uint32_t kSb = 0xefcdab89L;
  static constexpr uint32_t kSc = 0x98badcfeL;
  static constexpr uint32_t kSd = 0x10325476L;
  static constexpr uint32_t kSe = 0xc3d2e1f0L;
  static constexpr uint32_t kK1 =
      0x5a827999L;  // These are the constants used in the block transformation.
  static constexpr uint32_t kK2 = 0x6ed9eba1L;  // t=0..19 2^(1/2)/4
  static constexpr uint32_t kK3 = 0x8f1bbcdcL;  // t=20..39 3^(1/2)/4
  static constexpr uint32_t kK4 = 0xca62c1d6L;  // t=40..59 5^(1/2)/4

  // Source data is grouped into blocks of this size. Sizes are constexpr int
  // rather than enumerators: sizeof() makes an enumerator unsigned, which turns
  // every comparison against an int index into a sign mismatch.
  static constexpr int SRC_BLOCK_SIZE = 16 * static_cast<int>(sizeof(uint32_t));

  // Internal processing data is grouped into blocks this size.
  static constexpr int PROC_BLOCK_SIZE =
      80 * static_cast<int>(sizeof(uint32_t));

  static uint32_t Get_Constant(int index) {
    if (index < 20) {
      return kK1;
    }
    if (index < 40) {
      return kK2;
    }
    if (index < 60) {
      return kK3;
    }
    return kK4;
  }

  // Used for 0..19
  static uint32_t Function1(uint32_t X, uint32_t Y, uint32_t Z) {
    return Z ^ (X & (Y ^ Z));
  }

  // Used for 20..39
  static uint32_t Function2(uint32_t X, uint32_t Y, uint32_t Z) {
    return X ^ Y ^ Z;
  }

  // Used for 40..59
  static uint32_t Function3(uint32_t X, uint32_t Y, uint32_t Z) {
    return (X & Y) | (Z & (X | Y));
  }

  // Used for 60..79
  static uint32_t Function4(uint32_t X, uint32_t Y, uint32_t Z) {
    return X ^ Y ^ Z;
  }

  static uint32_t Do_Function(int index, uint32_t X, uint32_t Y, uint32_t Z) {
    if (index < 20) {
      return Function1(X, Y, Z);
    }
    if (index < 40) {
      return Function2(X, Y, Z);
    }
    if (index < 60) {
      return Function3(X, Y, Z);
    }
    return Function4(X, Y, Z);
  }

  // Process a full source data block.
  static void Process_Block(std::span<const std::byte> source,
                            Accumulator& acc);

  // Processes a partially filled source accumulator buffer.
  void Process_Partial(std::span<const std::byte>& data);

  /*
  **	This is the running accumulator values. These values
  **	are updated by a block processing step that occurs
  **	every 512 bits of source data.
  */
  Accumulator Acc{kSa, kSb, kSc, kSd, kSe};

  /*
  **	This is the running length of the source data
  **	processed so far. This total is used to modify the
  **	resulting hash value as if it were appended to the end
  **	of the source data.
  */
  int32_t Length = 0;

  /*
  **	This holds any partial source block. Partial source blocks are
  **	a consequence of submitting less than block sized data chunks
  **	to the SHA Engine.
  */
  int PartialCount = 0;
  char Partial[SRC_BLOCK_SIZE]{};
};

#define SHA_SOURCE1 "abc"
#define SHA_DIGEST1a                                                         \
  "\xA9\x99\x3E\x36\x47\x06\x81\x6A\xBA\x3E\x25\x71\x78\x50\xC2\x6C\x9C\xD0" \
  "\xD8\x9D"
#define SHA_DIGEST1b                                                         \
  "\x01\x64\xB8\xA9\x14\xCD\x2A\x5E\x74\xC4\xF7\xFF\x08\x2C\x4D\x97\xF1\xED" \
  "\xF8\x80"

#define SHA_SOURCE2 "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
#define SHA_DIGEST2a                                                         \
  "\x84\x98\x3E\x44\x1C\x3B\xD2\x6E\xBA\xAE\x4A\xA1\xF9\x51\x29\xE5\xE5\x46" \
  "\x70\xF1"
#define SHA_DIGEST2b                                                         \
  "\xD2\x51\x6E\xE1\xAC\xFA\x5B\xAF\x33\xDF\xC1\xC4\x71\xE4\x38\x44\x9E\xF1" \
  "\x34\xC8"

#define SHA_SOURCE3 \
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
#define SHA_DIGEST3a                                                         \
  "\x34\xAA\x97\x3C\xD4\xC4\xDA\xA4\xF6\x1E\xEB\x2B\xDB\xAD\x27\x31\x65\x34" \
  "\x01\x6F"
#define SHA_DIGEST3b                                                         \
  "\x32\x32\xAF\xFA\x48\x62\x8A\x26\x65\x3B\x5A\xAA\x44\x54\x1F\xD9\x0D\x69" \
  "\x06\x03"

#endif  // CNC_RED_ALERT_TECH_SHA_H_
