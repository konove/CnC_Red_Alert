#ifndef CNC_RED_ALERT_RA_DIALOG_H_
#define CNC_RED_ALERT_RA_DIALOG_H_

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "port/format.h"
#include "ra/defines.h"
#include "sdllib/wwstd.h"

void Draw_Caption(int text, int x, int y, int w);
void Draw_Caption(const char* text, int x, int y, int w);
// Word wraps "string" in place so that no line exceeds "max_line_len" pixels
// when rendered with the current font.
//
// Line breaks are written directly into the buffer: the space (or the '@'
// marker, which callers use to request an explicit break) at each break point
// is overwritten with '\r'. The string therefore keeps its original length and
// must be writable; never pass a string literal. If a single word is wider
// than "max_line_len" the line is broken mid word, which costs one character.
//
// "width" receives the pixel width of the widest resulting line and "height"
// the total pixel height of all lines. Returns the number of lines, or 0 if
// "string" is nullptr.
int Format_Window_String(char* string, int max_line_len, int& width,
                         int& height);
extern void Dialog_Box(int x, int y, int w, int h);
void Conquer_Clip_Text_Print(const char* /*text*/, int x, int y,
                             RemapControlType* fore, int back = kTBlack,
                             TextPrintType flag = static_cast<TextPrintType>(
                                 TPF_8POINT | TPF_DROPSHADOW),
                             int width = -1, const int* tabs = nullptr);
// Draws a bordered box to the current LogicPage.
//
// "x,y" is the upper left corner and "w,h" the size, both in pixels. "up"
// selects the border style, which also picks the color set used for the fill,
// edges, and corners. When "filled" is true the interior is filled first.
//
// This is a low level routine: it draws with raw palette indices and does no
// color adjustment for the current graphic mode.
void Draw_Box(int x, int y, int w, int h, BoxStyleEnum up, bool filled);
void Window_Box(WindowNumberType window, BoxStyleEnum style);
// Prints `text`, formatted with `args` as printf would, in the color scheme
// with a drop shadow. A text that is not a format for `args` prints verbatim
// (see port::FormatRuntime); a nullptr text only applies the flags.
void Fancy_Text_Print(const char* text, int x, int y, RemapControlType* fore,
                      int back, TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
// Same, with the text looked up by string-table number; TXT_NONE only applies
// the flags.
void Fancy_Text_Print(int text, int x, int y, RemapControlType* fore, int back,
                      TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(const char* text, int x, int y, RemapControlType* fore,
                      int back, TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Fancy_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(int text, int x, int y, RemapControlType* fore, int back,
                      TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Fancy_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}
void Simple_Text_Print(const char* text, int x, int y,
                       RemapControlType* fore, int back,
                       TextPrintType flag);
// Fancy_Text_Print with a single palette color in place of the color scheme.
void Plain_Text_Print(int text, int x, int y, int fore, int back,
                      TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
void Plain_Text_Print(const char* text, int x, int y, int fore, int back,
                      TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Plain_Text_Print(int text, int x, int y, int fore, int back,
                      TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Plain_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Plain_Text_Print(const char* text, int x, int y, int fore, int back,
                      TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Plain_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}

#endif  // CNC_RED_ALERT_RA_DIALOG_H_
