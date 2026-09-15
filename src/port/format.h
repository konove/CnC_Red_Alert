// File: printf-style formatting of strings known only at run time.
//
// The game's text printers format translated strings looked up from the
// string table, so the compiler cannot check the format against the
// arguments. FormatRuntime hands that check to absl::FormatUntyped, which
// verifies every conversion against the argument it receives instead of
// reading whatever is on the stack, and falls back to the unformatted text
// when the string is not a format for those arguments.
//
// Example:
//   // "%s is defeated" from the string table
//   const std::string line =
//       port::FormatRuntime(Text_String(TXT_DEFEATED), name);

#ifndef CNC_RED_ALERT_PORT_FORMAT_H_
#define CNC_RED_ALERT_PORT_FORMAT_H_

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "absl/strings/str_format.h"
#include "absl/types/span.h"

namespace port {

// Packs `args` into the span form the out-of-line printers take. The array
// only points at `args`, so it must not outlive them.
template <typename... Args>
std::array<absl::FormatArg, sizeof...(Args)> MakeFormatArgs(
    const Args&... args) {
  return {absl::FormatArg(args)...};
}

// Formats `format` with `args` as printf would, checking each conversion
// against its argument. Returns `format` unchanged when it is not a valid
// format for `args` (a stray `%`, a missing argument, `%d` for a string), so
// a translated string that was never meant as a format still prints; that
// case is DLOG'd when arguments were supplied.
std::string FormatRuntime(std::string_view format,
                          absl::Span<const absl::FormatArg> args);

template <typename... Args>
std::string FormatRuntime(std::string_view format, const Args&... args) {
  const std::array<absl::FormatArg, sizeof...(Args)> packed =
      MakeFormatArgs(args...);
  return FormatRuntime(format, absl::MakeConstSpan(packed));
}

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_FORMAT_H_
