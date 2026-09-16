#include "ra/winbits.h"

#include <cstdint>
#include <cstring>
#include <span>

#include "base/array.h"
#include "base/types.h"
#include "ra/defines.h"
#include "ra/dib.h"
#include "sdllib/drawbuff.h"
#include "sdllib/gbuffer.h"
#include "sdllib/ww_win.h"

LockedWindow::LockedWindow(WindowNumberType window)
    : view_(LogicPage->Get_Graphic_Buffer(),
            base::At(WindowList[static_cast<int>(window)], kWindowX) +
                LogicPage->Get_XPos(),
            base::At(WindowList[static_cast<int>(window)], kWindowY) +
                LogicPage->Get_YPos(),
            base::At(WindowList[static_cast<int>(window)], kWindowWidth),
            base::At(WindowList[static_cast<int>(window)], kWindowHeight)),

      locked_(view_.Lock()) {
  if (locked_) {
    // GraphicViewPortClass calls the end-of-line skip the "pitch", so the
    // distance between rows is that plus the visible width.
    stride_ = view_.Get_Pitch() + view_.Get_Width();
    bits_ = view_.Get_Offset();
  }
}

LockedWindow::~LockedWindow() {
  if (locked_) {
    view_.Unlock();
  }
}

bool SaveSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                     std::uint8_t* bits, WindowNumberType window) {
  const LockedWindow window_bits(window);
  if (!window_bits.bLocked()) {
    return false;
  }
  for (int y = 0; y != hRect; y++) {
    std::memcpy(bits + (base::ssize{y} * wRect),
                window_bits.Row(yRect + y) + xRect,
                static_cast<std::size_t>(wRect));
  }
  return true;
}

bool RestoreSurfaceRect(int xRect, int yRect, int wRect, int hRect,
                        const std::uint8_t* bits, WindowNumberType window) {
  const LockedWindow window_bits(window);
  if (!window_bits.bLocked()) {
    return false;
  }
  for (int y = 0; y != hRect; y++) {
    std::memcpy(window_bits.Row(yRect + y) + xRect,
                bits + (base::ssize{y} * wRect),
                static_cast<std::size_t>(wRect));
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
  if (!window_bits.bLocked()) {
    return;
  }

  const std::span<const std::uint8_t> source = image.Bits();
  for (int y = 0; y != image.Height(); y++) {
    // The image's first row is its bottom one, so it lands on the last line
    // of the destination rectangle.
    std::memcpy(window_bits.Row(yDest + image.Height() - 1 - y) + xDest,
                source.data() + (base::ssize{y} * image.Stride()),
                static_cast<std::size_t>(copy_width));
  }
}
