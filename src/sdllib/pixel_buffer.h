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

// File: PixelBuffer, the pages the games draw on, and PixelView, the windows
// onto them.
//
// A PixelBuffer owns a rectangle of 8-bit paletted pixels: either plain
// memory or, for the one buffer the window shows, a PixelSurface whose pixels
// exist only while it is locked. A PixelView is a rectangular window onto such
// a buffer. Coordinates passed to a view are relative to its corner and are
// clipped to it, so the drawing primitives need to know nothing about the page
// behind it. Drawing is a view's job alone: a buffer owns pixels and hands out
// the view covering the whole of itself through view().
//
// Every drawing primitive comes in two forms: a public one that locks the
// buffer around the work, and a `…Locked` one that does the work and expects
// the caller to hold the lock already.
//
// PixelView is declared first so that PixelBuffer can hold one by value.

#ifndef CNC_RED_ALERT_SDLLIB_PIXEL_BUFFER_H_
#define CNC_RED_ALERT_SDLLIB_PIXEL_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include "absl/base/attributes.h"
#include "base/array.h"
#include "sdllib/bitmap.h"
#include "sdllib/pixel_surface.h"
#include "sdllib/text_window.h"

// The VGA mode the games were written for. Both still decode their
// low-resolution movies at this size, whatever video mode is set.
inline constexpr int kDefaultScreenWidth = 320;
inline constexpr int kDefaultScreenHeight = 200;

struct FontStyle;
class PixelBuffer;
class PixelView;

// A rectangular window onto a PixelBuffer. Coordinates passed to the
// drawing members are relative to the window's top left corner and are
// clipped to it, so the same primitives serve the full page, the map area and
// a dialog box.
//
// A view is not usable until Attach() (or the four-argument constructor)
// binds it to a buffer; a default-constructed one has no buffer. It does not
// own the pixels and must not outlive the buffer it is attached to.
//
// Example:
//   PixelView view(&page, 0, 0, 320, 200);
//   view.Clear();
//   view.BlitTo(TheScreen().visible_view());
class PixelView {
 public:
  // Attaches the view to `buffer` at x,y with the given size; see
  // Attach() for how the rectangle is clamped to the buffer.
  PixelView(PixelBuffer* buffer, int x, int y, int width, int height);
  PixelView() = default;
  ~PixelView() = default;
  PixelView(const PixelView&) = delete;
  PixelView& operator=(const PixelView&) = delete;
  PixelView(PixelView&&) = delete;
  PixelView& operator=(PixelView&&) = delete;

  // Binds the view to the given rectangle of `buffer`, clamping it
  // to the buffer's bounds.
  void Attach(PixelBuffer* buffer, int x, int y, int width, int height);

  // Where the view sits in the buffer, and how its rows are laid out.

  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }
  [[nodiscard]] int x_pos() const { return x_pos_; }
  [[nodiscard]] int y_pos() const { return y_pos_; }
  // The bytes of the buffer's row that fall outside the view, that is the
  // buffer's width minus width().
  [[nodiscard]] int x_add() const { return x_add_; }
  // Padding the buffer keeps past the end of every row, beyond x_add().
  [[nodiscard]] int pitch() const { return pitch_; }
  // Bytes from the start of one row of the view to the start of the
  // next: the view's width plus everything the buffer keeps past it.
  [[nodiscard]] int stride() const { return width_ + x_add_ + pitch_; }

  // The buffer this view draws into, and its pixels.

  PixelBuffer* buffer() { return buffer_; }
  // A raw pointer to the view's top left pixel, valid only while the
  // buffer is locked. Prefer pixels().
  uint8_t* offset() { return offset_; }
  // The buffer's pixels from this view's top left corner to the end of
  // the buffer. Rows are stride() bytes apart. Empty if the view is not
  // attached to a buffer.
  std::span<uint8_t> pixels();

  // Locks the buffer's surface so its pixels can be read or written, and
  // reattaches this view to them, since locking can move them. Locks
  // nest: the surface is only really locked and unlocked by the outermost
  // pair. Lock() returns false if the view has no buffer, or if the
  // surface could not be locked, in which case the matching Unlock() must
  // not be called.
  bool Lock();
  void Unlock();
  // How deep the buffer's nested locks are; 0 when the view has no buffer.
  [[nodiscard]] int lock_count() const;
  // Whether drawing to this view has to lock a surface first.
  [[nodiscard]] bool NeedsLock() const;

  // The drawing primitives. Each comes as a pair: the plain name locks the
  // buffer, does the work and unlocks it, while the `…Locked` name does only
  // the work and requires the caller to hold the lock. The short overloads
  // fill in "the whole view" for the missing rectangle. The locking form is a
  // one-line wrapper, so a caller that draws many times can take the lock once
  // and call the locked form in a loop.

  // Sets one pixel, ignoring coordinates outside the view.
  void PutPixel(int x, int y, uint8_t color);
  void PutPixelLocked(int x, int y, uint8_t color);

  // Returns the palette index at x,y, or 0 outside the view.
  int GetPixel(int x, int y);
  int GetPixelLocked(int x, int y);

  void Clear(uint8_t color = 0);
  void ClearLocked(uint8_t color);

  // Copies the width x height rectangle at src_x,src_y in the view out to
  // plain memory, packed with no padding. The rectangle is clipped to the
  // view first, and a rectangle that misses the view entirely copies
  // nothing.
  void CopyToBuffer(int src_x, int src_y, int width, int height,
                    std::span<uint8_t> dest);
  void CopyToBufferLocked(int src_x, int src_y, int width, int height,
                          std::span<uint8_t> dest);

  // The other direction: copies a width x height image from plain memory
  // into the view at dst_x,dst_y, clipping it to the view. `source` is
  // packed with no padding.
  void CopyFromBuffer(int dst_x, int dst_y, int width, int height,
                      std::span<const uint8_t> source);
  void CopyFromBufferLocked(int dst_x, int dst_y, int width, int height,
                            std::span<const uint8_t> source);

  // Copies width x height pixels from src_x,src_y in this view to
  // dst_x,dst_y in `dest`, clipping to both. With `transparent`,
  // pixel 0 is left alone in the destination instead of being copied.
  // Copies nothing if either view cannot be locked.
  void BlitTo(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
              int width, int height, bool transparent = false);
  // Copies the whole view to the top left corner of `dest`.
  void BlitTo(PixelView& dest) { BlitTo(dest, 0, 0, 0, 0, width_, height_); }
  // Both views must be locked. Overlapping source and destination are
  // handled, so this also serves as a scroll within one view.
  void BlitToLocked(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
                    int width, int height, bool transparent);

  // Stretches or shrinks the src_width x src_height rectangle at src_x,src_y
  // in this view onto the dst_width x dst_height rectangle at dst_x,dst_y in
  // `dest`, clipping to both. With `transparent`, pixel 0 is left alone in the
  // destination; a non-empty `remap_table` translates every pixel through it.
  void Scale(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
             int src_width, int src_height, int dst_width, int dst_height,
             bool transparent = false,
             std::span<const uint8_t> remap_table = {});
  void ScaleLocked(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
                   int src_width, int src_height, int dst_width, int dst_height,
                   bool transparent, std::span<const uint8_t> remap_table);

  // Draws text in `style` at x,y, wrapping to a new line when the text runs
  // past the view's width. `fore_color` and `back_color` are palette indices
  // that stand in for glyph values 1 and 0, and a `back_color` of 0 leaves the
  // background untouched; the integer overload prints the number in decimal.
  // Does nothing if `text` is null or the style has no font.
  void Print(const FontStyle& style, const char* text, int x, int y,
             int fore_color, int back_color);
  void Print(const FontStyle& style, int value, int x, int y, int fore_color,
             int back_color);
  void PrintLocked(const FontStyle& style, const char* text, int x, int y,
                   int fore_color, int back_color);

  // x1,y1 and x2,y2 are the two corners, both inclusive, so DrawRect and
  // FillRect cover x2 - x1 + 1 pixels per row.
  void DrawLine(int x1, int y1, int x2, int y2, uint8_t color);
  void DrawRect(int x1, int y1, int x2, int y2, uint8_t color);
  void FillRect(int x1, int y1, int x2, int y2, uint8_t color);
  void DrawLineLocked(int x1, int y1, int x2, int y2, uint8_t color);
  void FillRectLocked(int x1, int y1, int x2, int y2, uint8_t color);

  // Replaces every pixel in the rectangle with remap_table[pixel].
  // `remap_table` is a 256-entry table; the shorter overload covers the whole
  // view.
  void Remap(int x1, int y1, int width, int height,
             std::span<const uint8_t> remap_table);
  void Remap(const std::span<const uint8_t> remap_table) {
    Remap(0, 0, width_, height_, remap_table);
  }
  void RemapLocked(int x1, int y1, int width, int height,
                   std::span<const uint8_t> remap_table);

  // Draws cell `cell` of an icon set (a terrain template) at x,y, clipped to
  // the WindowList entry `clip_window` rather than to the view - the map
  // draws its terrain through this. `remap_table` may be empty for no
  // remapping.
  void DrawStamp(std::span<const std::byte> icon_set, int cell, int x, int y,
                 std::span<const uint8_t> remap_table, int clip_window);
  // Clipped to the rectangle at clip_x,clip_y in view coordinates rather
  // than to a WindowList entry. x,y are relative to the rectangle's corner.
  // The set's map translates `cell` into the tile drawn; an empty cell, a
  // cell or a tile outside the set, damaged set data, or a `remap_table`
  // shorter than 256 draws nothing. Color 0 is
  // transparent when the tile is flagged transparent, and whenever
  // `remap_table` is given (tested after the remap). The buffer must be
  // locked.
  void DrawStampLocked(std::span<const std::byte> icon_set, int cell, int x,
                       int y, std::span<const uint8_t> remap_table, int clip_x,
                       int clip_y, int clip_width, int clip_height);

 private:
  // The view's top left pixel within the buffer. Null while the buffer
  // is a surface that is not currently locked.
  uint8_t* offset_ = nullptr;
  int width_ = 0;
  int height_ = 0;
  // The bytes of the buffer's row that fall outside the view, that is
  // the buffer's width minus width_. Together with pitch_ it turns the end of
  // one row of the view into the start of the next.
  int x_add_ = 0;
  // Where the view sits in the buffer.
  int x_pos_ = 0;
  int y_pos_ = 0;
  // Padding the buffer keeps past the end of every row, beyond x_add_. Copied
  // from the buffer, and zero for every buffer the games create.
  int pitch_ = 0;
  // The buffer this view draws into; null until Attach().
  PixelBuffer* buffer_ = nullptr;
};

// An allocated page of 8-bit paletted pixels. Both games keep a handful: the
// visible page, the hidden page the frame is composed on, and the staging
// pages movies decode into.
//
// The pixels come from one of three places, chosen by Init(): a span the
// caller owns, a block the buffer allocates and owns, or a PixelSurface -
// the window - whose pixels only exist between LockSurface() and
// UnlockSurface().
//
// A buffer draws nothing itself; view() hands out the view covering all of
// it, and every drawing primitive lives there.
//
// Example:
//   PixelBuffer page(320, 200);
//   page.view().Clear();
class PixelBuffer {
 public:
  // Sizes the buffer and gives it `buffer`'s pixels, or allocates `size`
  // bytes (width * height when `byte_count` is zero) if `buffer` is empty.
  PixelBuffer(int width, int height, std::span<uint8_t> buffer,
              int32_t byte_count);
  PixelBuffer(int width, int height, std::span<uint8_t> buffer = {});
  // Leaves the buffer empty; Init() gives it pixels later. Screen's pages
  // are built this way, before there is a window to size them against.
  PixelBuffer();
  ~PixelBuffer() = default;

  PixelBuffer(const PixelBuffer&) = delete;
  PixelBuffer& operator=(const PixelBuffer&) = delete;
  PixelBuffer(PixelBuffer&&) = delete;
  PixelBuffer& operator=(PixelBuffer&&) = delete;

  // Gives the buffer its pixels, replacing whatever it had: it takes
  // `buffer`, or allocates `byte_count` bytes (width * height when zero) when
  // `buffer` is empty. CHECK-fails if the pixels are too few for
  // width * height.
  void Init(int width, int height, std::span<uint8_t> buffer,
            int32_t byte_count);
  // Makes the buffer a width x height page of `surface`, whose pixels it
  // borrows between LockSurface() and UnlockSurface() and otherwise has none.
  // The surface must outlive the buffer, or be replaced by a later Init()
  // first; a lock CHECK-fails if the surface is smaller than the page.
  void Init(int width, int height, PixelSurface& surface);

  // The view covering the whole buffer. Everything that draws goes through a
  // view; this is the one for callers that want the entire page.
  PixelView& view() ABSL_ATTRIBUTE_LIFETIME_BOUND { return whole_; }

  [[nodiscard]] int width() const { return width_; }
  [[nodiscard]] int height() const { return height_; }
  // Padding kept past the end of every row: zero for memory, and whatever the
  // surface keeps past width() for a surface's buffer. A view copies it so its
  // rows still line up.
  [[nodiscard]] int pitch() const { return pitch_; }
  // The buffer's whole allocation. Empty before Init(), and empty for a
  // surface's buffer while the surface is unlocked.
  [[nodiscard]] std::span<uint8_t> bytes() { return bytes_; }

  // Whether the pixels belong to a PixelSurface, that is whether the buffer
  // has to be locked before they can be touched.
  [[nodiscard]] bool HasSurface() const { return surface_ != nullptr; }

  // Locks and unlocks the attached surface. Locks nest: only the outermost
  // pair reaches the surface. LockSurface() returns false if the surface
  // could not be locked, in which case the matching UnlockSurface() must not
  // be called. Both succeed and count nothing for a buffer without a surface.
  // Callers normally use PixelView::Lock/Unlock, which also reattach the view
  // to the freshly locked pixels.
  bool LockSurface();
  void UnlockSurface();
  // How deep the nested LockSurface() calls are; the surface is locked while
  // this is non-zero.
  [[nodiscard]] int lock_count() const { return lock_count_; }

  // Draws `bitmap` onto this buffer with its centre landing on `center`, scaled
  // and rotated. `scale` is 24.8 fixed point (0x100 = 1.0) and is ignored when
  // zero; `angle` is 0-255 over the full circle. Pixel 0 is transparent.
  // Whatever falls outside the buffer is dropped, so a bitmap that does not
  // fit is silently cropped.
  void DrawScaledRotated(const BitmapClass& bitmap, const TPoint2D& center,
                         int32_t scale, uint8_t angle);

 private:
  int width_ = 0;
  int height_ = 0;
  // Padding kept past the end of every row; zero for memory, set from the
  // surface's rows by LockSurface().
  int pitch_ = 0;
  // How deep the nested LockSurface() calls are; the surface is locked while
  // this is non-zero.
  int lock_count_ = 0;

  // The pixels, wherever they came from: owned_pixels_, the caller's span, or
  // the locked surface. Empty while surface_ is unlocked.
  std::span<uint8_t> bytes_;
  // The allocation behind bytes_ when the buffer allocated its own pixels;
  // null when the pixels are the caller's or the surface's.
  std::unique_ptr<uint8_t[]> owned_pixels_;
  // Where the pixels come from while locked; null for a buffer of plain
  // memory. Not owned.
  PixelSurface* surface_ = nullptr;

  // The view onto all of this buffer. Attached to `this` by the constructors
  // and re-attached whenever the pixels move: Init(), LockSurface() and
  // UnlockSurface().
  PixelView whole_;
};

// Inline rather than in the .cc because the window unit differs between the
// two games, and sdllib is compiled once, without TD defined.
inline void PixelView::DrawStamp(const std::span<const std::byte> icon_set,
                                 const int cell, const int x, const int y,
                                 const std::span<const uint8_t> remap_table,
                                 const int clip_window) {
  // Tiberian Dawn stores a window's x and width in units of eight pixels;
  // Red Alert stores them in pixels.
#ifdef TD
  constexpr int kWindowUnit = 8;
#else
  constexpr int kWindowUnit = 1;
#endif
  if (Lock()) {
    DrawStampLocked(
        icon_set, cell, x, y, remap_table,
        base::At(base::At(WindowList, clip_window), kWindowX) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowHeight));
    Unlock();
  }
}

#endif  // CNC_RED_ALERT_SDLLIB_PIXEL_BUFFER_H_
