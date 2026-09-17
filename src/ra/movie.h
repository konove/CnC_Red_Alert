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
#ifndef CNC_RED_ALERT_RA_MOVIE_H_
#define CNC_RED_ALERT_RA_MOVIE_H_

// File: VQA movie playback.

#include <cstdint>

#include "ra/defines.h"

// Set by the -NOMOVIES command line switch; suppresses movie playback.
inline bool bNoMovies = false;

// Plays a VQA movie, returning only once it has finished. name is the movie
// filename without its extension. theme optionally names background music to
// start first; clrscrn asks for the screen to be cleared afterwards.
//
// Does nothing outside of a normal (non-multiplayer) game or in the map editor.
void Play_Movie(const char* name, ThemeType theme = THEME_NONE,
                bool clear_screen = true);
void Play_Movie(VQType name, ThemeType theme = THEME_NONE,
                bool clear_screen = true);

// Per-frame callback installed into the VQA player. Scales the decoded frame
// onto the visible page, and returns non-zero to abort the movie (ESC, when
// breaking out is allowed).
int32_t VQ_Call_Back(unsigned char* buffer = nullptr, int32_t frame = 0);
int32_t VQ_Event_Handler(uint32_t event, void* buffer, int32_t nbytes);

#endif  // CNC_RED_ALERT_RA_MOVIE_H_
