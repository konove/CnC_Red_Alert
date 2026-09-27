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
// so every palette change creates the cursor again.

#ifndef CNC_RED_ALERT_ENGINE_WINDOW_HARDWARE_CURSOR_H_
#define CNC_RED_ALERT_ENGINE_WINDOW_HARDWARE_CURSOR_H_

#include <cstddef>
#include <memory>
#include <span>

#include "absl/base/attributes.h"

class SDL_Cursor;
class SDL_Surface;

namespace engine::window {

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
// one instance for the Game's lifetime, and the game feeds it what the engine
// does not route here: mouse motion and palette changes. Creating one touches
// neither SDL nor the display, so a test can create its own.
//
// Example:
//   mouse.Hide();
//   ... draw ...
//   mouse.Show();
class HardwareCursor {
 public:
  // Shows the cursor for the scope's lifetime, however many Hide() calls are
  // outstanding, and puts the hide count back as it was when the scope ends.
  // For a prompt that interrupts code which has the cursor hidden, such as a
  // request to insert a CD in the middle of loading.
  //
  // Example:
  //   {
  //     const HardwareCursor::ShownScope shown(TheMouse());
  //     WWMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
  //   }
  class [[nodiscard]] ShownScope {
   public:
    explicit ShownScope(HardwareCursor& cursor ABSL_ATTRIBUTE_LIFETIME_BOUND);
    ~ShownScope();

    ShownScope(const ShownScope&) = delete;
    ShownScope& operator=(const ShownScope&) = delete;
    ShownScope(ShownScope&&) = delete;
    ShownScope& operator=(ShownScope&&) = delete;

   private:
    HardwareCursor& cursor_;
    int saved_hide_count_;
  };

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

  // Shows the cursor however many Hide() calls are outstanding, dropping the
  // count to 0 for good. For a screen that takes the cursor over from here
  // on, such as a menu; a prompt that should hand the cursor back hidden
  // uses a ShownScope instead.
  void ForceShow();

  // Returns the current hide count (0 = visible).
  [[nodiscard]] int hide_count() const { return hide_count_; }

  // The pointer's last position that RecordPosition() recorded, in the game's
  // screen pixels.
  [[nodiscard]] int x() const { return x_; }
  [[nodiscard]] int y() const { return y_; }

  // Confines the mouse to the game window, unless mouse_grab_disabled is set,
  // or releases it. Both act on TheDisplay(); the games grab once the window
  // is open, release on shutdown, and release and grab again as the window
  // loses and regains focus.
  static void GrabMouse();
  static void ReleaseMouse();

  // Recolors the cursor with the display's current palette. SDL copies the
  // colors into a cursor when it creates one, so this creates the cursor again.
  // While the cursor is hidden the work waits for Show(); fades change
  // the palette every frame with the cursor hidden. Does nothing before the
  // first cursor or without a display palette. The games call it after every
  // Display::SetPalette().
  void UpdatePalette();

  // Records the pointer's position for x() and y(). The games' event handlers
  // report every mouse motion here.
  void RecordPosition(int x, int y) {
    x_ = x;
    y_ = y;
  }

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

// Returns whether the left or right mouse button is held down right now, read
// from SDL's live state rather than from the queued clicks.
bool IsLeftButtonDown();
bool IsRightButtonDown();

// Leaves the mouse free to leave the window, for debugging. Set from the
// command line before the window opens.
extern bool mouse_grab_disabled;

}  // namespace engine::window

#endif  // CNC_RED_ALERT_ENGINE_WINDOW_HARDWARE_CURSOR_H_
