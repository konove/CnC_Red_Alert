#include "td/ui/ui_skin.h"

#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include "base/cardinal.h"
#include "base/numeric.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/shape.h"
#include "engine/gfx/wwstd.h"
#include "td/assets/assets.h"
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
  return StringPixelWidth(TextFontStyle(FontOf(flags), StyleOf(flags)), text);
}

int UiSkin::Text_Height(const PrintFlags flags) {
  return FontLineHeight(TextFontStyle(FontOf(flags), StyleOf(flags)));
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
    Fancy_Text_Print(view, text, x, y, kCcGreen, kTBlack, FontOf(flags),
                     StyleOf(flags) | state | TPF_CENTER);
    return;
  }
  int color = kWhite;
  if (disabled) {
    color = kLtGrey;
  } else if (pressed) {
    color = base::Any(flags & TPF_NOSHADOW) ? kGrey : kLtGrey;
  }
  Fancy_Text_Print(view, text, x, y, on ? kRed : color, kTBlack, FontOf(flags),
                   StyleOf(flags) | TPF_CENTER);
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

unsigned char UiSkin::Gauge_Fill_Color() {
  return static_cast<unsigned char>(kCcBrightGreen);
}

FontStyle UiSkin::Font(const PrintFlags flags) {
  return TextFontStyle(FontOf(flags), StyleOf(flags));
}

UiSkin::PrintFlags UiSkin::Edit_Flags(const PrintFlags flags) { return flags; }

UiSkin::TextColor UiSkin::Edit_Default_Color() { return kCcGreen; }

void UiSkin::Draw_Edit_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_BOX, true);
}

void UiSkin::Draw_Edit_Text(PixelView& view, const char* text, const int x,
                            const int y, const int width, TextColor color,
                            const PrintFlags flags, const bool focused,
                            const int max_length) {
  // Conquer_Clip_Text_Print() selects the font of `flags`; the colour flags
  // added below do not change it.
  const FontStyle font = TextFontStyle(FontOf(flags), StyleOf(flags));
  if (font.font.data().data() ==
      TheAssets().font(FontType::k6PointGradient).data()) {
    const TextPrintType state =
        focused ? TPF_BRIGHT_COLOR : static_cast<TextPrintType>(0);

    Conquer_Clip_Text_Print(view, text, x + 1, y + 1, color, kTBlack,
                            FontOf(flags | state), StyleOf(flags | state),
                            width - 2);

    if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
        StringPixelWidth(font, text) + StringPixelWidth(font, "_") <
            width - 2) {
      Conquer_Clip_Text_Print(view, "_", x + 1 + StringPixelWidth(font, text),
                              y + 1, color, kTBlack, FontOf(flags | state),
                              StyleOf(flags | state));
    }
  } else {
    Conquer_Clip_Text_Print(view, text, x + 1, y + 1, focused ? kBlue : kWhite,
                            kTBlack, FontOf(flags), StyleOf(flags), width - 2);

    if (focused && std::cmp_less(std::string_view(text).size(), max_length) &&
        StringPixelWidth(font, text) + StringPixelWidth(font, "_") <
            width - 2) {
      Conquer_Clip_Text_Print(view, "_", x + 1 + StringPixelWidth(font, text),
                              y + 1, kBlue, kTBlack, FontOf(flags),
                              StyleOf(flags));
    }
  }
}

void UiSkin::Draw_List_Box(PixelView& view, const int x, const int y,
                           const int width, const int height) {
  Draw_Box(view, x, y, width, height, BOXSTYLE_GREEN_BOX, true);
}

void UiSkin::Draw_List_Entry(PixelView& view, const char* text, const int x,
                             const int y, const int width,
                             const int line_height, PrintFlags flags,
                             const bool selected, std::span<const int> tabs) {
  if (base::Any(flags & TPF_6PT_GRAD)) {
    TextPrintType state = flags;
    if (selected) {
      state = state | TPF_BRIGHT_COLOR;
      view.FillRect(x, y, x + width - 1, y + line_height - 1, kCcGreenShadow);
    } else if (!base::Any(state & TPF_USE_GRAD_PAL)) {
      state = state | TPF_MEDIUM_COLOR;
    }
    Conquer_Clip_Text_Print(view, text, x, y, kCcGreen, kTBlack, FontOf(state),
                            StyleOf(state), width, tabs);
  } else {
    Conquer_Clip_Text_Print(view, text, x, y, selected ? kBlue : kWhite,
                            kTBlack, FontOf(flags), StyleOf(flags), width,
                            tabs);
  }
}

void UiSkin::Print_Label(PixelView& view, const char* text, const int x,
                         const int y, TextColor color, const PrintFlags style) {
  Fancy_Text_Print(view, "%s", x, y, color, kTBlack, FontOf(style),
                   StyleOf(style), text);
}

void UiSkin::Print_Clipped(PixelView& view, const char* text, const int x,
                           const int y, TextColor color, const PrintFlags style,
                           const int width) {
  Conquer_Clip_Text_Print(view, text, x, y, color, kTBlack, FontOf(style),
                          StyleOf(style), width);
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
