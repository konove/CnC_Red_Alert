#include "td/ui/ui_skin.h"

#include <cstddef>
#include <span>

#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "td/core/defines.h"
#include "td/data/text.h"
#include "td/startup/conquer.h"
#include "td/ui/dialog.h"

namespace td {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

const char* UiSkin::Text_String(const int id) { return td::Text_String(id); }

int UiSkin::Text_Width(const PrintFlags flags, const char* text) {
  return StringPixelWidth(TextFontStyle(flags), text);
}

int UiSkin::Text_Height(const PrintFlags flags) {
  return FontLineHeight(TextFontStyle(flags));
}

void UiSkin::Draw_Button_Box(PixelView& view, const int x, const int y,
                             const int width, const int height,
                             const PrintFlags flags, const bool disabled,
                             const bool pressed) {
  const BoxStyleEnum style = [&] {
    if (base::Any(flags & TPF_6PT_GRAD)) {
      if (disabled) {
        return BOXSTYLE_GREEN_DIS_RAISED;
      }
      return pressed ? BOXSTYLE_GREEN_DOWN : BOXSTYLE_GREEN_RAISED;
    }
    if (disabled) {
      return BOXSTYLE_DIS_RAISED;
    }
    return pressed ? BOXSTYLE_DOWN : BOXSTYLE_RAISED;
  }();
  Draw_Box(view, x, y, width, height, style, true);
}

void UiSkin::Print_Button_Text(PixelView& view, const char* text, const int x,
                               const int y, const PrintFlags flags,
                               const bool disabled, const bool pressed,
                               const bool on) {
  if (base::Any(flags & TPF_6PT_GRAD)) {
    const TextPrintType state = [&] {
      if (disabled) {
        return static_cast<TextPrintType>(0);
      }
      return pressed || on ? TPF_USE_GRAD_PAL | TPF_BRIGHT_COLOR
                           : TPF_USE_GRAD_PAL | TPF_MEDIUM_COLOR;
    }();
    Fancy_Text_Print(view, text, x, y, kCcGreen, kTBlack,
                     flags | state | TPF_CENTER);
    return;
  }
  int color = kWhite;
  if (disabled) {
    color = kLtGrey;
  } else if (pressed) {
    color = base::Any(flags & TPF_NOSHADOW) ? kGrey : kLtGrey;
  }
  Fancy_Text_Print(view, text, x, y, on ? kRed : color, kTBlack,
                   flags | TPF_CENTER);
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
