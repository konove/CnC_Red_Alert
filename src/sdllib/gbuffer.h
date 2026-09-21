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

// File: The graphic buffer and the viewports that draw into it.
//
// A GraphicBufferClass owns a rectangle of 8-bit paletted pixels - either
// plain memory or, for the one visible buffer, an SDL surface that is locked
// while anything draws to it. A GraphicViewPortClass is a rectangular window
// onto such a buffer; clipping and coordinates are relative to the window, so
// the drawing primitives need to know nothing about the page behind it. A
// GraphicBufferClass is also a viewport onto itself, covering the whole page,
// which is why it derives from GraphicViewPortClass.
//
// The primitives themselves live in drawbuff.h as free Buffer_* functions.
// The inline members here exist to lock the surface around a call to one of
// them and to supply the defaults ("the whole viewport") for the shorter
// overloads.

#ifndef CNC_RED_ALERT_SDLLIB_GBUFFER_H_
#define CNC_RED_ALERT_SDLLIB_GBUFFER_H_

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

// How GraphicBufferClass::Init() should back the buffer.
enum class CNC_FLAG_ENUM GBC_Enum {
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
using enum GBC_Enum;
template <>
inline constexpr bool base::kIsFlagEnum<GBC_Enum> = true;

// The VGA mode the games were written for. Both still decode their
// low-resolution movies at this size, whatever video mode is set.
inline constexpr int kDefaultScreenWidth = 320;
constexpr int DEFAULT_SCREEN_HEIGHT = 200;

class GraphicBufferClass;

// Makes `ptr` the page the drawing code writes to, and returns the previous
// one so the caller can put it back.
GraphicViewPortClass* Set_Logic_Page(
    GraphicViewPortClass* ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);
GraphicViewPortClass* Set_Logic_Page(
    GraphicViewPortClass& ptr ABSL_ATTRIBUTE_LIFETIME_BOUND);

// A rectangular window onto a GraphicBufferClass. Coordinates passed to the
// drawing members are relative to the window's top left corner and are
// clipped to it, so the same primitives serve the full page, the map area and
// a dialog box.
//
// A viewport is not usable until Attach() (or the four-argument constructor)
// binds it to a buffer; a default-constructed one has no buffer. It does not
// own the pixels and must not outlive the buffer it is attached to.
//
// Example:
//   GraphicViewPortClass view(&page, 0, 0, 320, 200);
//   view.Clear();
//   view.Blit(TheScreen().visible_view());
class GraphicViewPortClass {
 public:
  // Attaches the viewport to `graphic_buff` at x,y with the given size; see
  // Attach() for how the rectangle is clamped to the buffer.
  GraphicViewPortClass(GraphicBufferClass* graphic_buff, int x, int y, int w,
                       int h);
  GraphicViewPortClass() = default;
  // Resets LogicPage if it points here, so that nothing draws into a
  // viewport that is gone.
  ~GraphicViewPortClass();
  GraphicViewPortClass(const GraphicViewPortClass&) = delete;
  GraphicViewPortClass& operator=(const GraphicViewPortClass&) = delete;
  GraphicViewPortClass(GraphicViewPortClass&&) = delete;
  GraphicViewPortClass& operator=(GraphicViewPortClass&&) = delete;

  // A raw pointer to the viewport's top left pixel, valid only while the
  // buffer is locked. Prefer Get_Pixels().
  std::uint8_t* Get_Offset();
  // The buffer's pixels from this viewport's top left corner to the end of
  // the buffer. Rows are Get_Width() + Get_XAdd() + Get_Pitch() bytes apart.
  // Empty if the viewport is not attached to a buffer.
  std::span<uint8_t> Get_Pixels();
  [[nodiscard]] int Get_Height() const;
  [[nodiscard]] int Get_Width() const;
  [[nodiscard]] int Get_XAdd() const;
  [[nodiscard]] int Get_XPos() const;
  [[nodiscard]] int Get_YPos() const;
  [[nodiscard]] int Get_Pitch() const;
  // Whether drawing to this viewport has to lock a surface first.
  inline bool Get_IsDirectDraw();
  GraphicBufferClass* Get_Graphic_Buffer();

  bool Change(int x, int y, int w, int h);

  // The drawing primitives. Each locks the buffer, calls the matching
  // Buffer_* function from drawbuff.h, and unlocks it; the short overloads
  // fill in "the whole viewport" for the missing rectangle.

  // Sets one pixel, ignoring coordinates outside the viewport.
  // Buffer_Put_Pixel is the same without the lock, for callers that hold one
  // already.
  void Put_Pixel(int x, int y, unsigned char color);
  void Buffer_Put_Pixel(int x, int y, unsigned char color);
  // Returns the palette index at x,y, or 0 outside the viewport.
  int Get_Pixel(int x, int y);
  void Clear(unsigned char color = 0);

  // Copies a rectangle of the viewport out to plain memory, packed with no
  // padding, and returns the number of bytes written. The rectangle is
  // clipped to the viewport first, and nothing is written if `buff` is
  // smaller than what is left.
  int32_t To_Buffer(int x, int y, int w, int h, std::span<uint8_t> buff,
                    int32_t size);
  int32_t To_Buffer(int x, int y, int w, int h, BufferClass* buff);
  int32_t To_Buffer(BufferClass* buff);

  // Copies pixel_width x pixel_height pixels from x_pixel,y_pixel in this
  // viewport to dx_pixel,dy_pixel in `dest`, clipping to both. With `trans`,
  // pixel 0 is left alone in the destination instead of being copied.
  // Returns false if either viewport could not be locked.
  bool Blit(GraphicViewPortClass& dest, int x_pixel, int y_pixel, int dx_pixel,
            int dy_pixel, int pixel_width, int pixel_height,
            bool trans = false);
  bool Blit(GraphicViewPortClass& dest, int dx, int dy, bool trans = false);
  bool Blit(GraphicViewPortClass& dest, bool trans = false);

  bool Scale(GraphicViewPortClass& dest, int src_x, int src_y, int dst_x,
             int dst_y, int src_w, int src_h, int dst_w, int dst_h,
             bool trans = false, std::span<const uint8_t> remap = {});
  bool Scale(GraphicViewPortClass& dest, int src_x, int src_y, int dst_x,
             int dst_y, int src_w, int src_h, int dst_w, int dst_h,
             std::span<const uint8_t> remap);
  bool Scale(GraphicViewPortClass& dest, bool trans = false,
             std::span<const uint8_t> remap = {});
  bool Scale(GraphicViewPortClass& dest, std::span<const uint8_t> remap);

  // Draws text in the current font at x_pixel,y_pixel. `fcolor` and `bcolor`
  // are palette indices; the integer overload prints the number in decimal.
  void Print(const char* string, int x_pixel, int y_pixel, int fcolor,
             int bcolor);
  void Print(int num, int x_pixel, int y_pixel, int fcolor, int bcolor);

  // sx,sy and dx,dy are the two corners, both inclusive, so Draw_Rect and
  // Fill_Rect cover dx - sx + 1 pixels per row.
  void Draw_Line(int sx, int sy, int dx, int dy, unsigned char color);
  void Draw_Rect(int sx, int sy, int dx, int dy, unsigned char color);
  void Fill_Rect(int sx, int sy, int dx, int dy, unsigned char color);

  // Replaces every pixel in the rectangle with remap[pixel]. `remap` is a
  // 256-entry table; the shorter overload covers the whole viewport.
  void Remap(int sx, int sy, int width, int height,
             std::span<const uint8_t> remap);
  void Remap(std::span<const uint8_t> remap);

  // Draws tile `icon` of an icon set at x_pixel,y_pixel, clipped to the
  // WindowList entry `clip_window` rather than to the viewport - the map
  // draws its terrain through this. `remap` may be empty for no remapping.
  void Draw_Stamp(std::span<const std::byte> icondata, int icon, int x_pixel,
                  int y_pixel, std::span<const uint8_t> remap, int clip_window);

  // Locks the buffer's surface so its pixels can be read or written, and
  // reattaches this viewport to them, since locking can move them. Locks
  // nest: the surface is only really locked and unlocked by the outermost
  // pair. Lock() returns false if the viewport has no buffer, or if the
  // surface could not be locked, in which case the matching Unlock() must
  // not be called.
  inline bool Lock();
  inline bool Unlock();
  [[nodiscard]] inline int Get_LockCount() const;

  // Binds the viewport to the given rectangle of `graphic_buff`, clamping it
  // to the buffer's bounds. Has no effect on a GraphicBufferClass, which is
  // permanently the viewport covering itself.
  void Attach(GraphicBufferClass* graphic_buff, int x, int y, int w, int h);

 protected:
  // The viewport's top left pixel within the buffer. Null while the buffer
  // is a surface that is not currently locked.
  std::uint8_t* Offset = nullptr;
  int Width = 0;
  int Height = 0;
  // The bytes of the buffer's row that fall outside the viewport, that is
  // the buffer's width minus Width. Together with Pitch it turns the end of
  // one row of the viewport into the start of the next.
  int XAdd = 0;
  // Where the viewport sits in the buffer.
  int XPos = 0;
  int YPos = 0;
  // Padding the buffer keeps past the end of every row, beyond XAdd. Copied
  // from the buffer, and zero for every buffer the games create.
  int32_t Pitch = 0;
  // The buffer this viewport draws into; null until Attach(). A
  // GraphicBufferClass points at itself.
  GraphicBufferClass* GraphicBuff = nullptr;
  // How deep the nested Lock() calls are; the surface is locked while this
  // is non-zero. Only the buffer's own count is used, so a viewport carries
  // this member without ever changing it.
  int LockCount = 0;
};

// An allocated page of 8-bit paletted pixels, and the viewport covering the
// whole of it. Both games keep a handful: the visible page, the hidden page
// the frame is composed on, and the staging pages movies decode into.
//
// The pixels come from one of three places, chosen by Init(): a span the
// caller owns, a new[] block the buffer owns, or - with GBC_VISIBLE - an SDL
// surface, whose pixels only exist between Lock_Surface() and
// Unlock_Surface().
//
// Example:
//   GraphicBufferClass page(320, 200);
//   page.Clear();
class GraphicBufferClass : public GraphicViewPortClass, public BufferClass {
 public:
  // Sizes the buffer and gives it `buffer`'s pixels, or allocates `size`
  // bytes (w * h when `size` is zero) if `buffer` is empty.
  GraphicBufferClass(int w, int h, std::span<uint8_t> buffer, int32_t size);
  GraphicBufferClass(int w, int h, std::span<uint8_t> buffer = {});
  // Leaves the buffer empty; Init() gives it pixels later. Screen's pages
  // are built this way, before there is a window to size them against.
  GraphicBufferClass();
  // Also resets WindowBuffer if this buffer is the window's surface.
  ~GraphicBufferClass();

  GraphicBufferClass(const GraphicBufferClass&) = delete;
  GraphicBufferClass& operator=(const GraphicBufferClass&) = delete;
  GraphicBufferClass(GraphicBufferClass&&) = delete;
  GraphicBufferClass& operator=(GraphicBufferClass&&) = delete;

  // Gives the buffer its pixels, replacing whatever it had. With
  // GBC_VISIBLE it creates the window's surface and texture and records
  // itself in WindowBuffer; otherwise it takes `buffer`, or allocates
  // `size` bytes when `buffer` is empty. CHECK-fails if a caller-supplied
  // buffer is too small for w * h.
  void Init(int w, int h, std::span<uint8_t> buffer, int32_t size,
            GBC_Enum flags);
  // Releases the window texture and surfaces Init() created for a visible
  // buffer, and cancels its pending redraw. The destructor calls it; calling
  // it again does nothing.
  void Un_Init();

  // Locks and unlocks the underlying SDL surface. Callers normally use the
  // inherited GraphicViewPortClass::Lock/Unlock, which also reattach the
  // viewport to the freshly locked pixels.
  bool Lock_Surface();
  bool Unlock_Surface();

  // Draws `bmp` onto this buffer with its centre landing on `pt`, scaled and
  // rotated. `scale` is 24.8 fixed point (0x100 = 1.0) and is ignored when
  // zero; `angle` is 0-255 over the full circle. Pixel 0 is transparent.
  // Whatever falls outside the buffer is dropped, so a bitmap that does not
  // fit is silently cropped.
  void Scale_Rotate(const BitmapClass& bmp, const TPoint2D& pt, int32_t scale,
                    uint8_t angle);

  // Whether this is the buffer the window shows, that is whether it was
  // initialized with GBC_VISIBLE.
  [[nodiscard]] bool Is_Window_Surface() const {
    return WindowTexture != nullptr;
  }
  // Presents the buffer's current contents. Unlock_Surface() calls it with
  // `end_frame` false, which only arms a timer to redraw if nothing else
  // presents within the next frame; Video_End_Frame() passes true to present
  // immediately.
  void Update_Window_Surface(bool end_frame);
  // Sets the 256 RGB triples the paletted pixels are shown through, and
  // redraws with them. Anything already presented changes color, the way a
  // VGA palette write did.
  void Update_Palette(std::span<const uint8_t> palette);
  // The SDL_Palette of the display surface, as a void* so that callers need
  // no SDL header.
  [[nodiscard]] const void* Get_Palette() const;

  // Presents `paletted_data`, `width` x `height` pixels, stretched to the
  // window by SDL rather than by the game - this is how a 320x200 movie
  // fills a 640x400 screen without the game scaling every frame itself.
  // Uses the palette already set via Update_Palette. The frame stays on screen,
  // following later Update_Palette() calls the way a VGA screen would, until
  // something is drawn to the display surface.
  void Render_Scaled_Frame(std::span<const uint8_t> paletted_data, int width,
                           int height);
  // Drops the scaling texture, so the next present shows the display
  // surface again. Unlock_Surface() calls it as soon as anything draws.
  void Destroy_VQA_Texture();

 protected:
  void Init_Display_Surface();
  void Release_Display_Surface();
  // SDL types, held as void* so that this header pulls in no SDL headers.
  void* WindowTexture = nullptr;   // SDL_Texture*, the window's contents
  void* PaletteSurface = nullptr;  // SDL_Surface*, the 8-bit pixels
  int RedrawTimer = 0;             // SDL timer id, 0 when no redraw is pending
  void* VQATexture = nullptr;  // SDL_Texture* for low-res content scaling
  int VQATextureWidth = 0;
  int VQATextureHeight = 0;

 private:
  // Converts scaled_frame_ to RGBA with the current palette and uploads it to
  // VQATexture. Returns false if SDL refused the texture.
  bool Upload_Scaled_Frame();

  // The paletted pixels behind VQATexture, VQATextureWidth x VQATextureHeight.
  // The texture holds baked colors, so a palette change has to convert these
  // again. Empty while there is no texture.
  std::vector<uint8_t> scaled_frame_;
};

extern GraphicBufferClass* WindowBuffer;

void Do_Set_Palette(std::span<const uint8_t> palette);

inline int GraphicViewPortClass::Get_LockCount() const { return LockCount; }

inline bool GraphicViewPortClass::Get_IsDirectDraw() {
  // Named for the DirectDraw surfaces this used to mean; callers read it as
  // "do the pixels have to be locked before they can be touched", which is
  // true of exactly the window's surface.
  return GraphicBuff != nullptr && GraphicBuff->Is_Window_Surface();
}

inline bool GraphicViewPortClass::Lock() {
  if (GraphicBuff == nullptr) {
    return false;
  }

  const bool lock = GraphicBuff->Lock_Surface();
  if (!lock) {
    return false;
  }

  if (this != GraphicBuff) {
    Attach(GraphicBuff, XPos, YPos, Width, Height);
  }
  return true;
}

inline bool GraphicViewPortClass::Unlock() {
  return GraphicBuff == nullptr || GraphicBuff->Unlock_Surface();
}

inline std::uint8_t* GraphicViewPortClass::Get_Offset() { return Offset; }
inline std::span<uint8_t> GraphicViewPortClass::Get_Pixels() {
  if (GraphicBuff == nullptr) {
    return {};
  }
  const auto pixels = GraphicBuff->Get_Bytes();
  if (this == GraphicBuff) {
    return GraphicBuff->Get_Bytes();
  }
  // Width + XAdd is the buffer's width by construction in Attach(), so this
  // is the usual row-stride times y plus x.
  return pixels.subspan(base::ToSize((YPos * (Width + XAdd + Pitch)) + XPos));
}

inline int GraphicViewPortClass::Get_Height() const { return Height; }

inline int GraphicViewPortClass::Get_Width() const { return Width; }

inline int GraphicViewPortClass::Get_XAdd() const { return XAdd; }
inline int GraphicViewPortClass::Get_XPos() const { return XPos; }

inline int GraphicViewPortClass::Get_YPos() const { return YPos; }

inline GraphicBufferClass* GraphicViewPortClass::Get_Graphic_Buffer() {
  return GraphicBuff;
}

inline void GraphicViewPortClass::Put_Pixel(int x, int y, unsigned char color) {
  if (!Lock()) {
    return;
  }

  this->Buffer_Put_Pixel(x, y, color);

  Unlock();
}

inline void GraphicViewPortClass::Buffer_Put_Pixel(const int x, const int y,
                                                   const unsigned char color) {
  if (x >= 0 && y >= 0 && x < Get_Width() && y < Get_Height()) {
    const base::ssize pitch = Get_XAdd() + Get_Width() + Get_Pitch();
    base::At(Get_Pixels(), base::ToSize(x + (y * pitch))) = color;
  }
}

inline int GraphicViewPortClass::Get_Pixel(int x, int y) {
  int return_code = 0;

  if (Lock()) {
    return_code = Buffer_Get_Pixel(this, x, y);
    Unlock();
  }
  return return_code;
}

inline void GraphicViewPortClass::Clear(unsigned char color) {
  if (Lock()) {
    Buffer_Clear(this, color);
    Unlock();
  }
}

inline int32_t GraphicViewPortClass::To_Buffer(int x, int y, int w, int h,
                                               std::span<uint8_t> buff,
                                               int32_t size) {
  int32_t return_code = 0;
  if (Lock()) {
    return_code = Buffer_To_Buffer(this, x, y, w, h, buff, size);
    Unlock();
  }
  return return_code;
}

inline int32_t GraphicViewPortClass::To_Buffer(int x, int y, int w, int h,
                                               BufferClass* buff) {
  return To_Buffer(x, y, w, h, buff->Get_Bytes(), buff->Get_Size());
}

inline int32_t GraphicViewPortClass::To_Buffer(BufferClass* buff) {
  return To_Buffer(0, 0, Width, Height, buff->Get_Bytes(), buff->Get_Size());
}

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, int x_pixel,
                                       int y_pixel, int dx_pixel, int dy_pixel,
                                       int pixel_width, int pixel_height,
                                       bool trans) {
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

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, int dx,
                                       int dy, bool trans) {
  return Blit(dest, 0, 0, dx, dy, Width, Height, trans);
}

inline bool GraphicViewPortClass::Blit(GraphicViewPortClass& dest, bool trans) {
  return Blit(dest, 0, 0, 0, 0, Width, Height, trans);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, int src_x,
                                        int src_y, int dst_x, int dst_y,
                                        int src_w, int src_h, int dst_w,
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

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, int src_x,
                                        int src_y, int dst_x, int dst_y,
                                        int src_w, int src_h, int dst_w,
                                        int dst_h,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h,
               false, remap);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest, bool trans,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(),
               dest.Get_Height(), trans, remap);
}

inline bool GraphicViewPortClass::Scale(GraphicViewPortClass& dest,
                                        std::span<const uint8_t> remap) {
  return Scale(dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(),
               dest.Get_Height(), false, remap);
}

inline void GraphicViewPortClass::Print(const char* string, int x_pixel,
                                        int y_pixel, int fcolor, int bcolor) {
  if (!Lock()) {
    return;
  }
  Buffer_Print(this, string, x_pixel, y_pixel, fcolor, bcolor);
  Unlock();
}

inline void GraphicViewPortClass::Print(int num, int x_pixel, int y_pixel,
                                        int fcolor, int bcolor) {
  Print(absl::StrCat(num).c_str(), x_pixel, y_pixel, fcolor, bcolor);
}

inline void GraphicViewPortClass::Draw_Stamp(
    std::span<const std::byte> icondata, int icon, int x_pixel, int y_pixel,
    const std::span<const uint8_t> remap, int clip_window) {
  if (Lock()) {
    // Tiberian Dawn stores a window's x and width in units of eight pixels;
    // Red Alert stores them in pixels.
#ifdef TD
    Buffer_Draw_Stamp_Clip(
        this, icondata, icon, x_pixel, y_pixel, remap,
        base::At(base::At(WindowList, clip_window), kWindowX) * 8,
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth) * 8,
        base::At(base::At(WindowList, clip_window), kWindowHeight));
#else
    Buffer_Draw_Stamp_Clip(
        this, icondata, icon, x_pixel, y_pixel, remap,
        base::At(base::At(WindowList, clip_window), kWindowX),
        base::At(base::At(WindowList, clip_window), kWindowY),
        base::At(base::At(WindowList, clip_window), kWindowWidth),
        base::At(base::At(WindowList, clip_window), kWindowHeight));
#endif
    Unlock();
  }
}

inline void GraphicViewPortClass::Draw_Line(int sx, int sy, int dx, int dy,
                                            unsigned char color) {
  if (Lock()) {
    Buffer_Draw_Line(this, sx, sy, dx, dy, color);
    Unlock();
  }
}

inline void GraphicViewPortClass::Fill_Rect(int sx, int sy, int dx, int dy,
                                            unsigned char color) {
  if (Lock()) {
    Buffer_Fill_Rect(this, sx, sy, dx, dy, color);
    Unlock();
  }
}

inline void GraphicViewPortClass::Remap(int sx, int sy, int width, int height,
                                        std::span<const uint8_t> remap) {
  if (Lock()) {
    Buffer_Remap(this, sx, sy, width, height, remap);
    Unlock();
  }
}

inline void GraphicViewPortClass::Remap(std::span<const uint8_t> remap) {
  Remap(0, 0, Width, Height, remap);
}

inline int GraphicViewPortClass::Get_Pitch() const {
  return static_cast<int>(Pitch);
}
// BufferClass's copies to a page live here rather than in buffer.h because
// they need the complete GraphicViewPortClass.

inline int32_t Buffer_To_Page(int x, int y, int w, int h,
                              std::span<const uint8_t> Buffer,
                              GraphicViewPortClass& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, w, h, Buffer, &view);
    view.Unlock();
  }
  return return_code;
}

inline int32_t BufferClass::To_Page(int w, int h, GraphicViewPortClass& view) {
  return To_Page(0, 0, w, h, view);
}
inline int32_t BufferClass::To_Page(GraphicViewPortClass& view) {
  return To_Page(0, 0, view.Get_Width(), view.Get_Height(), view);
}
inline int32_t BufferClass::To_Page(int x, int y, int w, int h,
                                    GraphicViewPortClass& view) {
  int32_t return_code = 0;
  if (view.Lock()) {
    return_code = Buffer_To_Page(x, y, w, h, Get_Bytes(), &view);
    view.Unlock();
  }
  return return_code;
}

#endif  // CNC_RED_ALERT_SDLLIB_GBUFFER_H_
