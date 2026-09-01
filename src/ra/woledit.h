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

#ifndef CNC_RED_ALERT_RA_WOLEDIT_H_
#define CNC_RED_ALERT_RA_WOLEDIT_H_

/***************************************************************************
 * WOLEditClass -- Derived from EditClass, includes changes I wanted for
 *                 wolapi integration stuff.
 *
 * HISTORY:    07/17/1998 ajw : Created.
 *=========================================================================*/

#include "ra/edit.h"

// Set by WOLEditClass::Action when Tab is pressed, and cleared by whoever
// reads it. The login dialog uses it to move focus between its two edit boxes,
// which EditClass has no way to express on its own.
extern bool bTabKeyPressedHack;

class WOLEditClass : public EditClass {
 public:
  WOLEditClass(int id, char* text, int max_len, TextPrintType flags, int x,
               int y, int w, int h, EditStyle style)
      : EditClass(id, text, max_len, flags, x, y, w, h, style) {}

  int Action(unsigned flags, KeyNumType& key) override;  //	Override of base

 protected:
  void Draw_Text(const char* text) override;  //	Override of base
};

#endif  // CNC_RED_ALERT_RA_WOLEDIT_H_
