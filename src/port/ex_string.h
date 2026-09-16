#ifndef CNC_RED_ALERT_PORT_EX_STRING_H_
#define CNC_RED_ALERT_PORT_EX_STRING_H_

#include <cstddef>
#include <string_view>

#include "absl/base/attributes.h"

// Path component buffer sizes, matching the Microsoft CRT's _MAX_* limits.
inline constexpr int kMaxPath = 260;
inline constexpr int kMaxFname = 256;
inline constexpr int kMaxExt = 256;
inline constexpr int kMaxDrive = 3;

namespace port {
// Compares bounded text ranges without requiring a terminating NUL.
int CompareIgnoreCase(std::string_view view1, std::string_view view2);
}  // namespace port

// The Microsoft CRT ships all of these; redeclaring them with C++ linkage is
// an error there, so the portable versions exist only on other platforms.
#ifdef _WIN32
#include <string.h>  // IWYU pragma: keep
#else
// case-insensitive comparisons
int stricmp(const char* string1, const char* string2);
int strnicmp(const char* string1, const char* string2, std::size_t count);

// in-place modification
char* strupr(char* str ABSL_ATTRIBUTE_LIFETIME_BOUND);
char* strlwr(char* str ABSL_ATTRIBUTE_LIFETIME_BOUND);
char* strrev(char* str ABSL_ATTRIBUTE_LIFETIME_BOUND);
#endif  // _WIN32

#endif  // CNC_RED_ALERT_PORT_EX_STRING_H_
