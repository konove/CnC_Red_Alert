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

// File: the default VQA player configuration.
//
// Originally written by Bill Randolph and Denzil E. Long, Jr. at Westwood
// Studios, April 1995.

#include "winvq/vqa32/vqaplay.h"

void SetVqaConfigDefaults(VqaConfig* config) {
  *config = VqaConfig{};
  config->image_width = 320;
  config->image_height = 200;
  // Center the image in the buffer.
  config->margin_x = -1;
  config->margin_y = -1;
  // Load and draw at the movie's own frame rate.
  config->frame_rate = -1;
  config->draw_rate = -1;
  config->clock_source = kVqaClockDefault;
  config->option_flags = kVqaOptionAudio;
  config->frame_buffer_count = 6;
  config->codebook_buffer_count = 3;
  // Size the audio ring from the movie's sound.
  config->audio_buffer_bytes = -1;
  config->audio_block_bytes = 2048;
}
