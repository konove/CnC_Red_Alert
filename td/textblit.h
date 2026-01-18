/*
**	Command & Conquer(tm)
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
#ifndef CNC_RED_ALERT_TD_TEXTBLIT_H_
#define CNC_RED_ALERT_TD_TEXTBLIT_H_

#define MAX_ENTRIES 128
#include "sdllib/include/gbuffer.h"

class TextBlitClass {
 public:
  TextBlitClass();
  ~TextBlitClass() {};

  void Add(int x, int y, int dx, int dy, int w, int h);
  void Clear();
  void Update();

 private:
  typedef struct {
    int SourceX;
    int SourceY;
    int DestX;
    int DestY;
    int Width;
    int Height;
  } BlitEntryType;

  BlitEntryType BlitListo[MAX_ENTRIES];
  int Count;
};

extern GraphicBufferClass* TextPrintBuffer;
extern TextBlitClass BlitList;

#endif  // CNC_RED_ALERT_TD_TEXTBLIT_H_
