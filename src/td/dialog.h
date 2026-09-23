#ifndef CNC_RED_ALERT_TD_DIALOG_H_
#define CNC_RED_ALERT_TD_DIALOG_H_

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "port/format.h"
#include "sdllib/pixel_buffer.h"
#include "sdllib/wwstd.h"
#include "td/defines.h"
#include "td/jshell.h"

int Format_Window_String(std::span<char> string, int max_line_len, int& width,
                         int& height);
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
// The font state a text print needs beyond the glyphs themselves.
struct TextStyle {
  TextPrintType flag;  // The flags after the chosen font's own fixups.
  int forecolor;       // Palette index the glyphs print in.
};

// Selects the font, spacing and font palette that `flag` asks for, and returns
// the style a print of that text would use. Draws nothing, so code that only
// needs StringPixelWidth() or g_font_max_height to be right calls this and
// ignores the result.
TextStyle Select_Text_Font(TextPrintType flag, int fore = kTBlack,
                           int back = kTBlack);
void Simple_Text_Print(PixelView& view, const char* text, int x, int y,
                       int fore, int back, TextPrintType flag);

#endif  // CNC_RED_ALERT_TD_DIALOG_H_
