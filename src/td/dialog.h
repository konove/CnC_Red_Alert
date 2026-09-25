#ifndef CNC_RED_ALERT_TD_DIALOG_H_
#define CNC_RED_ALERT_TD_DIALOG_H_

#include <span>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "base/strings/format.h"
#include "engine/gfx/font.h"
#include "engine/gfx/pixel_buffer.h"
#include "engine/gfx/wwstd.h"
#include "td/defines.h"
#include "td/jshell.h"

// Word wraps `string` in place so that no line exceeds `max_line_len` pixels
// in `font`, writing '\r' at each break. `width` and `height` receive the
// size of the wrapped text. Returns the number of lines.
int Format_Window_String(const FontStyle& font, std::span<char> string,
                         int max_line_len, int& width, int& height);
extern void Dialog_Box(PixelView& view, int x, int y, int w, int h);
void Conquer_Clip_Text_Print(PixelView& view, const char* /*text*/, int x,
                             int y, int fore, int back = kTBlack,
                             TextPrintType flag = TPF_8POINT | TPF_DROPSHADOW,
                             int width = -1, std::span<const int> tabs = {});
void Draw_Box(PixelView& view, int x, int y, int w, int h, BoxStyleEnum up,
              bool filled);
void Window_Box(PixelView& view, WindowNumberType window, BoxStyleEnum style);
// Prints `text`, formatted with `args` as printf would, with a drop shadow.
// A text that is not a format for `args` prints verbatim (see
// base::FormatRuntime); a nullptr text only applies the flags.
void Fancy_Text_Print(PixelView& view, const char* text, int x, int y, int fore,
                      int back, TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
// Same, with the text looked up by string-table number; TXT_NONE only applies
// the flags.
void Fancy_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(PixelView& view, const char* text, int x, int y, int fore,
                      int back, TextPrintType flag, const Args&... args) {
  const auto packed = base::MakeFormatArgs(args...);
  Fancy_Text_Print(view, text, x, y, fore, back, flag,
                   absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, TextPrintType flag, const Args&... args) {
  const auto packed = base::MakeFormatArgs(args...);
  Fancy_Text_Print(view, text, x, y, fore, back, flag,
                   absl::MakeConstSpan(packed));
}
// What a text print draws with: its adjusted flags, colour and font.
struct TextStyle {
  TextPrintType flag{};  // The flags after the chosen font's own fixups.
  int forecolor = 0;     // Palette index the glyphs print in.
  FontStyle font_style;  // The font, spacing and glyph palette.
};

// Returns the font and spacing `flag` selects by its point size and shadow,
// with the identity palette: all that measuring text needs. A flag without a
// point size (TPF_LASTPOINT) selects the 8-point font.
FontStyle TextFontStyle(TextPrintType flag);

// Returns the style a print with `flag` in the colours `fore` and `back` uses:
// TextFontStyle() plus the glyph palette.
TextStyle TextStyleFor(TextPrintType flag, int fore = kTBlack,
                       int back = kTBlack);

void Simple_Text_Print(PixelView& view, const char* text, int x, int y,
                       int fore, int back, TextPrintType flag);

#endif  // CNC_RED_ALERT_TD_DIALOG_H_
