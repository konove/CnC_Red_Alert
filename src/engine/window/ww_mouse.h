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
// HardwareCursor.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_

#include <cstddef>
#include <memory>
#include <span>

class SDL_Cursor;
class SDL_Surface;

// Deleters that let std::unique_ptr own SDL cursors and surfaces.
struct SdlCursorDeleter {
  void operator()(SDL_Cursor* cursor) const noexcept;
};

struct SdlSurfaceDeleter {
  void operator()(SDL_Surface* surface) const noexcept;
};

using SdlCursorPtr = std::unique_ptr<SDL_Cursor, SdlCursorDeleter>;
using SdlSurfacePtr = std::unique_ptr<SDL_Surface, SdlSurfaceDeleter>;

// The hardware cursor that replaces the original's software cursor.
//
// The game hands it cursor shapes and hides and shows it in nested pairs, as it
// did the software cursor; the OS does the drawing. Each game's Input owns the
// one instance (see Input::InstallMouse()); constructing it grabs the mouse
// into the window and makes it the instance the free functions below reach, and
// destroying it lets go of the mouse. Both need an installed Display.
class HardwareCursor {
 public:
  HardwareCursor(int max_width, int max_height);
  HardwareCursor(const HardwareCursor&) = delete;
  HardwareCursor& operator=(const HardwareCursor&) = delete;
  HardwareCursor(HardwareCursor&&) = delete;
  HardwareCursor& operator=(HardwareCursor&&) = delete;
  ~HardwareCursor();

  // Decodes a game cursor shape and makes it the SDL cursor. `shape` is a
  // Shape_Type header followed by its LCW-compressed pixels; the hot spot is in
  // the shape's own, unscaled pixels. Only 256-color shapes (ShapeType 0) no
  // larger than the size given to the constructor are accepted. Anything else,
  // and a shape whose data is corrupt, leaves the previous cursor in place.
  // Handing over the same buffer as last time does nothing: the cursor is
  // recognized by its address, not its contents.
  void SetShape(int hot_x, int hot_y, std::span<const std::byte> shape);

  // Reference-counted visibility control. Hide increments the hide count,
  // Show decrements it, and the cursor is visible only while the count is zero,
  // so nested pairs work. Show at a count of zero does nothing. Showing the
  // cursor applies a palette change that arrived while it was hidden.
  void Hide();
  void Show();

  // Returns the current hide count (0 = visible).
  [[nodiscard]] int hide_count() const;

  // The pointer's last position that RecordPosition() recorded, in the game's
  // screen pixels.
  [[nodiscard]] int x() const;
  [[nodiscard]] int y() const;

  // Confines the mouse to the game window, unless mouse_grab_disabled is set,
  // or releases it. Both act on TheDisplay().
  static void GrabMouse();
  static void ReleaseMouse();

  // Recolors the cursor with the display's current palette. SDL copies the
  // colors into a cursor when it creates one, so this creates the cursor again.
  // While the cursor is hidden the work waits for Show(); fades change
  // the palette every frame with the cursor hidden. Does nothing before the
  // first cursor or without a display palette.
  void UpdatePalette();

  // Records the pointer's position for x() and y().
  void RecordPosition(int x, int y);

 private:
  // Makes sdl_surface_ the SDL cursor, with the hot spot scaled like it.
  void RebuildCursor();

  // The cursor SDL is showing, or null before the first SetShape().
  SdlCursorPtr sdl_cursor_;
  // The scaled 8-bit image sdl_cursor_ was made from, kept so that
  // UpdatePalette() can recolor it and make the cursor again.
  SdlSurfacePtr sdl_surface_;
  // The hot spot in sdl_surface_'s pixels: SetShape()'s, scaled like the shape.
  int scaled_hot_x_ = 0;
  int scaled_hot_y_ = 0;

  // The largest shape SetShape() accepts, in pixels.
  int max_width_;
  int max_height_;

  // The buffer of the shape now showing, compared by address so that setting
  // the same shape every frame costs nothing. nullptr before the first one.
  const std::byte* current_shape_ = nullptr;

  // Reference count for Hide/Show. Cursor visible when hide_count_ == 0.
  int hide_count_ = 0;

  // The pointer position, as RecordPosition() last recorded it.
  int x_ = 0;
  int y_ = 0;

  // Set when the palette changes while the cursor is hidden; the next Show()
  // that makes it visible calls UpdatePalette().
  bool palette_stale_ = false;
};

// The interface the games use. Each forwards to the live HardwareCursor and,
// when there is none, does nothing or returns 0.
void HideMouse();
void ShowMouse();
int MouseHideCount();
void SetMouseShape(int hot_x, int hot_y, std::span<const std::byte> shape);
int MouseX();
int MouseY();

// Returns whether the left or right mouse button is held down right now, read
// from SDL's live state rather than from the queued clicks.
bool IsLeftButtonDown();
bool IsRightButtonDown();

// Display::SetPalette() calls this after every palette change.
void UpdateMousePalette();
// The event loop reports every mouse motion here.
void RecordMousePosition(int x, int y);

// Leaves the mouse free to leave the window, for debugging. Set from the
// command line before the mouse is created.
extern bool mouse_grab_disabled;

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_WW_MOUSE_H_
