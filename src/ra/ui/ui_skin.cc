#include "ra/ui/ui_skin.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include "base/fixed.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "ra/assets/compat.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/core/inline.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

const char* UiSkin::Text_String(const int id) { return ra::Text_String(id); }

int UiSkin::Text_Width(const PrintFlags flags, const char* text) {
  return StringPixelWidth(TextFontStyle(flags), text);
}

int UiSkin::Text_Height(const PrintFlags flags) {
  return FontLineHeight(TextFontStyle(flags));
}

void UiSkin::Draw_Button_Box(PixelView& view, const int x, const int y,
                             const int width, const int height,
                             PrintFlags /*flags*/, const bool disabled,
                             const bool pressed) {
  const BoxStyleEnum style = [&] {
    if (disabled) {
      return config::kWolapiEnabled ? BOXSTYLE_BOX : BOXSTYLE_DIS_RAISED;
    }
    return pressed ? BOXSTYLE_DOWN : BOXSTYLE_RAISED;
  }();
  Draw_Box(view, x, y, width, height, style, true);
}

void UiSkin::Print_Button_Text(PixelView& view, const char* text, const int x,
                               const int y, const PrintFlags flags,
                               const bool disabled, const bool pressed,
                               const bool on) {
  const TextPrintType state = [&] {
    if (disabled) {
      return static_cast<TextPrintType>(0);
    }
    return pressed || on ? TPF_USE_GRAD_PAL | TPF_BRIGHT_COLOR
                         : TPF_USE_GRAD_PAL | TPF_MEDIUM_COLOR;
  }();
  Fancy_Text_Print(view, text, x, y, Get_Color_Scheme(), kTBlack,
                   flags | state | TPF_CENTER);
}

int UiSkin::Scale(const int amount, const int numerator,
                  const int denominator) {
  return amount * Fixed(numerator, denominator);
}

void UiSkin::Draw_Gauge_Box(PixelView& view, const int x, const int y,
                            const int width, const int height,
                            const bool raised) {
  Draw_Box(view, x, y, width, height, raised ? BOXSTYLE_RAISED : BOXSTYLE_DOWN,
           true);
}

unsigned char UiSkin::Gauge_Fill_Color() { return Get_Color_Scheme()->Bright; }

FontStyle UiSkin::Font(const PrintFlags flags) { return TextFontStyle(flags); }

UiSkin::PrintFlags UiSkin::Edit_Flags(const PrintFlags flags) {
  return flags & ~TPF_CENTER;
}

UiSkin::TextColor UiSkin::Edit_Default_Color() { return Get_Color_Scheme(); }

void UiSkin::Draw_Edit_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_BOX, true);
}

void UiSkin::Draw_Edit_Text(PixelView& view, const char* text, const int x,
                            const int y, const int width, TextColor color,
                            const PrintFlags flags, const bool focused,
                            const int max_length) {
  const TextPrintType state =
      focused ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

  Conquer_Clip_Text_Print(view, text, x + 1, y + 1, color, kTBlack,
                          flags | state, width - 2);

  const FontStyle font = TextFontStyle(flags | state);
  if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
      StringPixelWidth(font, text) + StringPixelWidth(font, "_") < width - 2) {
    Conquer_Clip_Text_Print(view, "_", x + 1 + StringPixelWidth(font, text),
                            y + 1, color, kTBlack, flags | state);
  }
}

void UiSkin::Draw_List_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_BOX, true);
}

void UiSkin::Draw_List_Entry(PixelView& view, const char* text, const int x,
                             const int y, const int width,
                             const int line_height, PrintFlags flags,
                             const bool selected, std::span<const int> tabs) {
  RemapControlType* scheme = Get_Color_Scheme();
  if (selected) {
    flags = flags | TPF_BRIGHT_COLOR;
    view.FillRect(x, y, x + width - 1, y + line_height - 1, scheme->Shadow);
  } else if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
    flags = flags | TPF_MEDIUM_COLOR;
  }
  Conquer_Clip_Text_Print(view, text, x, y, scheme, kTBlack, flags, width,
                          tabs);
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
