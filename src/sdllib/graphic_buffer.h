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
  GBC_NONE = 0,
  // Kept for the call sites the DOS and DirectDraw versions used to
  // distinguish; SDL gives no say in where a surface lives, so Init()
  // ignores it.
  GBC_VIDEOMEM = 1,
  // The buffer is the one the window shows. Init() creates an SDL surface
  // and texture for it instead of allocating memory, and records it in
  // WindowBuffer.
  GBC_VISIBLE = 2,
};
using enum PixelBufferFlags;
template <>
inline constexpr bool base::kIsFlagEnum<PixelBufferFlags> = true;

// The VGA mode the games were written for. Both still decode their
// low-resolution movies at this size, whatever video mode is set.
inline constexpr int kDefaultScreenWidth = 320;
inline constexpr int kDefaultScreenHeight = 200;

class PixelBuffer;

// Makes `ptr` the page the drawing code writes to, and returns the previous
// one so the caller can put it back.
PixelView* SetLogicPage(PixelView* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);
PixelView* SetLogicPage(PixelView& ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);

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
  // Attaches the view to `graphic_buff` at x,y with the given size; see
  // Attach() for how the rectangle is clamped to the buffer.
  PixelView(PixelBuffer* graphic_buff, int x, int y, int w, int h);
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
  // clipped to the view first, and nothing is written if `buff` is
  // smaller than what is left.
  int32_t CopyToBuffer(int x, int y, int w, int h, std::span<uint8_t> buff,
                       int32_t size);

  // Copies pixel_width x pixel_height pixels from x_pixel,y_pixel in this
  // view to dx_pixel,dy_pixel in `dest`, clipping to both. With `trans`,
  // pixel 0 is left alone in the destination instead of being copied.
  // Returns false if either view could not be locked.
  bool Blit(PixelView& dest, int x_pixel, int y_pixel, int dx_pixel,
            int dy_pixel, int pixel_width, int pixel_height,
            bool trans = false);
  bool Blit(PixelView& dest, int dx, int dy, bool trans = false);
  bool Blit(PixelView& dest, bool trans = false);

  bool Scale(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
             int src_w, int src_h, int dst_w, int dst_h, bool trans = false,
             std::span<const uint8_t> remap = {});
  bool Scale(PixelView& dest, int src_x, int src_y, int dst_x, int dst_y,
             int src_w, int src_h, int dst_w, int dst_h,
             std::span<const uint8_t> remap);
  bool Scale(PixelView& dest, bool trans = false,
             std::span<const uint8_t> remap = {});
  bool Scale(PixelView& dest, std::span<const uint8_t> remap);

  // Draws text in the current font at x_pixel,y_pixel. `fcolor` and `bcolor`
  // are palette indices; the integer overload prints the number in decimal.
  void Print(const char* string, int x_pixel, int y_pixel, int fcolor,
             int bcolor);
  void Print(int num, int x_pixel, int y_pixel, int fcolor, int bcolor);

  // sx,sy and dx,dy are the two corners, both inclusive, so DrawRect and
  // FillRect cover dx - sx + 1 pixels per row.
  void DrawLine(int sx, int sy, int dx, int dy, uint8_t color);
  void DrawRect(int sx, int sy, int dx, int dy, uint8_t color);
  void FillRect(int sx, int sy, int dx, int dy, uint8_t color);

  // Replaces every pixel in the rectangle with remap[pixel]. `remap` is a
  // 256-entry table; the shorter overload covers the whole view.
  void Remap(int sx, int sy, int width, int height,
             std::span<const uint8_t> remap);
  void Remap(std::span<const uint8_t> remap);

  // Draws tile `icon` of an icon set at x_pixel,y_pixel, clipped to the
  // WindowList entry `clip_window` rather than to the view - the map
  // draws its terrain through this. `remap` may be empty for no remapping.
  void DrawStamp(std::span<const std::byte> icondata, int icon, int x_pixel,
                 int y_pixel, std::span<const uint8_t> remap, int clip_window);

  // Locks the buffer's surface so its pixels can be read or written, and
  // reattaches this view to them, since locking can move them. Locks
  // nest: the surface is only really locked and unlocked by the outermost
  // pair. Lock() returns false if the view has no buffer, or if the
  // surface could not be locked, in which case the matching Unlock() must
  // not be called.
  inline bool Lock();
  inline bool Unlock();
  [[nodiscard]] inline int lock_count() const;

  // Binds the view to the given rectangle of `graphic_buff`, clamping it
  // to the buffer's bounds. Has no effect on a PixelBuffer, which is
  // permanently the view covering itself.
  void Attach(PixelBuffer* graphic_buff, int x, int y, int w, int h);

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
// caller owns, a new[] block the buffer owns, or - with GBC_VISIBLE - an SDL
// surface, whose pixels only exist between LockSurface() and
// UnlockSurface().
//
// Example:
//   PixelBuffer page(320, 200);
//   page.Clear();
class PixelBuffer : public PixelView, public BufferClass {
 public:
  // Sizes the buffer and gives it `buffer`'s pixels, or allocates `size`
  // bytes (w * h when `size` is zero) if `buffer` is empty.
  PixelBuffer(int w, int h, std::span<uint8_t> buffer, int32_t size);
  PixelBuffer(int w, int h, std::span<uint8_t> buffer = {});
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
  // GBC_VISIBLE it creates the window's surface and texture and records
  // itself in WindowBuffer; otherwise it takes `buffer`, or allocates
  // `size` bytes when `buffer` is empty. CHECK-fails if a caller-supplied
  // buffer is too small for w * h.
  void Init(int w, int h, std::span<uint8_t> buffer, int32_t size,
            PixelBufferFlags flags);
  // Releases the window texture and surfaces Init() created for a visible
  // buffer, and cancels its pending redraw. The destructor calls it; calling
  // it again does nothing.
  void ReleaseSurfaces();

  // Locks and unlocks the underlying SDL surface. Callers normally use the
  // inherited PixelView::Lock/Unlock, which also reattach the
  // view to the freshly locked pixels.
  bool LockSurface();
  bool UnlockSurface();

  // Draws `bmp` onto this buffer with its centre landing on `pt`, scaled and
  // rotated. `scale` is 24.8 fixed point (0x100 = 1.0) and is ignored when
  // zero; `angle` is 0-255 over the full circle. Pixel 0 is transparent.
  // Whatever falls outside the buffer is dropped, so a bitmap that does not
  // fit is silently cropped.
  void DrawScaledRotated(const BitmapClass& bmp, const TPoint2D& pt,
                         int32_t scale, uint8_t angle);

  // Whether this is the buffer the window shows, that is whether it was
  // initialized with GBC_VISIBLE.
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

  // Presents `paletted_data`, `width` x `height` pixels, stretched to the
  // window by SDL rather than by the game - this is how a 320x200 movie
  // fills a 640x400 screen without the game scaling every frame itself.
  // Uses the palette already set via UpdatePalette. The frame stays on screen,
  // following later UpdatePalette() calls the way a VGA screen would, until
  // something is drawn to the display surface.
  void PresentScaledFrame(std::span<const uint8_t> paletted_data, int width,
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

inline int32_t PixelView::CopyToBuffer(int x, int y, int w, int h,
                                       std::span<uint8_t> buff, int32_t size) {
  int32_t return_code = 0;
  if (Lock()) {
    return_code = Buffer_To_Buffer(this, x, y, w, h, buff, size);
    Unlock();
  }
  return return_code;
}

inline bool PixelView::Blit(PixelView& dest, int x_pixel, int y_pixel,
                            int dx_pixel, int dy_pixel, int pixel_width,
                            int pixel_height, bool trans) {
  bool return_code = false;

  if (Lock()) {
    if (dest.Lock()) {
      return_code =
          Linear_Blit_To_Linear(this, &dest, x_pixel, y_pixel, dx_pixel,
                                dy_pixel, pixel_width, pixel_height, trans);
      dest.Unlock();
    }
    Unlock();
  }

  return return_code;
}

inline bool PixelView::Blit(PixelView& dest, int dx, int dy, bool trans) {
  return Blit(dest, 0, 0, dx, dy, width_, height_, trans);
}

inline bool PixelView::Blit(PixelView& dest, bool trans) {
  return Blit(dest, 0, 0, trans);
}

inline bool PixelView::Scale(PixelView& dest, int src_x, int src_y, int dst_x,
                             int dst_y, int src_w, int src_h, int dst_w,
                             int dst_h, bool trans,
                             std::span<const uint8_t> remap) {
  bool return_code = false;
  if (Lock()) {
    if (dest.Lock()) {
      return_code =
          Linear_Scale_To_Linear(this, &dest, src_x, src_y, dst_x, dst_y, src_w,
                                 src_h, dst_w, dst_h, trans, remap);
      dest.Unlock();
    }
    Unlock();
  }
  return return_code;
}

inline bool PixelView::Scale(PixelView& dest, int src_x, int src_y, int dst_x,
                             int dst_y, int src_w, int src_h, int dst_w,
                             int dst_h, std::span<const uint8_t> remap) {
  return Scale(dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h,
               false, remap);
}

inline bool PixelView::Scale(PixelView& dest, bool trans,
                             std::span<const uint8_t> remap) {
  return Scale(dest, 0, 0, 0, 0, width_, height_, dest.width(), dest.height(),
               trans, remap);
}

inline bool PixelView::Scale(PixelView& dest, std::span<const uint8_t> remap) {
  return Scale(dest, false, remap);
}

inline void PixelView::Print(const char* string, int x_pixel, int y_pixel,
                             int fcolor, int bcolor) {
  if (!Lock()) {
    return;
  }
  Buffer_Print(this, string, x_pixel, y_pixel, fcolor, bcolor);
  Unlock();
}

inline void PixelView::Print(int num, int x_pixel, int y_pixel, int fcolor,
                             int bcolor) {
  Print(absl::StrCat(num).c_str(), x_pixel, y_pixel, fcolor, bcolor);
}

inline void PixelView::DrawStamp(std::span<const std::byte> icondata, int icon,
                                 int x_pixel, int y_pixel,
                                 const std::span<const uint8_t> remap,
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
        this, icondata, icon, x_pixel, y_pixel, remap,
        base::At(base::At(WindowList, clip_window), kWindowX) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth) * kWindowUnit,
        base::At(base::At(WindowList, clip_window), kWindowHeight));
    Unlock();
  }
}

inline void PixelView::DrawLine(int sx, int sy, int dx, int dy, uint8_t color) {
  if (Lock()) {
    Buffer_Draw_Line(this, sx, sy, dx, dy, color);
    Unlock();
  }
}

inline void PixelView::FillRect(int sx, int sy, int dx, int dy, uint8_t color) {
  if (Lock()) {
    Buffer_Fill_Rect(this, sx, sy, dx, dy, color);
    Unlock();
  }
}

inline void PixelView::Remap(int sx, int sy, int width, int height,
                             std::span<const uint8_t> remap) {
  if (Lock()) {
    Buffer_Remap(this, sx, sy, width, height, remap);
    Unlock();
  }
}

inline void PixelView::Remap(std::span<const uint8_t> remap) {
  Remap(0, 0, width_, height_, remap);
}

inline int PixelView::pitch() const { return pitch_; }
// BufferClass's copies to a page live here rather than in buffer.h because
// they need the complete PixelView.

inline int32_t Buffer_To_Page(int x, int y, int w, int h,
                              std::span<const uint8_t> Buffer,
                              PixelView& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, w, h, Buffer, &view);
    view.Unlock();
  }
  return return_code;
}

inline int32_t BufferClass::To_Page(int w, int h, PixelView& view) {
  return To_Page(0, 0, w, h, view);
}
inline int32_t BufferClass::To_Page(PixelView& view) {
  return To_Page(0, 0, view.width(), view.height(), view);
}
inline int32_t BufferClass::To_Page(int x, int y, int w, int h,
                                    PixelView& view) {
  return Buffer_To_Page(x, y, w, h, Get_Bytes(), view);
}

#endif  // CNC_RED_ALERT_SDLLIB_GRAPHIC_BUFFER_H_
