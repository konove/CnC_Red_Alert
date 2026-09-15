#include "port/format.h"

#include <string>
#include <string_view>

#include "absl/log/log.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"

namespace port {

std::string FormatRuntime(const std::string_view format,
                          const absl::Span<const absl::FormatArg> args) {
  std::string out;
  if (!absl::FormatUntyped(&out, absl::UntypedFormatSpec(format), args)) {
    // A string without arguments is usually plain text that happens to hold
    // a '%', so only a failed format that had arguments is worth a note.
    DLOG_IF(WARNING, !args.empty())
        << "format \"" << format << "\" does not match its " << args.size()
        << " argument(s); printing it unformatted";
    out.assign(format);
  }
  return out;
}

}  // namespace port
