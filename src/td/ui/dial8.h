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

/* $Header:   F:\projects\c&c\vcs\code\dial8.h_v   2.18   16 Oct 1995 16:47:28
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : DIAL8.H *
 *                                                                                             *
 *                   Programmer : Bill Randolph
 **
 *                                                                                             *
 *                   Start Date : 02/06/95 *
 *                                                                                             *
 *                  Last Update : February 6, 1995 [BR]
 **
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_TD_UI_DIAL8_H_
#define CNC_RED_ALERT_TD_UI_DIAL8_H_

#include "engine/gfx/pixel_buffer.h"
#include "engine/window/keyboard.h"
#include "game/ui/control.h"
#include "td/core/defines.h"

namespace td {

class Dial8Class : public game::ControlClass {
 public:
  /*
  ** Constructor/Destructor
  */
  Dial8Class(int id, int x, int y, int w, int h, game::Direction dir);

  /*
  ** Get/Set the direction the dial is currently pointing
  */
  [[nodiscard]] game::Direction Get_Direction() const;
  void Set_Direction(game::Direction dir);

  /*
  ** Overloaded draw routine
  */
  bool Draw(PixelView& view, bool forced = false) override;

 protected:
  /*
  ** Overloaded event processing routine
  */
  bool Action(InputFlags flags, engine::window::KeyNumber& key, int x,
              int y) override;

 private:
  int FaceX;             // x-coord of center of face
  int FaceY;             // y-coord of center of face
  int FacePoint[8][2]{};  // coords of the little dial decorations
  int FaceLine[8][2]{};  // coords for drawing the dial hand
  game::Direction Direction;   // 0-255 numerical direction of dial
  game::Facing Facing;         // numerical facing direction of dial (0 - 7)
  game::Facing OldFacing;      // previous Facing value
};

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_DIAL8_H_
