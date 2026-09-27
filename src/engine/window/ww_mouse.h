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

// The mouse cursor, drawn by the OS as an SDL hardware cursor.
//
// Converts the game's cursor shapes (LCW-compressed, zero-run-length encoded,
// 8-bit paletted) into SDL color cursors, scaled up by a whole factor on
// high-resolution displays. SDL bakes the colors in when it creates a cursor,
// so every palette change creates the cursor again. The free functions at the
// bottom are the interface the games use; they forward to the one live
// WWMouseClass.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "engine/gfx/pixel_buffer.h"

class SDL_Cursor;
class SDL_Surface;

// Deleters that let std::unique_ptr own SDL cursors and surfaces.
struct SDLCursorDeleter {
  void operator()(SDL_Cursor* p) const noexcept;
};

struct SDLSurfaceDeleter {
  void operator()(SDL_Surface* p) const noexcept;
};

using SDLCursorPtr = std::unique_ptr<SDL_Cursor, SDLCursorDeleter>;
using SDLSurfacePtr = std::unique_ptr<SDL_Surface, SDLSurfaceDeleter>;

// The hardware cursor that replaces the original's software cursor.
//
// It keeps the software cursor's interface: the game hands it cursor shapes,
// hides and shows it in nested pairs, and still calls the draw, erase and
// conditional-hide entry points, which do nothing now that the OS draws the
// cursor. Each game's Input owns the one instance (see Input::InstallMouse());
// constructing it grabs the mouse into the window and makes it the instance the
// free functions below reach, and destroying it lets go of the mouse. Both need
// an installed Display.
class WWMouseClass {
 public:
  WWMouseClass(PixelView* scr, int max_width, int max_height);
  WWMouseClass(const WWMouseClass&) = delete;
  WWMouseClass& operator=(const WWMouseClass&) = delete;
  WWMouseClass(WWMouseClass&&) = delete;
  WWMouseClass& operator=(WWMouseClass&&) = delete;
  ~WWMouseClass();

  // Decodes a game cursor shape and makes it the SDL cursor. `cursor` is a
  // Shape_Type header followed by its LCW-compressed pixels; the hot spot is in
  // the shape's own, unscaled pixels. Only 256-color shapes (ShapeType 0) no
  // larger than the size given to the constructor are accepted. Anything else,
  // and a shape whose data is corrupt, leaves the previous cursor in place.
  // Handing over the same buffer as last time does nothing: the cursor is
  // recognized by its address, not its contents.
  void Set_Cursor(int xhotspot, int yhotspot,
                  std::span<const std::byte> cursor);

  // Reference-counted visibility control. Hide increments the hide count,
  // Show decrements it, and the cursor is visible only while the count is zero,
  // so nested pairs work. Show at a count of zero does nothing. Showing the
  // cursor applies a palette change that arrived while it was hidden.
  void Hide_Mouse();
  void Show_Mouse();

  // Legacy API stubs. The original game used these for software cursor
  // clipping during screen updates; hardware cursors don't need this.
  void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
  void Conditional_Show_Mouse();

  // Returns the current hide count (0 = visible).
  [[nodiscard]] int Get_Mouse_State() const;

  // The pointer's last position that Update_Pos() recorded, in the game's
  // screen pixels.
  [[nodiscard]] int Get_Mouse_X() const;
  [[nodiscard]] int Get_Mouse_Y() const;

  // No-ops for hardware cursor. Kept for API compatibility with legacy code
  // that expected software cursor rendering.
  void Draw_Mouse(PixelView* scr);
  void Erase_Mouse(PixelView* scr, bool forced = false);

  // Confines the mouse to the game window, unless NoMouseGrab is set, or
  // releases it. Both act on TheDisplay().
  static void Set_Cursor_Clip();
  static void Clear_Cursor_Clip();

  // Recolors the cursor with the display's current palette. SDL copies the
  // colors into a cursor when it creates one, so this creates the cursor again.
  // While the cursor is hidden the work waits for Show_Mouse(); fades change
  // the palette every frame with the cursor hidden. Does nothing before the
  // first cursor or without a display palette.
  void Update_Palette();

  // Records the pointer's position for Get_Mouse_X() and Get_Mouse_Y().
  void Update_Pos(int x, int y);

 private:
  // Scratch space of MaxWidth * MaxHeight pixels that Set_Cursor() decodes a
  // shape into, unscaled; only the first Width * Height pixels are the shape.
  std::vector<uint8_t> MouseCursor;
  // The cursor SDL is showing, or null before the first Set_Cursor().
  SDLCursorPtr sdl_cursor_;
  // The scaled 8-bit image sdl_cursor_ was made from, kept so that
  // Update_Palette() can recolor it and make the cursor again.
  SDLSurfacePtr sdl_surface_;
  // The hot spot as Set_Cursor() received it, in unscaled shape pixels.
  int MouseXHot = 0;
  int MouseYHot = 0;

  // The decoded shape, unscaled, and its size in pixels. Set_Cursor() scales
  // it straight away; nothing reads it later.
  std::vector<uint8_t> OriginalCursor;
  int OriginalWidth = 0;
  int OriginalHeight = 0;
  // The factor the current cursor was scaled by, fixed when it was set.
  int CurrentScale = 1;

  // The largest shape Set_Cursor() accepts, in pixels.
  int MaxWidth;
  int MaxHeight;

  // The buffer of the shape now showing, compared by address so that setting
  // the same shape every frame costs nothing. nullptr before the first one.
  const std::byte* PrevCursor = nullptr;

  // Reference count for Hide/Show. Cursor visible when State == 0.
  int State{0};

  // The pointer position, as Update_Pos() last recorded it.
  int LastX = 0, LastY = 0;

  // Set when palette changes while cursor is hidden; triggers Update_Palette
  // on next Show_Mouse call.
  bool PaletteDirty = false;
};

// The interface the games use. Each forwards to the live WWMouseClass and,
// when there is none, does nothing or returns 0.
void Hide_Mouse();
void Show_Mouse();
void Conditional_Hide_Mouse(int x1, int y1, int x2, int y2);
void Conditional_Show_Mouse();
int Get_Mouse_State();
void Set_Mouse_Cursor(int hotx, int hoty, std::span<const std::byte> cursor);
int Get_Mouse_X();
int Get_Mouse_Y();

// Returns whether the left or right mouse button is held down right now, read
// from SDL's live state rather than from the queued clicks.
bool IsLeftButtonDown();
bool IsRightButtonDown();

// Display::SetPalette() calls this after every palette change.
void Update_Mouse_Palette();
// The event loop reports every mouse motion here.
void Update_Mouse_Pos(int x, int y);

// Leaves the mouse free to leave the window, for debugging. Set from the
// command line before the mouse is created.
extern bool NoMouseGrab;

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_
