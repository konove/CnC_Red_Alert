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

#ifndef MP_H
#define MP_H

#include <cstdint>
#include <cstdlib>

#include "tech/straw.h"

extern unsigned short primeTable[3511];

// #define uint32_t uint32_t
// #define signeddigit int32_t
#define LOG_UNITSIZE 5
#define UNITSIZE 32
#define UPPER_MOST_BIT 0x80000000L
#define SEMI_UPPER_MOST_BIT 0x8000
#define SEMI_MASK ((unsigned short)~0)
#define MAX_BIT_PRECISION 2048
#define MAX_UNIT_PRECISION (MAX_BIT_PRECISION / UNITSIZE)
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

int XMP_Significance(const uint32_t* r, int precision);
void XMP_Inc(uint32_t* r, int precision);
void XMP_Dec(uint32_t* r, int precision);
void XMP_Neg(uint32_t* r, int precision);
void XMP_Abs(uint32_t* r, int precision);
void XMP_Shift_Right_Bits(uint32_t* r1, int bits, int precision);
void XMP_Shift_Left_Bits(uint32_t* r1, int bits, int precision);
bool XMP_Rotate_Left(uint32_t* r1, bool carry, int precision);
void XMP_Not(uint32_t* digit_ptr, int precision);
void XMP_Init(uint32_t* r, uint32_t value, int precision);
unsigned XMP_Count_Bits(const uint32_t* r, int precision);
int XMP_Count_Bytes(const uint32_t* r, int precision);
void XMP_Move(uint32_t* dest, const uint32_t* source, int precision);
int XMP_Compare(const uint32_t* r1, const uint32_t* r2, int precision);
bool XMP_Add(uint32_t* result, const uint32_t* r1, const uint32_t* r2,
             bool carry, int precision);
bool XMP_Add_Int(uint32_t* result, const uint32_t* r1, uint32_t r2, bool carry,
                 int precision);
bool XMP_Sub(uint32_t* result, const uint32_t* r1, const uint32_t* r2,
             bool borrow, int precision);
bool XMP_Sub_Int(uint32_t* result, const uint32_t* r1, unsigned short r2,
                 bool borrow, int precision);
int XMP_Unsigned_Mult(uint32_t* prod, const uint32_t* multiplicand,
                      const uint32_t* multiplier, int precision);
int XMP_Unsigned_Mult_Int(uint32_t* prod, const uint32_t* multiplicand,
                          short multiplier, int precision);
int XMP_Signed_Mult_Int(uint32_t* prod, const uint32_t* multiplicand,
                        signed short multiplier, int precision);
int XMP_Signed_Mult(uint32_t* prod, const uint32_t* multiplicand,
                    const uint32_t* multiplier, int precision);
unsigned short XMP_Unsigned_Div_Int(uint32_t* quotient,
                                    const uint32_t* dividend,
                                    unsigned short divisor, int precision);
int XMP_Unsigned_Div(uint32_t* remainder, uint32_t* quotient,
                     const uint32_t* dividend, const uint32_t* divisor,
                     int precision);
void XMP_Signed_Div(uint32_t* remainder, uint32_t* quotient,
                    const uint32_t* dividend, const uint32_t* divisor,
                    int precision);
int XMP_Reciprocal(uint32_t* quotient, const uint32_t* divisor, int precision);
void XMP_Decode_ASCII(const char* str, uint32_t* mpn, int precision);
void xmp_single_mul(unsigned short* prod, unsigned short* multiplicand,
                    unsigned short multiplier, int precision);
void XMP_Double_Mul(uint32_t* prod, const uint32_t* multiplicand,
                    const uint32_t* multiplier, int precision);
int xmp_stage_modulus(const uint32_t* n_modulus, int precision);
int XMP_Mod_Mult(uint32_t* prod, const uint32_t* multiplicand,
                 const uint32_t* multiplier, int precision);
void XMP_Mod_Mult_Clear(int precision);
unsigned short mp_quo_digit(const unsigned short* dividend);
int xmp_exponent_mod(uint32_t* expout, const uint32_t* expin,
                     const uint32_t* exponent_ptr, const uint32_t* modulus,
                     int precision);
bool XMP_Is_Small_Prime(const uint32_t* candidate, int precision);
bool XMP_Small_Divisors_Test(const uint32_t* candidate, int precision);
bool XMP_Fermat_Test(const uint32_t* candidate_prime, unsigned rounds,
                     int precision);
void XMP_Inverse_A_Mod_B(uint32_t* result, const uint32_t* number,
                         const uint32_t* modulus, int precision);
void XMP_Signed_Decode(uint32_t* result, const unsigned char* from,
                       int frombytes, int precision);
void XMP_Unsigned_Decode(uint32_t* result, const unsigned char* from,
                         int frombytes, int precision);
unsigned XMP_Encode(unsigned char* to, const uint32_t* from, int precision);
unsigned XMP_Encode(unsigned char* to, unsigned tobytes, const uint32_t* from,
                    int precision);
void XMP_Randomize(uint32_t* result, Straw& rng, int nbits, int precision);
void XMP_Randomize(uint32_t* result, Straw& rng, const uint32_t* min,
                   const uint32_t* max, int precision);
bool XMP_Is_Prime(const uint32_t* prime, int precision);
bool XMP_Rabin_Miller_Test(Straw& rng, const uint32_t* w, int rounds,
                           int precision);
int XMP_DER_Length_Encode(unsigned long length, unsigned char* output);
int XMP_DER_Encode(const uint32_t* from, unsigned char* output, int precision);
void XMP_DER_Decode(uint32_t* result, const unsigned char* input,
                    int precision);

inline int XMP_Digits_To_Bits(int digits) { return digits << LOG_UNITSIZE; }

inline int XMP_Bits_To_Digits(int bits) {
  return (bits + (UNITSIZE - 1)) / UNITSIZE;
}

inline uint32_t XMP_Bits_To_Mask(int bits) {
  if (!bits) return 0;
  return 1 << ((bits - 1) % UNITSIZE);
}

inline bool XMP_Is_Negative(const uint32_t* r, int precision) {
  return static_cast<int32_t>(*(r + (precision - 1))) < 0;
}

inline bool XMP_Test_Eq_Int(const uint32_t* r, int i, int p) {
  return *r == i && XMP_Significance(r, p) <= 1;
}

inline void XMP_Set_Bit(uint32_t* r, unsigned bit) {
  r[bit >> LOG_UNITSIZE] |= static_cast<uint32_t>(1) << (bit & UNITSIZE - 1);
}

inline bool XMP_Test_Bit(const uint32_t* r, unsigned bit) {
  return r[bit >> LOG_UNITSIZE] & static_cast<uint32_t>(1)
                                      << (bit & UNITSIZE - 1);
}

// Misc functions.
void memrev(char* buffer, size_t length);

#endif
