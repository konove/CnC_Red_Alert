#include "td/ui/ui_skin.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include "base/cardinal.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/palette_index.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/rect.h"
#include "engine/gfx/shape.h"
#include "game/ui/standard_colors.h"
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

int UiSkin::Text_Width(const Font font, const PrintFlags flags,
                       const char* text) {
  return StringPixelWidth(TextFontStyle(font, flags), text);
}

int UiSkin::Text_Height(const Font font, const PrintFlags flags) {
  return FontLineHeight(TextFontStyle(font, flags));
}

void UiSkin::Draw_Button_Box(PixelView& view, const int x, const int y,
                             const int width, const int height, const Font font,
                             PrintFlags /*flags*/, const bool disabled,
                             const bool pressed) {
  const BoxStyleEnum style = [&] {
    if (font == FontType::k6PointGradient) {
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
                               const int y, const Font font,
                               const PrintFlags flags, const bool disabled,
                               const bool pressed, const bool on) {
  if (font == FontType::k6PointGradient) {
    const TextPrintType state = [&] {
      if (disabled) {
        return static_cast<TextPrintType>(0);
      }
      return pressed || on ? TPF_USE_GRAD_PAL | TPF_BRIGHT_COLOR
                           : TPF_USE_GRAD_PAL | TPF_MEDIUM_COLOR;
    }();
    TextPrinter(view, {kCcGreen, game::kTransparentBlack, font,
                       flags | state | TPF_CENTER})
        .Print(text, {x, y});
    return;
  }
  engine::gfx::PaletteIndex color = game::kWhite;
  if (disabled) {
    color = game::kLightGrey;
  } else if (pressed) {
    color = base::Any(flags & TPF_NOSHADOW) ? game::kGrey : game::kLightGrey;
  }
  TextPrinter(view, {on ? game::kRed : color, game::kTransparentBlack, font,
                     flags | TPF_CENTER})
      .Print(text, {x, y});
}

int UiSkin::Scale(const int amount, const int numerator,
                  const int denominator) {
  return base::FixedToCardinal(amount,
                               base::CardinalToFixed(denominator, numerator));
}

void UiSkin::Draw_Gauge_Box(PixelView& view, const int x, const int y,
                            const int width, const int height,
                            const bool raised) {
  Draw_Box(view, x, y, width, height,
           raised ? BOXSTYLE_GREEN_RAISED : BOXSTYLE_GREEN_DOWN, true);
}

engine::gfx::PaletteIndex UiSkin::Gauge_Fill_Color() { return kCcBrightGreen; }

FontStyle UiSkin::FontStyleFor(const Font font, const PrintFlags flags) {
  return TextFontStyle(font, flags);
}

UiSkin::PrintFlags UiSkin::Edit_Flags(const PrintFlags flags) { return flags; }

UiSkin::TextColor UiSkin::Edit_Default_Color() { return kCcGreen; }

void UiSkin::Draw_Edit_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_BOX, true);
}

void UiSkin::Draw_Edit_Text(PixelView& view, const char* text, const int x,
                            const int y, const int width, TextColor color,
                            const Font font, const PrintFlags flags,
                            const bool focused, const int max_length) {
  const FontStyle style = TextFontStyle(font, flags);
  if (font == FontType::k6PointGradient) {
    const TextPrintType state =
        focused ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

    const TextPrinter printer(
        view, {color, game::kTransparentBlack, font, flags | state});
    printer.PrintClipped(text, {x + 1, y + 1}, width - 2);

    if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
        StringPixelWidth(style, text) + StringPixelWidth(style, "_") <
            width - 2) {
      printer.PrintClipped("_", {x + 1 + StringPixelWidth(style, text), y + 1},
                           -1);
    }
  } else {
    const TextPrinter printer(view, {focused ? game::kBlue : game::kWhite,
                                     game::kTransparentBlack, font, flags});
    printer.PrintClipped(text, {x + 1, y + 1}, width - 2);

    if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
        StringPixelWidth(style, text) + StringPixelWidth(style, "_") <
            width - 2) {
      // `focused` holds here, so the printer is the blue one.
      printer.PrintClipped("_", {x + 1 + StringPixelWidth(style, text), y + 1},
                           -1);
    }
  }
}

void UiSkin::Draw_List_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_BOX, true);
}

void UiSkin::Draw_List_Entry(PixelView& view, const char* text, const int x,
                             const int y, const int width,
                             const int line_height, const Font font,
                             PrintFlags flags, const bool selected,
                             std::span<const int> tabs) {
  if (font == FontType::k6PointGradient) {
    TextPrintType state = flags;
    if (selected) {
      state = state | TPF_BRIGHT_COLOR;
      view.FillRect(Rect{x, y, width, line_height}, kCcGreenShadow);
    } else if (!base::Any(state & TPF_USE_GRAD_PAL)) {
      state = state | TPF_MEDIUM_COLOR;
    }
    TextPrinter(view, {kCcGreen, game::kTransparentBlack, font, state})
        .PrintClipped(text, {x, y}, width, tabs);
  } else {
    TextPrinter(view, {selected ? game::kBlue : game::kWhite,
                       game::kTransparentBlack, font, flags})
        .PrintClipped(text, {x, y}, width, tabs);
  }
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
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_DOWN, false);
  view.FillRect(Rect{x + 1, y + 1, width - 2, height - 2}, game::kGrey);
  if (on) {
    view.DrawLine(x + 1, y + 1, x + width - 2, y + height - 2, game::kBlack);
    view.DrawLine(x + width - 2, y + 1, x + 1, y + height - 2, game::kBlack);
  }
}

}  // namespace td
