#include "ra/ui/ui_skin.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include "base/fixed.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"
#include "engine/gfx/shape.h"
#include "game/ui/standard_colors.h"
#include "ra/assets/compat.h"
#include "ra/assets/shape_draw.h"
#include "ra/core/config.h"
#include "ra/core/defines.h"
#include "ra/data/text.h"
#include "ra/ui/color_scheme.h"
#include "ra/ui/dialog.h"

namespace ra {

void UiSkin::Draw_Shape(PixelView& view, std::span<const std::byte> shapes,
                        const int frame, const int x, const int y) {
  CC_Draw_Shape(view, shapes, frame, x, y, WINDOW_MAIN,
                engine::gfx::ShapeFlags::kNone);
}

const char* UiSkin::Text_String(const int id) { return ra::TextString(id); }

int UiSkin::Text_Width(const Font font, const PrintFlags flags,
                       const char* text) {
  return StringPixelWidth(TextFontStyle(font, flags), text);
}

int UiSkin::Text_Height(const Font font, const PrintFlags flags) {
  return FontLineHeight(TextFontStyle(font, flags));
}

void UiSkin::Draw_Button_Box(PixelView& view, const int x, const int y,
                             const int width, const int height, Font /*font*/,
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
                               const int y, const Font font,
                               const PrintFlags flags, const bool disabled,
                               const bool pressed, const bool on) {
  const TextPrintType state = [&] {
    if (disabled) {
      return static_cast<TextPrintType>(0);
    }
    return pressed || on ? TPF_USE_GRAD_PAL | TPF_BRIGHT_COLOR
                         : TPF_USE_GRAD_PAL | TPF_MEDIUM_COLOR;
  }();
  TextPrinter(view, {Get_Color_Scheme(), game::kTransparentBlack, font,
                     flags | state | TPF_CENTER})
      .Print(text, {x, y});
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

engine::gfx::PaletteIndex UiSkin::Gauge_Fill_Color() {
  return engine::gfx::PaletteIndex{Get_Color_Scheme()->Bright};
}

FontStyle UiSkin::FontStyleFor(const Font font, const PrintFlags flags) {
  return TextFontStyle(font, flags);
}

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
                            const Font font, const PrintFlags flags,
                            const bool focused, const int max_length) {
  const TextPrintType state =
      focused ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

  const TextPrinter printer(
      view, {color, game::kTransparentBlack, font, flags | state});
  printer.PrintClipped(text, {x + 1, y + 1}, width - 2);

  const FontStyle style = TextFontStyle(font, flags | state);
  if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
      StringPixelWidth(style, text) + StringPixelWidth(style, "_") <
          width - 2) {
    printer.PrintClipped("_", {x + 1 + StringPixelWidth(style, text), y + 1},
                         -1);
  }
}

void UiSkin::Draw_List_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_BOX, true);
}

void UiSkin::Draw_List_Entry(PixelView& view, const char* text, const int x,
                             const int y, const int width,
                             const int line_height, const Font font,
                             PrintFlags flags, const bool selected,
                             std::span<const int> tabs) {
  RemapControlType* scheme = Get_Color_Scheme();
  if (selected) {
    flags = flags | TPF_BRIGHT_COLOR;
    view.FillRect(Rect{x, y, width, line_height}, scheme->Shadow);
  } else if (!base::Any(flags & TPF_USE_GRAD_PAL)) {
    flags = flags | TPF_MEDIUM_COLOR;
  }
  TextPrinter(view, {scheme, game::kTransparentBlack, font, flags})
      .PrintClipped(text, {x, y}, width, tabs);
}

void UiSkin::Print_Label(PixelView& view, const char* text, const int x,
                         const int y, TextColor color, const Font font,
                         const PrintFlags style) {
  TextPrinter(view, {color, game::kTransparentBlack, font, style})
      .Print(text, {x, y});
}

void UiSkin::Print_Clipped(PixelView& view, const char* text, const int x,
                           const int y, TextColor color, const Font font,
                           const PrintFlags style, const int width) {
  TextPrinter(view, {color, game::kTransparentBlack, font, style})
      .PrintClipped(text, {x, y}, width);
}

void UiSkin::Draw_Check_Box(PixelView& view, const int x, const int y,
                            const int width, const int height, const bool on) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_DOWN, false);
  view.FillRect(Rect{x + 1, y + 1, width - 2, height - 2}, DKGREY);
  if (on) {
    view.FillRect(Rect{x + 1, y + 1, width - 2, height - 2}, game::kLightGreen);
  }
}

}  // namespace ra
