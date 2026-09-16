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

/* $Header: /CounterStrike/MP.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : MP.H *
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

#ifndef CNC_RED_ALERT_TECH_MP_H_
#define CNC_RED_ALERT_TECH_MP_H_

#include <cstdint>
#include <cstdlib>
#include <span>
#include <string_view>
#include <utility>

#include "base/buffer.h"
#include "tech/byte_source.h"
#include "tech/digit_cursor.h"

extern uint16_t primeTable[3511];

// #define uint32_t uint32_t
// #define signeddigit int32_t
#define LOG_UNITSIZE 5
#define UNITSIZE 32
#define UPPER_MOST_BIT 0x80000000L
#define SEMI_UPPER_MOST_BIT 0x8000
inline constexpr uint16_t kSemiMask = 0xFFFF;
#define MAX_BIT_PRECISION 2048
#define MAX_UNIT_PRECISION (MAX_BIT_PRECISION / UNITSIZE)

int XMP_Significance(DigitCursor<const uint32_t> r, int precision);
void XMP_Inc(DigitCursor<uint32_t> r, int precision);
void XMP_Dec(DigitCursor<uint32_t> r, int precision);
void XMP_Neg(DigitCursor<uint32_t> r, int precision);
void XMP_Abs(DigitCursor<uint32_t> r, int precision);
void XMP_Shift_Right_Bits(DigitCursor<uint32_t> number, int bits,
                          int precision);
void XMP_Shift_Left_Bits(DigitCursor<uint32_t> number, int bits, int precision);
bool XMP_Rotate_Left(DigitCursor<uint32_t> number, bool carry, int precision);
void XMP_Not(DigitCursor<uint32_t> number, int precision);
void XMP_Init(DigitCursor<uint32_t> r, uint32_t value, int precision);
int XMP_Count_Bits(DigitCursor<const uint32_t> r, int precision);
int XMP_Count_Bytes(DigitCursor<const uint32_t> r, int precision);
void XMP_Move(DigitCursor<uint32_t> dest, DigitCursor<const uint32_t> source,
              int precision);
int XMP_Compare(DigitCursor<const uint32_t> left_number,
                DigitCursor<const uint32_t> right_number, int precision);
bool XMP_Add(DigitCursor<uint32_t> result,
             DigitCursor<const uint32_t> left_number,
             DigitCursor<const uint32_t> right_number, bool carry,
             int precision);
bool XMP_Add_Int(DigitCursor<uint32_t> result,
                 DigitCursor<const uint32_t> left_number, uint32_t right_number,
                 bool carry, int precision);
bool XMP_Sub(DigitCursor<uint32_t> result,
             DigitCursor<const uint32_t> left_number,
             DigitCursor<const uint32_t> right_number, bool borrow,
             int precision);
bool XMP_Sub_Int(DigitCursor<uint32_t> result,
                 DigitCursor<const uint32_t> left_number, uint16_t right_number,
                 bool borrow, int precision);
int XMP_Unsigned_Mult(DigitCursor<uint32_t> prod,
                      DigitCursor<const uint32_t> multiplicand,
                      DigitCursor<const uint32_t> multiplier, int precision);
int XMP_Unsigned_Mult_Int(DigitCursor<uint32_t> prod,
                          DigitCursor<const uint32_t> multiplicand,
                          uint16_t multiplier, int precision);
int XMP_Signed_Mult_Int(DigitCursor<uint32_t> prod,
                        DigitCursor<const uint32_t> multiplicand,
                        int16_t multiplier, int precision);
int XMP_Signed_Mult(DigitCursor<uint32_t> prod,
                    DigitCursor<const uint32_t> multiplicand,
                    DigitCursor<const uint32_t> multiplier, int precision);
uint16_t XMP_Unsigned_Div_Int(DigitCursor<uint32_t> quotient,
                              DigitCursor<const uint32_t> dividend,
                              uint16_t divisor, int precision);
int XMP_Unsigned_Div(DigitCursor<uint32_t> remainder,
                     DigitCursor<uint32_t> quotient,
                     DigitCursor<const uint32_t> dividend,
                     DigitCursor<const uint32_t> divisor, int precision);
void XMP_Signed_Div(DigitCursor<uint32_t> remainder,
                    DigitCursor<uint32_t> quotient,
                    DigitCursor<const uint32_t> dividend,
                    DigitCursor<const uint32_t> divisor, int precision);
int XMP_Reciprocal(DigitCursor<uint32_t> quotient,
                   DigitCursor<const uint32_t> divisor, int precision);
void XMP_Decode_ASCII(std::string_view text, DigitCursor<uint32_t> mpn,
                      int precision);
void xmp_single_mul(DigitCursor<uint16_t> prod,
                    DigitCursor<uint16_t> multiplicand, uint16_t multiplier,
                    int precision);
void XMP_Double_Mul(DigitCursor<uint32_t> prod,
                    DigitCursor<const uint32_t> multiplicand,
                    DigitCursor<const uint32_t> multiplier, int precision);
int xmp_stage_modulus(DigitCursor<const uint32_t> n_modulus, int precision);
int XMP_Mod_Mult(DigitCursor<uint32_t> prod,
                 DigitCursor<const uint32_t> multiplicand,
                 DigitCursor<const uint32_t> multiplier, int precision);
void XMP_Mod_Mult_Clear(int precision);
uint16_t mp_quo_digit(DigitCursor<const uint16_t> dividend);
int xmp_exponent_mod(DigitCursor<uint32_t> expout,
                     DigitCursor<const uint32_t> expin,
                     DigitCursor<const uint32_t> exponent_ptr,
                     DigitCursor<const uint32_t> modulus, int precision);
bool XMP_Is_Small_Prime(DigitCursor<const uint32_t> candidate, int precision);
bool XMP_Small_Divisors_Test(DigitCursor<const uint32_t> candidate,
                             int precision);
bool XMP_Fermat_Test(DigitCursor<const uint32_t> candidate_prime,
                     unsigned rounds, int precision);
void XMP_Inverse_A_Mod_B(DigitCursor<uint32_t> result,
                         DigitCursor<const uint32_t> number,
                         DigitCursor<const uint32_t> modulus, int precision);
void XMP_Signed_Decode(DigitCursor<uint32_t> result,
                       DigitCursor<const unsigned char> from, int frombytes,
                       int precision);
void XMP_Unsigned_Decode(DigitCursor<uint32_t> result,
                         DigitCursor<const unsigned char> from, int frombytes,
                         int precision);
int XMP_Encode(DigitCursor<unsigned char> to, DigitCursor<const uint32_t> from,
               int precision);
unsigned XMP_Encode(DigitCursor<unsigned char> to, unsigned tobytes,
                    DigitCursor<const uint32_t> from, int precision);
void XMP_Randomize(DigitCursor<uint32_t> result, ByteSource& rng,
                   int total_bits, int precision);
void XMP_Randomize(DigitCursor<uint32_t> result, ByteSource& rng,
                   DigitCursor<const uint32_t> min,
                   DigitCursor<const uint32_t> max, int precision);
bool XMP_Is_Prime(DigitCursor<const uint32_t> prime, int precision);
bool XMP_Rabin_Miller_Test(ByteSource& rng, DigitCursor<const uint32_t> w,
                           int rounds, int precision);
int XMP_DER_Length_Encode(uint32_t length, DigitCursor<unsigned char> output);
int XMP_DER_Encode(DigitCursor<const uint32_t> from,
                   DigitCursor<unsigned char> output, int precision);
void XMP_DER_Decode(DigitCursor<uint32_t> result,
                    DigitCursor<const unsigned char> input, int precision);

inline int XMP_Digits_To_Bits(int digits) { return digits * UNITSIZE; }

inline int XMP_Bits_To_Digits(int bits) {
  return (bits + (UNITSIZE - 1)) / UNITSIZE;
}

inline uint32_t XMP_Bits_To_Mask(int bits) {
  if (!bits) {
    return 0;
  }
  return base::Bit<uint32_t>((bits - 1) % UNITSIZE);
}

inline bool XMP_Is_Negative(DigitCursor<const uint32_t> r, int precision) {
  return static_cast<int32_t>(*(r + (precision - 1))) < 0;
}

inline bool XMP_Test_Eq_Int(DigitCursor<const uint32_t> r, int i, int p) {
  return std::cmp_equal(*r, i) && XMP_Significance(r, p) <= 1;
}

inline void XMP_Set_Bit(DigitCursor<uint32_t> r, int bit) {
  r[bit / UNITSIZE] |= base::Bit<uint32_t>(bit % UNITSIZE);
}

inline bool XMP_Test_Bit(DigitCursor<const uint32_t> r, int bit) {
  return (r[bit / UNITSIZE] & base::Bit<uint32_t>(bit % UNITSIZE)) != 0;
}

// Misc functions.
void memrev(std::span<char> buffer);

int XMP_Prepare_Modulus(DigitCursor<const uint32_t> modulus, int precision);

#endif  // CNC_RED_ALERT_TECH_MP_H_
