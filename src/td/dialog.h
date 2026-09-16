#ifndef CNC_RED_ALERT_TD_DIALOG_H_
#define CNC_RED_ALERT_TD_DIALOG_H_

#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "port/format.h"
#include "sdllib/wwstd.h"
#include "td/defines.h"
#include "td/jshell.h"

int Format_Window_String(std::span<char> string, int max_line_len, int& width,
                         int& height);
extern void Dialog_Box(int x, int y, int w, int h);
void Conquer_Clip_Text_Print(const char* /*text*/, int x, int y, int fore,
                             int back = kTBlack,
                             TextPrintType flag = TPF_8POINT | TPF_DROPSHADOW,
                             int width = -1, std::span<const int> tabs = {});
void Draw_Box(int x, int y, int w, int h, BoxStyleEnum up, bool filled);
void Window_Box(WindowNumberType window, BoxStyleEnum style);
// Prints `text`, formatted with `args` as printf would, with a drop shadow.
// A text that is not a format for `args` prints verbatim (see
// port::FormatRuntime); a nullptr text only applies the flags.
void Fancy_Text_Print(const char* text, int x, int y, int fore, int back,
                      TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
// Same, with the text looked up by string-table number; TXT_NONE only applies
// the flags.
void Fancy_Text_Print(int text, int x, int y, int fore, int back,
                      TextPrintType flag,
                      absl::Span<const absl::FormatArg> args = {});
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(const char* text, int x, int y, int fore, int back,
                      TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Fancy_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}
template <typename... Args>
  requires(sizeof...(Args) > 0)
void Fancy_Text_Print(int text, int x, int y, int fore, int back,
                      TextPrintType flag, const Args&... args) {
  const auto packed = port::MakeFormatArgs(args...);
  Fancy_Text_Print(text, x, y, fore, back, flag, absl::MakeConstSpan(packed));
}
void Simple_Text_Print(const char* text, int x, int y, int fore,
                       int back, TextPrintType flag);

#endif  // CNC_RED_ALERT_TD_DIALOG_H_
