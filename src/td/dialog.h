#ifndef CNC_RED_ALERT_TD_DIALOG_H_
#define CNC_RED_ALERT_TD_DIALOG_H_

#include <span>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "port/format.h"
#include "sdllib/font.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/wwstd.h"
#include "td/defines.h"
#include "td/jshell.h"

int Format_Window_String(std::span<char> string, int max_line_len, int& width,
                         int& height);
// Same, measured in `font` rather than the current font.
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
// port::FormatRuntime); a nullptr text only applies the flags.
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
  const auto packed = port::MakeFormatArgs(args...);
  Fancy_Text_Print(view, text, x, y, fore, back, flag,
                   absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(PixelView& view, int text, int x, int y, int fore,
                      int back, TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
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
// point size (TPF_LASTPOINT) keeps the current font, for now.
FontStyle TextFontStyle(TextPrintType flag);

// Returns the style a print with `flag` in the colours `fore` and `back` uses:
// TextFontStyle() plus the glyph palette.
TextStyle TextStyleFor(TextPrintType flag, int fore = kTBlack,
                       int back = kTBlack);

// TextStyleFor(), also installed in the font globals. Temporary: callers move
// to the style it returns (docs/FONT_GLOBALS_PLAN.md). Draws nothing, so code
// that only needs StringPixelWidth() or g_font_max_height to be right calls
// this and ignores the result.
TextStyle Select_Text_Font(TextPrintType flag, int fore = kTBlack,
                           int back = kTBlack);
void Simple_Text_Print(PixelView& view, const char* text, int x, int y,
                       int fore, int back, TextPrintType flag);

#endif  // CNC_RED_ALERT_TD_DIALOG_H_
