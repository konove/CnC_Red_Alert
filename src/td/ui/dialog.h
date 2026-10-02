#ifndef CNC_RED_ALERT_TD_UI_DIALOG_H_
#define CNC_RED_ALERT_TD_UI_DIALOG_H_

#include <span>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/strings/format.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "td/core/defines.h"
#include "td/core/jshell.h"

namespace td {

// Word wraps `string` in place so that no line exceeds `max_line_len` pixels
// in `font`, writing '\r' at each break. `width` and `height` receive the
// size of the wrapped text. Returns the number of lines.
int Format_Window_String(const FontStyle& font, std::span<char> string,
                         int max_line_len, int& width, int& height);
extern void Dialog_Box(PixelView& view, int x, int y, int w, int h);
void Conquer_Clip_Text_Print(PixelView& view, const char* /*text*/, int x,
                             int y, int fore, int back = kTBlack,
                             FontType font = FontType::k8Point,
                             TextPrintType flag = TPF_DROPSHADOW,
                             int width = -1, std::span<const int> tabs = {});
void Draw_Box(PixelView& view, int x, int y, int w, int h, BoxStyleEnum up,
              bool filled);
void Window_Box(PixelView& view, WindowNumberType window, BoxStyleEnum style);
// Prints `text`, formatted with `args` as printf would, with a drop shadow.
// A text that is not a format for `args` prints verbatim (see
// base::FormatRuntime); a nullptr text only applies the flags.
void Fancy_Text_Print(PixelView& view, const char* text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
// Same, with the text looked up by string-table number; TXT_NONE only applies
// the flags.
void Fancy_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(PixelView& view, const char* text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flag,
                      const Args&... args) {
  const auto packed = base::MakeFormatArgs(args...);
  Fancy_Text_Print(view, text, x, y, fore, back, font, flag,
                   absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, FontType font, TextPrintType flag,
                      const Args&... args) {
  const auto packed = base::MakeFormatArgs(args...);
  Fancy_Text_Print(view, text, x, y, fore, back, font, flag,
                   absl::MakeConstSpan(packed));
}
// What a text print draws with: its adjusted flags, colour and font.
struct TextStyle {
  TextPrintType flag{};  // The style flags after the font's own fixups.
  int forecolor = 0;     // Palette index the glyphs print in.
  FontStyle font_style;  // The font, spacing and glyph palette.
};

// Returns the font and spacing `font` and the shadow in `flags` select, with
// the identity palette: all that measuring text needs.
FontStyle TextFontStyle(FontType font, TextPrintType flags);

// Returns the style a print in `font` with `flags` in the colours `fore` and
// `back` uses: TextFontStyle() plus the glyph palette.
TextStyle TextStyleFor(FontType font, TextPrintType flags, int fore = kTBlack,
                       int back = kTBlack);

// Transitional, deleted by docs/TEXT_PRINT_SPLIT_PLAN.md Task 8: the font a
// combined TextPrintType selects (none selects the 8-point font) and its flags
// without the font bits.
FontType FontOf(TextPrintType combined);
TextPrintType StyleOf(TextPrintType combined);

void Simple_Text_Print(PixelView& view, const char* text, int x, int y,
                       int fore, int back, FontType font, TextPrintType flag);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_UI_DIALOG_H_
