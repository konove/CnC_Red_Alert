#include "ra/ui/ui_skin.h"

#include <cstddef>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "ra/assets/compat.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/defines.h"
#include "ra/ui/dialog.h"

namespace ra {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

void UiSkin::Print_Label(PixelView& view, const char* text, const int x,
                         const int y, TextColor color, const PrintFlags style) {
  Simple_Text_Print(view, text, x, y, color, kTBlack, style);
}

void UiSkin::Print_Clipped(PixelView& view, const char* text, const int x,
                           const int y, TextColor color, const PrintFlags style,
                           const int width) {
  Conquer_Clip_Text_Print(view, text, x, y, color, kTBlack, style, width);
}

void UiSkin::Draw_Check_Box(PixelView& view, const int x, const int y,
                            const int width, const int height, const bool on) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_DOWN, false);
  view.FillRect(x + 1, y + 1, x + width - 2, y + height - 2, DKGREY);
  if (on) {
    view.FillRect(x + 1, y + 1, x + width - 2, y + height - 2, kLtGreen);
  }
}

}  // namespace ra
