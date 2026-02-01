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

// SDL2-based mouse cursor management for the Westwood game engine.
//
// Converts game cursor shapes (LCW+RLE compressed, 8-bit paletted) into SDL
// hardware cursors with automatic scaling for high-DPI displays. The cursor
// must be recreated when the game palette changes since SDL bakes colors in
// at cursor creation time.

#ifndef CNC_RED_ALERT_SDLLIB_WW_MOUSE_H_
#define CNC_RED_ALERT_SDLLIB_WW_MOUSE_H_

#include <cstdint>
#include <memory>
#include <vector>

#include "sdllib/include/gbuffer.h"

class SDL_Cursor;
class SDL_Surface;

// Custom deleters for SDL resource RAII wrappers.
struct SDLCursorDeleter {
  void operator()(SDL_Cursor* p) const noexcept;
};

struct SDLSurfaceDeleter {
  void operator()(SDL_Surface* p) const noexcept;
};

using SDLCursorPtr = std::unique_ptr<SDL_Cursor, SDLCursorDeleter>;
using SDLSurfacePtr = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

// Manages hardware mouse cursor rendering via SDL2.
//
// This class bridges the original game's software cursor system with modern
// hardware cursors. It decodes game cursor shapes, scales them for high-DPI
// displays, and handles palette synchronization.
//
// Hide/Show operations are reference-counted to support nested hide/show pairs.
class WWMouseClass {
 public:
  WWMouseClass(GraphicViewPortClass* scr, int max_width, int max_height);
  WWMouseClass(const WWMouseClass&) = delete;
  WWMouseClass& operator=(const WWMouseClass&) = delete;
  WWMouseClass(WWMouseClass&&) = delete;
  WWMouseClass& operator=(WWMouseClass&&) = delete;
  ~WWMouseClass();

  // Decodes a game cursor shape and creates an SDL hardware cursor.
  // The cursor pointer must be a Shape_Type structure (LCW+RLE compressed).
  void Set_Cursor(int xhotspot, int yhotspot, void* cursor);

  // Reference-counted visibility control. Hide increments the hide count,
  // Show decrements it. The cursor is only visible when the count is zero.
  void Hide_Mouse();
  void Show_Mouse();

  // Legacy API stubs. The original game used these for software cursor
  // clipping during screen updates; hardware cursors don't need this.
  void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
  void Conditional_Show_Mouse();

  // Returns the current hide count (0 = visible).
  int Get_Mouse_State();

  int Get_Mouse_X();
  int Get_Mouse_Y();

  // No-ops for hardware cursor. Kept for API compatibility with legacy code
  // that expected software cursor rendering.
  void Draw_Mouse(GraphicViewPortClass* scr);
  void Erase_Mouse(GraphicViewPortClass* scr, bool forced = false);

  // Controls mouse confinement to the game window.
  void Set_Cursor_Clip();
  void Clear_Cursor_Clip();

  // Recreates the SDL cursor with the current game palette.
  void Update_Palette();

  void Update_Pos(int x, int y);

 private:
  // Legacy flags for conditional hide (unused with hardware cursors).
  enum {
    CONDHIDE = 1,
    CONDHIDDEN = 2,
  };

  // Decoded cursor pixels (scaled for current display).
  std::vector<uint8_t> MouseCursor;
  SDLCursorPtr sdl_cursor_;
  SDLSurfacePtr sdl_surface_;
  int MouseXHot;
  int MouseYHot;

  // Original unscaled cursor, retained for palette updates and rescaling.
  std::vector<uint8_t> OriginalCursor;
  int OriginalWidth = 0;
  int OriginalHeight = 0;
  int CurrentScale = 1;

  int MaxWidth;
  int MaxHeight;

  // Tracks cursor identity to avoid redundant Set_Cursor work.
  char* PrevCursor;

  // Reference count for Hide/Show. Cursor visible when State == 0.
  int State;

  int LastX = 0, LastY = 0;

  // Set when palette changes while cursor is hidden; triggers Update_Palette
  // on next Show_Mouse call.
  bool PaletteDirty = false;
};

// Global C-style API for legacy game code. These delegate to the singleton
// WWMouseClass instance.
void Hide_Mouse();
void Show_Mouse();
void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
void Conditional_Show_Mouse();
int Get_Mouse_State();
void Set_Mouse_Cursor(int hotx, int hoty, void* cursor);
int Get_Mouse_X();
int Get_Mouse_Y();

void Update_Mouse_Palette();
void Update_Mouse_Pos(int x, int y);

#endif  // CNC_RED_ALERT_SDLLIB_WW_MOUSE_H_
