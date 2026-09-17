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

/* $Header: /CounterStrike/LZW.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : LZW.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 08/28/96 *
 *                                                                                             *
 *                  Last Update : August 28, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TECH_LZW_H_
#define CNC_RED_ALERT_TECH_LZW_H_

#include <cstddef>
#include <cstdint>
#include <span>

class LZWEngine {
 public:
  LZWEngine();

  // Both return the number of bytes written to `output`. The output span
  // bounds the write and the input span bounds the read, so a corrupt code
  // stream cannot run past either buffer.
  int Compress(std::span<const std::byte> input, std::span<std::byte> output);
  int Uncompress(std::span<const std::byte> input, std::span<std::byte> output);

  void Reset();

 private:
  using CodeType = int16_t;
  struct CodeClass {
    CodeType CodeValue = kUnused;
    CodeType ParentCode = 0;
    unsigned char CharValue = 0;

    CodeClass() = default;
    CodeClass(CodeType code, CodeType parent, unsigned char c)
        : CodeValue(code), ParentCode(parent), CharValue(c) {}

    static constexpr int kUnused = -1;
    void Make_Unused() { CodeValue = kUnused; }
    [[nodiscard]] bool Is_Unused() const { return CodeValue == kUnused; }
    [[nodiscard]] bool Is_Matching(CodeType code, unsigned char c) const {
      return ParentCode == code && CharValue == c;
    }
  };

  static constexpr int kBits = 12;
  static constexpr int kMaxCode = 4095;
  static constexpr int kFirstCode = 257;  // (1 << kBits) - 1
  static constexpr int kEndOfStream = 256;
  static constexpr int kTableSize = 5021;
  CodeClass dict[kTableSize];

  unsigned char decode_stack[kTableSize]{};

  int Find_Child_Node(CodeType parent_code, unsigned char child_character);
  int Decode_String(std::span<unsigned char> output, CodeType code);
  static int Make_LZW_Hash(CodeType code, unsigned char character);
};

// Returns the largest output LZW_Compress produces for `length` input bytes:
// one 16-bit code per byte, plus the end-of-stream code.
constexpr int LzwWorstCaseSize(int length) { return 2 * (length + 1); }

// One-shot wrappers around a fresh LZWEngine; return the bytes written.
int LZW_Compress(std::span<const std::byte> input, std::span<std::byte> output);
int LZW_Uncompress(std::span<const std::byte> input,
                   std::span<std::byte> output);

#endif  // CNC_RED_ALERT_TECH_LZW_H_
