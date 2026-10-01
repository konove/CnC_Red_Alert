#include "td/ui/ui_skin.h"

#include <cstddef>
#include <span>

#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "td/core/defines.h"
#include "td/startup/conquer.h"
#include "td/ui/dialog.h"

namespace td {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

void UiSkin::Print_Label(PixelView& view, const char* text, const int x,
                         const int y, TextColor color, const PrintFlags style) {
  Fancy_Text_Print(view, "%s", x, y, color, kTBlack, style, text);
}

void UiSkin::Print_Clipped(PixelView& view, const char* text, const int x,
                           const int y, TextColor color, const PrintFlags style,
                           const int width) {
  Conquer_Clip_Text_Print(view, text, x, y, color, kTBlack, style, width);
}

void UiSkin::Draw_Check_Box(PixelView& view, const int x, const int y,
                            const int width, const int height, const bool on) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_DOWN, false);
  view.FillRect(x + 1, y + 1, x + width - 2, y + height - 2, kGrey);
  if (on) {
    view.DrawLine(x + 1, y + 1, x + width - 2, y + height - 2, kBlack);
    view.DrawLine(x + width - 2, y + 1, x + 1, y + height - 2, kBlack);
  }
}

}  // namespace td
