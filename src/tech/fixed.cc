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

/* $Header: /CounterStrike/FIXED.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : FIXED.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 06/20/96 *
 *                                                                                             *
 *                  Last Update : July 3, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * fixed::As_ASCII -- Returns a pointer (static) of this number as
 *an ASCII string.          * fixed::To_ASCII -- Convert a fixed point number
 *into an ASCII string.                     * fixed::fixed -- Constructor for
 *fixed integral from ASCII initializer.                    *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "tech/fixed.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

const fixed fixed::_1_2(1, 2);
const fixed fixed::_1_3(1, 3);
const fixed fixed::_1_4(1, 4);
const fixed fixed::_3_4(3, 4);
const fixed fixed::_2_3(2, 3);

fixed::fixed(int numerator, int denominator) {
  if (denominator == 0) {
    Data.Raw = 0;
  } else {
    Data.Raw = static_cast<unsigned short>((unsigned)(numerator * 256) /
                                           (unsigned)denominator);
  }
}

fixed::fixed(const char* ascii) {
  // Handles null pointer, which can occur when the compiler selects this
  // overload instead of the int constructor for literal "0".
  if (ascii == nullptr) {
    Data.Raw = 0;
    return;
  }

  const char* wholepart = ascii;

  // Skip leading whitespace.
  while (isspace(*ascii)) {
    ascii++;
  }

  // Check for trailing '%' to detect percentage format.
  const char* tptr = ascii;
  while (isdigit(*tptr)) {
    tptr++;
  }

  // Percentage: "75%" → 75 * 256 / 100 ≈ 0.75 in 8.8 fixed point.
  if (*tptr == '\%') {
    Data.Raw = static_cast<unsigned short>(atoi(ascii) * 256 / 100);
  } else {
    Data.Composite.Whole = Data.Composite.Fraction = 0;
    if (wholepart && *wholepart != '.') {
      Data.Composite.Whole = static_cast<unsigned char>(atoi(wholepart));
    }

    const char* fracpart = strchr(ascii, '.');
    if (fracpart) {
      fracpart++;
    }
    if (fracpart) {
      int frac = atoi(fracpart);

      int base = 1;
      const char* fptr = fracpart;
      while (isdigit(*fptr)) {
        fptr++;
        base *= 10;
      }

      Data.Composite.Fraction = static_cast<unsigned char>(256 * frac / base);
    }
  }
}

int fixed::To_ASCII(char* buffer, int buffer_size) const {
  if (buffer == nullptr) {
    return 0;
  }

  int whole = Data.Composite.Whole;
  // Convert 8-bit fraction (0-255) to thousandths for decimal display.
  int frac = static_cast<int>(Data.Composite.Fraction) * 1000 / 256;
  char tbuffer[32];

  if (frac == 0) {
    sprintf(tbuffer, "%d", whole);
  } else {
    sprintf(tbuffer, "%d.%02d", whole, frac);

    // Strip trailing zeros from the fractional part.
    char* ptr = &tbuffer[strlen(tbuffer) - 1];
    while (*ptr == '0') {
      *ptr = '\0';
      ptr--;
    }
  }

  if (buffer_size == -1) {
    buffer_size = strlen(tbuffer) + 1;
  }

  strncpy(buffer, tbuffer, buffer_size);

  int len = strlen(tbuffer);
  if (len < buffer_size - 1) {
    return len;
  }
  return buffer_size - 1;
}

const char* fixed::As_ASCII() const {
  static char buffer[32];

  To_ASCII(buffer, sizeof(buffer));
  return buffer;
}
