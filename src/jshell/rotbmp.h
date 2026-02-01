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

#ifndef CNC_RED_ALERT_JSHELL_ROTBMP_H_
#define CNC_RED_ALERT_JSHELL_ROTBMP_H_

#include "base/trig.h"
#include "sdllib/gbuffer.h"

#define MAKE_PTR(vp, x, y) \
  (char*)(vp->Get_Offset() + ((y) * (vp->Get_Width() + vp->Get_XAdd())) + (x))
#define MAKE_PTR2(vp, x, y)  \
  (char*)(vp->Get_Offset() + \
          (((y) << 1) * (vp->Get_Width() + vp->Get_XAdd())) + ((x) << 1))

int Rotate_Bitmap(GraphicViewPortClass* srcvp, GraphicViewPortClass* destvp,
                  int angle);

#endif  // CNC_RED_ALERT_JSHELL_ROTBMP_H_
