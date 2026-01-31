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

/* $Header: /CounterStrike/HSV.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : HSV.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 12/02/95 *
 *                                                                                             *
 *                  Last Update : December 2, 1995 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef HSV_H
#define HSV_H

class RGBClass;

/*
**	Each color entry is represented by this class. It holds the values for
*the color *	attributes. The values are recorded in a range from 0 to 255
*with 255 being the *	maximum.
*/
class HSVClass {
 private:
  static const HSVClass BlackColor;

 public:
  HSVClass() : Hue(0), Saturation(0), Value(0) {}
  HSVClass(unsigned char hue, unsigned char saturation, unsigned char value)
      : Hue(hue), Saturation(saturation), Value(value) {}
  operator RGBClass() const;

  enum { MAX_VALUE = 255 };

  void Adjust(int ratio, const HSVClass& hsv);
  int Difference(const HSVClass& hsv) const;
  int Hue_Component() const { return Hue; }
  int Saturation_Component() const { return Saturation; }
  int Value_Component() const { return Value; }

 private:
  unsigned char Hue;
  unsigned char Saturation;
  unsigned char Value;
};

#endif
