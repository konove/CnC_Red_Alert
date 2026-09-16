#ifndef CNC_RED_ALERT_PORT_SAFE_STRING_H_
#define CNC_RED_ALERT_PORT_SAFE_STRING_H_

#include <span>
#include <string_view>

#include "absl/base/attributes.h"

namespace port {

// Copies a terminated source into dest, truncating and zero-padding the rest.
// Empty destinations are ignored; a null source sets the first byte to NUL.
void SafeCopy(std::span<char> dest, const char* src);
// Copies bounded text, which need not be terminated.
void SafeCopy(std::span<char> dest, std::string_view src);

// Appends a terminated source and zero-pads unused capacity. Truncates to keep
// a terminating NUL, including when the original destination was unterminated.
// An empty destination or null source is ignored.
void SafeAppend(std::span<char> dest, const char* src);
// Appends bounded text, which need not be terminated.
void SafeAppend(std::span<char> dest, std::string_view src);

// Returns the writable characters of an existing terminated C string, INCLUDING
// its NUL. Returns an empty span for nullptr. The caller must provide a valid
// writable C string; this does not recover spare destination capacity.
// Clang cannot trace the pointer through libstdc++'s span constructor.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-lifetimebound-violation)
std::span<char> MutableCString(char* text ABSL_ATTRIBUTE_LIFETIME_BOUND);

// Allocates a new copy of `src` on the heap.
//
// Returns a pointer to a newly allocated character array containing a copy of
// `src`. The caller takes ownership of the returned pointer and must free it
// using `delete[]`.
//
// If `src` is null, returns nullptr.
char* CloneString(const char* src);

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_SAFE_STRING_H_
