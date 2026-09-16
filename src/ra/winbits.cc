#include "ra/winbits.h"

#include <cstddef>
#include <cstdint>
#include <span>

#include "base/array.h"
#include "base/buffer.h"
#include "base/types.h"
#include "ra/defines.h"
#include "ra/dib.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

LockedWindow::LockedWindow(WindowNumberType window)
    : view_(LogicPage->Get_Graphic_Buffer(),
            base::At(base::At(WindowList, static_cast<int>(window)), kWindowX) +
                LogicPage->Get_XPos(),
            base::At(base::At(WindowList, static_cast<int>(window)), kWindowY) +
                LogicPage->Get_YPos(),
            base::At(base::At(WindowList, static_cast<int>(window)),
                     kWindowWidth),
            base::At(base::At(WindowList, static_cast<int>(window)),
                     kWindowHeight)),

      locked_(view_.Lock()) {
  if (locked_) {
    // GraphicViewPortClass calls the end-of-line skip the "pitch", so the
    // distance between rows is that plus the visible width.
    stride_ = view_.Get_Pitch() + view_.Get_Width();
    bits_ = view_.Get_Pixels();
  }
}

LockedWindow::~LockedWindow() {
  if (locked_) {
    view_.Unlock();
  }
}

bool SaveSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                     std::span<std::uint8_t> bits, WindowNumberType window) {
  const LockedWindow window_bits(window);
  if (!window_bits.bLocked() ||
      !window_bits.Contains(xRect, yRect, wRect, hRect) ||
      static_cast<size_t>(wRect) * static_cast<size_t>(hRect) > bits.size()) {
    return false;
  }
  for (int y = 0; y != hRect; y++) {
    base::CopyBytes(std::as_writable_bytes(bits.subspan(
                        static_cast<size_t>(y) * static_cast<size_t>(wRect))),
                    std::as_bytes(window_bits.Row(yRect + y).subspan(
                        static_cast<size_t>(xRect))),
                    wRect);
  }
  return true;
}

bool RestoreSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                        std::span<const std::uint8_t> bits,
                        WindowNumberType window) {
  const LockedWindow window_bits(window);
  if (!window_bits.bLocked() ||
      !window_bits.Contains(xRect, yRect, wRect, hRect) ||
      static_cast<size_t>(wRect) * static_cast<size_t>(hRect) > bits.size()) {
    return false;
  }
  for (int y = 0; y != hRect; y++) {
    base::CopyBytes(std::as_writable_bytes(window_bits.Row(yRect + y).subspan(
                        static_cast<size_t>(xRect))),
                    std::as_bytes(bits.subspan(static_cast<size_t>(y) *
                                               static_cast<size_t>(wRect))),
                    wRect);
  }
  return true;
}

void DrawDib(const dib::Image& image, int xDest, int yDest, int iWidth,
             WindowNumberType window) {
  if (iWidth < 0) {
    return;
  }
  const base::ssize copy_width =
      iWidth > image.Width() ? image.Width() : base::ssize{iWidth};

  const LockedWindow window_bits(window);
  if (!window_bits.bLocked() ||
      !window_bits.Contains(xDest, yDest, static_cast<int>(copy_width),
                            image.Height())) {
    return;
  }

  const std::span<const std::uint8_t> source = image.Bits();
  for (int y = 0; y != image.Height(); y++) {
    // The image's first row is its bottom one, so it lands on the last line
    // of the destination rectangle.
    base::CopyBytes(
        std::as_writable_bytes(window_bits.Row(yDest + image.Height() - 1 - y)
                                   .subspan(static_cast<size_t>(xDest))),
        std::as_bytes(source.subspan(static_cast<size_t>(y * image.Stride()))),
        copy_width);
  }
}
