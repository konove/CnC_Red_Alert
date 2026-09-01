// File: Direct byte access to the pixels of a window on LogicPage.
//
// The drawing code in this game normally goes through shapes, text and the
// gadget classes. A few places -- tooltips saving the pixels they are about to
// cover, the lobby blitting a downloaded bitmap -- need the raw bytes instead.
// This is that: a window's slice of LogicPage, locked for the length of one
// copy and unlocked again on the way out.
//
// Every routine here works in palette indices, one byte per pixel, and does no
// clipping. The caller is responsible for staying inside the window.

#ifndef CNC_RED_ALERT_RA_WINBITS_H_
#define CNC_RED_ALERT_RA_WINBITS_H_

#include <cstdint>

#include "base/types.h"
#include "ra/defines.h"
#include "ra/dib.h"
#include "sdllib/gbuffer.h"

// A window's pixels, locked from construction until destruction.
//
// Lock() can fail, so bLocked() must be checked before Row() is called.
//
// Example:
//   LockedWindow bits(WINDOW_MAIN);
//   if (bits.bLocked()) {
//     std::memset(bits.Row(0), 0, 32);
//   }
class LockedWindow {
 public:
  explicit LockedWindow(WindowNumberType window);

  LockedWindow(const LockedWindow&) = delete;
  LockedWindow& operator=(const LockedWindow&) = delete;

  ~LockedWindow();

  // False when the surface could not be locked; Row() must not be called then.
  bool bLocked() const { return locked_; }

  // Bytes from the start of one row to the start of the next.
  base::ssize Stride() const { return stride_; }

  // The first pixel of row `y`, counting down from the top of the window.
  std::uint8_t* Row(int y) const { return bits_ + (y * stride_); }

 private:
  GraphicViewPortClass view_;
  std::uint8_t* bits_;
  base::ssize stride_;
  bool locked_;
};

// Copies a rectangle of `window` into `bits`, which must hold at least
// `wRect * hRect` bytes. Rows are stored top-down with no padding. Returns
// false if the surface could not be locked, leaving `bits` untouched.
bool SaveSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                     std::uint8_t* bits, WindowNumberType window);

// Puts back what SaveSurfaceRect took, with the same arguments.
bool RestoreSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                        const std::uint8_t* bits, WindowNumberType window);

// Draws `image` with its top left corner at xDest,yDest. The image's rows are
// stored bottom-up and are flipped on the way out.
//
// `iWidth` clips the drawing to that many pixels per row; a width wider than
// the image draws the whole image, and a negative width draws nothing.
void DrawDib(const dib::Image& image, int xDest, int yDest, int iWidth,
             WindowNumberType window);

#endif  // CNC_RED_ALERT_RA_WINBITS_H_
