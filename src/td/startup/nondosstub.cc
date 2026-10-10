// parts of winstub that didn't depend on windows
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

#include <cstdio>
#include <cstring>

#include "engine/audio/audio_mixer.h"
#include "engine/window/hardware_cursor.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/startup/winstub.h"

namespace td {

// Pausing the mixer holds every sound, the score included, where it is, so
// the music carries on from the same note when the focus comes back.
void Focus_Loss() {
  engine::audio::TheAudio().Pause();
  engine::window::HardwareCursor::ReleaseMouse();
}

void Focus_Restore() {
  TheMap().Flag_To_Redraw(true);
  engine::audio::TheAudio().Resume();
  engine::window::HardwareCursor::GrabMouse();
}

}  // namespace td
