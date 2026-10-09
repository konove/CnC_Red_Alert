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
#include "td/assets/theme.h"
#include "td/core/defines.h"
#include "td/editor/mapedit.h"
#include "td/map/world.h"
#include "td/startup/game_state.h"
#include "td/startup/winstub.h"

namespace td {

static ThemeType OldTheme = THEME_NONE;

/***********************************************************************************************
 * Focus_Loss -- this function is called when a library function detects focus
 *loss            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing *
 *                                                                                             *
 * OUTPUT:   Nothing *
 *                                                                                             *
 * WARNINGS: None *
 *                                                                                             *
 * HISTORY: * 2/1/96 2:10PM ST : Created *
 *=============================================================================================*/

void Focus_Loss() {
  if (TheGameState().sound_on() && (OldTheme == THEME_NONE)) {
    OldTheme = TheTheme().What_Is_Playing();
  }

  TheTheme().Stop();
  engine::audio::TheAudio().Pause();
  engine::window::HardwareCursor::ReleaseMouse();
}

void Focus_Restore() {
  TheMap().Flag_To_Redraw(true);
  engine::audio::TheAudio().Resume();

  if (!TheGameState().in_movie()) {
    TheTheme().Queue_Song(OldTheme);
    OldTheme = THEME_NONE;
  }

  engine::window::HardwareCursor::GrabMouse();
}

}  // namespace td
