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

//	Bigcheck.h
//	ajw 9/14/98

#ifndef CNC_RED_ALERT_RA_BIGCHECK_H_
#define CNC_RED_ALERT_RA_BIGCHECK_H_

#include <string>

#include "ra/toggle.h"

#define BIGCHECK_OFFSETX 20
#define BIGCHECK_OFFSETY 0

//***********************************************************************************************
class BigCheckBoxClass : public ToggleClass {
 public:
  BigCheckBoxClass(unsigned id, int x, int y, int w, int h,
                   const char* szCaptionIn, TextPrintType text_flags,
                   bool bInitiallyChecked = false)
      : ToggleClass(id, x, y, w, h),
        TextFlags(text_flags),
        szCaption(szCaptionIn) {
    if (bInitiallyChecked) {
      Turn_On();
    }
    IsToggleType = 1;
  }

  int Draw_Me(bool forced = false) override;
  int Action(unsigned flags, KeyNumType& key) override;

  bool Toggle() {
    if (IsOn) {
      Turn_Off();
      return false;
    }
    Turn_On();
    return true;
  }

 protected:
  TextPrintType TextFlags;
  std::string szCaption;
};

#endif  // CNC_RED_ALERT_RA_BIGCHECK_H_
