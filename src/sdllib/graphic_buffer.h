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
// memory or, for the one buffer the window shows, an SDL surface whose pixels
// exist only while it is locked. A PixelView is a rectangular window onto such
// a buffer. Coordinates passed to a view are relative to its corner and are
// clipped to it, so the drawing primitives need to know nothing about the page
// behind it. A PixelBuffer is also the view covering the whole of itself,
// which is why it derives from PixelView.
//
// The primitives themselves are the free Buffer_* functions in drawbuff.h.
// The inline members here exist to lock the surface around a call to one of
// them and to fill in "the whole view" for the shorter overloads.

#ifndef CNC_RED_ALERT_SDLLIB_GRAPHIC_BUFFER_H_
#define CNC_RED_ALERT_SDLLIB_GRAPHIC_BUFFER_H_

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "absl/strings/str_cat.h"
#include "base/array.h"
#include "base/attributes.h"
#include "base/flags.h"
#include "base/numeric.h"
#include "base/types.h"
#include "sdllib/bitmap.h"
#include "sdllib/buffer.h"
#include "sdllib/drawbuff.h"
#include "sdllib/ww_win.h"

// How PixelBuffer::Init() should back the buffer.
enum class CNC_FLAG_ENUM PixelBufferFlags {
  BUFFER_NONE = 0,
  // The buffer is the one the window shows. Init() creates an SDL surface
  // and texture for it instead of allocating memory, and records it in
  // WindowBuffer.
  BUFFER_VISIBLE = 2,
};
using enum PixelBufferFlags;
template <>
inline constexpr bool base::kIsFlagEnum<PixelBufferFlags> = true;

// The VGA mode the games were written for. Both still decode their
// low-resolution movies at this size, whatever video mode is set.
inline constexpr int kDefaultScreenWidth = 320;
inline constexpr int kDefaultScreenHeight = 200;

class PixelBuffer;

// Makes `page` the page the drawing code writes to, and returns the previous
// one so the caller can put it back.
PixelView* SetLogicPage(PixelView* page ABSL_ATTRIBUTE_LIFETIME_BOUND);
PixelView* SetLogicPage(PixelView& page ABSL_ATTRIBUTE_LIFETIME_BOUND);

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
//   view.Blit(TheScreen().visible_view());
class PixelView {
 public:
  // Attaches the view to `buffer` at x,y with the given size; see
  // Attach() for how the rectangle is clamped to the buffer.
  PixelView(PixelBuffer* buffer, int x, int y, int width, int height);
  PixelView() = default;
  // Resets LogicPage if it points here, so that nothing draws into a
  // view that is gone.
  ~PixelView();
  PixelView(const PixelView&) = delete;
  PixelView& operator=(const PixelView&) = delete;
  PixelView(PixelView&&) = delete;
  PixelView& operator=(PixelView&&) = delete;

  // A raw pointer to the view's top left pixel, valid only while the
  // buffer is locked. Prefer pixels().
  uint8_t* offset();
  // The buffer's pixels from this view's top left corner to the end of
  // the buffer. Rows are width() + x_add() + pitch() bytes apart.
  // Empty if the view is not attached to a buffer.
  std::span<uint8_t> pixels();
  [[nodiscard]] int height() const;
  [[nodiscard]] int width() const;
  [[nodiscard]] int x_add() const;
  // Bytes from the start of one row of the view to the start of the
  // next: the view's width plus everything the buffer keeps past it.
  [[nodiscard]] int stride() const;
  [[nodiscard]] int x_pos() const;
  [[nodiscard]] int y_pos() const;
  [[nodiscard]] int pitch() const;
  // Whether drawing to this view has to lock a surface first.
  inline bool NeedsLock();
  PixelBuffer* buffer();

  // The drawing primitives. Each locks the buffer, calls the matching
  // Buffer_* function from drawbuff.h, and unlocks it; the short overloads
  // fill in "the whole view" for the missing rectangle.

  // Sets one pixel, ignoring coordinates outside the view.
  // PutPixelLocked is the same without the lock, for callers that hold one
  // already.
  void PutPixel(int x, int y, uint8_t color);
  void PutPixelLocked(int x, int y, uint8_t color);
  // Returns the palette index at x,y, or 0 outside the view.
  int GetPixel(int x, int y);
  void Clear(uint8_t color = 0);

  // Copies a rectangle of the view out to plain memory, packed with no
  // padding, and returns the number of bytes written. The rectangle is
  // clipped to the view first, and nothing is written if `dest` is
  // smaller than what is left.
  int32_t CopyToBuffer(int x, int y, int width, int height,
                       std::span<uint8_t> dest, int32_t dest_size);

  // Copies width x height pixels from src_x,src_y in this view to
  // dst_x,dst_y in `dest`, clipping to both. With `transparent`,
  // pixel 0 is left alone in the destination instead of being copied.
  // Returns false if either view could not be locked.
  bool Blit(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
            int width, int height, bool transparent = false);
  bool Blit(PixelView& dest, int dst_x, int dst_y, bool transparent = false);
  bool Blit(PixelView& dest, bool transparent = false);

  bool Scale(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
             int src_width, int src_height, int dst_width, int dst_height,
             bool transparent = false,
             std::span<const uint8_t> remap_table = {});
  bool Scale(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
             int src_width, int src_height, int dst_width, int dst_height,
             std::span<const uint8_t> remap_table);
  bool Scale(PixelView& dest, bool transparent = false,
             std::span<const uint8_t> remap_table = {});
  bool Scale(PixelView& dest, std::span<const uint8_t> remap_table);

  // Draws text in the current font at x,y. `fore_color` and `back_color`
  // are palette indices; the integer overload prints the number in decimal.
  void Print(const char* text, int x, int y, int fore_color, int back_color);
  void Print(int value, int x, int y, int fore_color, int back_color);

  // x1,y1 and x2,y2 are the two corners, both inclusive, so DrawRect and
  // FillRect cover x2 - x1 + 1 pixels per row.
  void DrawLine(int x1, int y1, int x2, int y2, uint8_t color);
  void DrawRect(int x1, int y1, int x2, int y2, uint8_t color);
  void FillRect(int x1, int y1, int x2, int y2, uint8_t color);

  // Replaces every pixel in the rectangle with remap_table[pixel].
  // `remap_table` is a 256-entry table; the shorter overload covers the whole
  // view.
  void Remap(int x1, int y1, int width, int height,
             std::span<const uint8_t> remap_table);
  void Remap(std::span<const uint8_t> remap_table);

  // Draws tile `icon` of an icon set at x,y, clipped to the
  // WindowList entry `clip_window` rather than to the view - the map
  // draws its terrain through this. `remap_table` may be empty for no
  // remapping.
  void DrawStamp(std::span<const std::byte> icon_data, int icon, int x, int y,
                 std::span<const uint8_t> remap_table, int clip_window);

  // Locks the buffer's surface so its pixels can be read or written, and
  // reattaches this view to them, since locking can move them. Locks
  // nest: the surface is only really locked and unlocked by the outermost
  // pair. Lock() returns false if the view has no buffer, or if the
  // surface could not be locked, in which case the matching Unlock() must
  // not be called.
  inline bool Lock();
  inline bool Unlock();
  [[nodiscard]] inline int lock_count() const;

  // Binds the view to the given rectangle of `buffer`, clamping it
  // to the buffer's bounds. Has no effect on a PixelBuffer, which is
  // permanently the view covering itself.
  void Attach(PixelBuffer* buffer, int x, int y, int width, int height);

 protected:
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
  // The buffer this view draws into; null until Attach(). A
  // PixelBuffer points at itself.
  PixelBuffer* buffer_ = nullptr;
  // How deep the nested Lock() calls are; the surface is locked while this
  // is non-zero. Only the buffer's own count is used, so a view carries
  // this member without ever changing it.
  int lock_count_ = 0;
};

// An allocated page of 8-bit paletted pixels, and the view covering the
// whole of it. Both games keep a handful: the visible page, the hidden page
// the frame is composed on, and the staging pages movies decode into.
//
// The pixels come from one of three places, chosen by Init(): a span the
// caller owns, a new[] block the buffer owns, or - with BUFFER_VISIBLE - an SDL
// surface, whose pixels only exist between LockSurface() and
// UnlockSurface().
//
// Example:
//   PixelBuffer page(320, 200);
//   page.Clear();
class PixelBuffer : public PixelView, public BufferClass {
 public:
  // Sizes the buffer and gives it `buffer`'s pixels, or allocates `size`
  // bytes (width * height when `byte_count` is zero) if `buffer` is empty.
  PixelBuffer(int width, int height, std::span<uint8_t> buffer,
              int32_t byte_count);
  PixelBuffer(int width, int height, std::span<uint8_t> buffer = {});
  // Leaves the buffer empty; Init() gives it pixels later. Screen's pages
  // are built this way, before there is a window to size them against.
  PixelBuffer();
  // Also resets WindowBuffer if this buffer is the window's surface.
  ~PixelBuffer();

  PixelBuffer(const PixelBuffer&) = delete;
  PixelBuffer& operator=(const PixelBuffer&) = delete;
  PixelBuffer(PixelBuffer&&) = delete;
  PixelBuffer& operator=(PixelBuffer&&) = delete;

  // Gives the buffer its pixels, replacing whatever it had. With
  // BUFFER_VISIBLE it creates the window's surface and texture and records
  // itself in WindowBuffer; otherwise it takes `buffer`, or allocates
  // `byte_count` bytes when `buffer` is empty. CHECK-fails if a
  // caller-supplied
  // buffer is too small for width * height.
  void Init(int width, int height, std::span<uint8_t> buffer,
            int32_t byte_count, PixelBufferFlags flags);
  // Releases the window texture and surfaces Init() created for a visible
  // buffer, and cancels its pending redraw. The destructor calls it; calling
  // it again does nothing.
  void ReleaseSurfaces();

  // Locks and unlocks the underlying SDL surface. Callers normally use the
  // inherited PixelView::Lock/Unlock, which also reattach the
  // view to the freshly locked pixels.
  bool LockSurface();
  bool UnlockSurface();

  // Draws `bitmap` onto this buffer with its centre landing on `center`, scaled
  // and rotated. `scale` is 24.8 fixed point (0x100 = 1.0) and is ignored when
  // zero; `angle` is 0-255 over the full circle. Pixel 0 is transparent.
  // Whatever falls outside the buffer is dropped, so a bitmap that does not
  // fit is silently cropped.
  void DrawScaledRotated(const BitmapClass& bitmap, const TPoint2D& center,
                         int32_t scale, uint8_t angle);

  // Whether this is the buffer the window shows, that is whether it was
  // initialized with BUFFER_VISIBLE.
  [[nodiscard]] bool IsWindowSurface() const {
    return window_texture_ != nullptr;
  }
  // Presents the buffer's current contents. UnlockSurface() calls it with
  // `end_frame` false, which only arms a timer to redraw if nothing else
  // presents within the next frame; Video_End_Frame() passes true to present
  // immediately.
  void Present(bool end_frame);
  // Sets the 256 RGB triples the paletted pixels are shown through, and
  // redraws with them. Anything already presented changes color, the way a
  // VGA palette write did.
  void UpdatePalette(std::span<const uint8_t> palette);
  // The SDL_Palette of the display surface, as a void* so that callers need
  // no SDL header.
  [[nodiscard]] const void* palette() const;

  // Presents `frame`, `width` x `height` pixels, stretched to the
  // window by SDL rather than by the game - this is how a 320x200 movie
  // fills a 640x400 screen without the game scaling every frame itself.
  // Uses the palette already set via UpdatePalette. The frame stays on screen,
  // following later UpdatePalette() calls the way a VGA screen would, until
  // something is drawn to the display surface.
  void PresentScaledFrame(std::span<const uint8_t> frame, int width,
                          int height);
  // Drops the scaling texture, so the next present shows the display
  // surface again. UnlockSurface() calls it as soon as anything draws.
  void DropScaledFrame();

 protected:
  void CreateDisplaySurface();
  void DestroyDisplaySurface();
  // SDL types, held as void* so that this header pulls in no SDL headers.
  void* window_texture_ = nullptr;   // SDL_Texture*, the window's contents
  void* palette_surface_ = nullptr;  // SDL_Surface*, the 8-bit pixels
  int redraw_timer_ = 0;  // SDL timer id, 0 when no redraw is pending
  void* scaled_frame_texture_ =
      nullptr;  // SDL_Texture* for low-res content scaling
  int scaled_frame_width_ = 0;
  int scaled_frame_height_ = 0;

 private:
  // Converts scaled_frame_ to RGBA with the current palette and uploads it to
  // scaled_frame_texture_. Returns false if SDL refused the texture.
  bool UploadScaledFrame();

  // The paletted pixels behind scaled_frame_texture_, scaled_frame_width_ x
  // scaled_frame_height_. The texture holds baked colors, so a palette change
  // has to convert these again. Empty while there is no texture.
  std::vector<uint8_t> scaled_frame_;
};

extern PixelBuffer* WindowBuffer;

void SetScreenPalette(std::span<const uint8_t> palette);

inline int PixelView::lock_count() const { return lock_count_; }

inline bool PixelView::NeedsLock() {
  // Named for the DirectDraw surfaces this used to mean; callers read it as
  // "do the pixels have to be locked before they can be touched", which is
  // true of exactly the window's surface.
  return buffer_ != nullptr && buffer_->IsWindowSurface();
}

inline bool PixelView::Lock() {
  if (buffer_ == nullptr) {
    return false;
  }

  const bool lock = buffer_->LockSurface();
  if (!lock) {
    return false;
  }

  if (this != buffer_) {
    Attach(buffer_, x_pos_, y_pos_, width_, height_);
  }
  return true;
}

inline bool PixelView::Unlock() {
  return buffer_ == nullptr || buffer_->UnlockSurface();
}

inline uint8_t* PixelView::offset() { return offset_; }
inline std::span<uint8_t> PixelView::pixels() {
  if (buffer_ == nullptr) {
    return {};
  }
  if (this == buffer_) {
    return buffer_->Get_Bytes();
  }
  return buffer_->Get_Bytes().subspan(
      base::ToSize((y_pos_ * stride()) + x_pos_));
}

inline int PixelView::height() const { return height_; }

inline int PixelView::width() const { return width_; }

inline int PixelView::x_add() const { return x_add_; }

inline int PixelView::stride() const { return width_ + x_add_ + pitch_; }
inline int PixelView::x_pos() const { return x_pos_; }

inline int PixelView::y_pos() const { return y_pos_; }

inline PixelBuffer* PixelView::buffer() { return buffer_; }

inline void PixelView::PutPixel(int x, int y, uint8_t color) {
  if (!Lock()) {
    return;
  }

  this->PutPixelLocked(x, y, color);

  Unlock();
}

inline void PixelView::PutPixelLocked(const int x, const int y,
                                      const uint8_t color) {
  if (x >= 0 && y >= 0 && x < width() && y < height()) {
    base::At(pixels(), base::ToSize(x + (y * stride()))) = color;
  }
}

inline int PixelView::GetPixel(int x, int y) {
  int return_code = 0;

  if (Lock()) {
    return_code = Buffer_Get_Pixel(this, x, y);
    Unlock();
  }
  return return_code;
}

inline void PixelView::Clear(uint8_t color) {
  if (Lock()) {
    Buffer_Clear(this, color);
    Unlock();
  }
}

inline int32_t PixelView::CopyToBuffer(int x, int y, int width, int height,
                                       std::span<uint8_t> dest,
                                       int32_t dest_size) {
  int32_t return_code = 0;
  if (Lock()) {
    return_code = Buffer_To_Buffer(this, x, y, width, height, dest, dest_size);
    Unlock();
  }
  return return_code;
}

inline bool PixelView::Blit(PixelView& dest, int src_x, int src_y, int dst_x,
                            int dst_y, int width, int height,
                            bool transparent) {
  bool return_code = false;

  if (Lock()) {
    if (dest.Lock()) {
      return_code = Linear_Blit_To_Linear(this, &dest, src_x, src_y, dst_x,
                                          dst_y, width, height, transparent);
      dest.Unlock();
    }
    Unlock();
  }

  return return_code;
}

inline bool PixelView::Blit(PixelView& dest, int dst_x, int dst_y,
                            bool transparent) {
  return Blit(dest, 0, 0, dst_x, dst_y, width_, height_, transparent);
}

inline bool PixelView::Blit(PixelView& dest, bool transparent) {
  return Blit(dest, 0, 0, transparent);
}

inline bool PixelView::Scale(PixelView& dest, int src_x, int src_y, int dst_x,
                             int dst_y, int src_width, int src_height,
                             int dst_width, int dst_height, bool transparent,
                             std::span<const uint8_t> remap_table) {
  bool return_code = false;
  if (Lock()) {
    if (dest.Lock()) {
      return_code = Linear_Scale_To_Linear(
          this, &dest, src_x, src_y, dst_x, dst_y, src_width, src_height,
          dst_width, dst_height, transparent, remap_table);
      dest.Unlock();
    }
    Unlock();
  }
  return return_code;
}

inline bool PixelView::Scale(PixelView& dest, int src_x, int src_y, int dst_x,
                             int dst_y, int src_width, int src_height,
                             int dst_width, int dst_height,
                             std::span<const uint8_t> remap_table) {
  return Scale(dest, src_x, src_y, dst_x, dst_y, src_width, src_height,
               dst_width, dst_height, false, remap_table);
}

inline bool PixelView::Scale(PixelView& dest, bool transparent,
                             std::span<const uint8_t> remap_table) {
  return Scale(dest, 0, 0, 0, 0, width_, height_, dest.width(), dest.height(),
               transparent, remap_table);
}

inline bool PixelView::Scale(PixelView& dest,
                             std::span<const uint8_t> remap_table) {
  return Scale(dest, false, remap_table);
}

inline void PixelView::Print(const char* text, int x, int y, int fore_color,
                             int back_color) {
  if (!Lock()) {
    return;
  }
  Buffer_Print(this, text, x, y, fore_color, back_color);
  Unlock();
}

inline void PixelView::Print(int value, int x, int y, int fore_color,
                             int back_color) {
  Print(absl::StrCat(value).c_str(), x, y, fore_color, back_color);
}

inline void PixelView::DrawStamp(std::span<const std::byte> icon_data, int icon,
                                 int x, int y,
                                 const std::span<const uint8_t> remap_table,
                                 int clip_window) {
  // Tiberian Dawn stores a window's x and width in units of eight pixels;
  // Red Alert stores them in pixels.
#ifdef TD
  constexpr int kWindowUnit = 8;
#else
  constexpr int kWindowUnit = 1;
#endif
  if (Lock()) {
    Buffer_Draw_Stamp_Clip(
        this, icon_data, icon, x, y, remap_table,
        base::At(base::At(WindowList, clip_window), kWindowX) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowHeight));
    Unlock();
  }
}

inline void PixelView::DrawLine(int x1, int y1, int x2, int y2, uint8_t color) {
  if (Lock()) {
    Buffer_Draw_Line(this, x1, y1, x2, y2, color);
    Unlock();
  }
}

inline void PixelView::FillRect(int x1, int y1, int x2, int y2, uint8_t color) {
  if (Lock()) {
    Buffer_Fill_Rect(this, x1, y1, x2, y2, color);
    Unlock();
  }
}

inline void PixelView::Remap(int x1, int y1, int width, int height,
                             std::span<const uint8_t> remap_table) {
  if (Lock()) {
    Buffer_Remap(this, x1, y1, width, height, remap_table);
    Unlock();
  }
}

inline void PixelView::Remap(std::span<const uint8_t> remap_table) {
  Remap(0, 0, width_, height_, remap_table);
}

inline int PixelView::pitch() const { return pitch_; }
// BufferClass's copies to a page live here rather than in buffer.h because
// they need the complete PixelView.

inline int32_t Buffer_To_Page(int x, int y, int width, int height,
                              std::span<const uint8_t> Buffer,
                              PixelView& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, width, height, Buffer, &view);
    view.Unlock();
  }
  return return_code;
}

inline int32_t BufferClass::To_Page(int width, int height, PixelView& view) {
  return To_Page(0, 0, width, height, view);
}
inline int32_t BufferClass::To_Page(PixelView& view) {
  return To_Page(0, 0, view.width(), view.height(), view);
}
inline int32_t BufferClass::To_Page(int x, int y, int width, int height,
                                    PixelView& view) {
  return Buffer_To_Page(x, y, width, height, Get_Bytes(), view);
}

#endif  // CNC_RED_ALERT_SDLLIB_GRAPHIC_BUFFER_H_
