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
#include <optional>
#include <span>
#include <string_view>

#include "engine/audio/audio_mixer.h"
#include "engine/gfx/palette.h"
#include "engine/gfx/pcx_file.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/window/hardware_cursor.h"
#include "ra/assets/graphics_loader.h"
#include "ra/assets/theme.h"
#include "ra/editor/mapedit.h"
#include "ra/map/world.h"
#include "ra/startup/winstub.h"

namespace ra {

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
  TheTheme().Suspend();
  engine::audio::TheAudio().Pause();
  engine::window::HardwareCursor::ReleaseMouse();
}

void Focus_Restore() {
  TheMap().Flag_To_Redraw(true);
  engine::audio::TheAudio().Resume();
  engine::window::HardwareCursor::GrabMouse();
}

void Load_Title_Screen(std::string_view name, PixelView* video_page,
                       engine::gfx::Palette& palette) {
  if (const std::optional<engine::gfx::PcxPicture> picture =
          engine::gfx::ReadPcxFile(name)) {
    video_page->CopyFromBuffer(0, 0, picture->width, picture->height,
                               picture->pixels);
    palette = picture->palette;
  }
}

}  // namespace ra
