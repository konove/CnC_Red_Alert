// Bounded access to the legacy games' in-memory INI text.
#ifndef CNC_RED_ALERT_PORT_PROFILE_BUFFER_H_
#define CNC_RED_ALERT_PORT_PROFILE_BUFFER_H_

#include <cstddef>
#include <optional>
#include <span>
#include <string_view>

namespace port {

// Reads a trimmed value, or the default when absent, into output. A null key
// enumerates complete key names separated by NUL, with a final extra NUL when
// output has room. Empty output performs lookup only. Returns the key-line
// offset (section-body offset for enumeration), or nullopt when absent.
// Section/key names compare without case. LF and CRLF lines are accepted.
std::optional<std::size_t> ReadProfile(std::string_view text,
                                     std::string_view section, const char* key,
                                     const char* default_value,
                                     std::span<char> output);

// Updates a terminated INI document within its actual writable capacity.
// A null key removes its section; a null value removes its key. New lines use
// CRLF. Returns false without modifying storage if it is unterminated or the
// complete result, including NUL, does not fit. Missing deletions succeed.
bool WriteProfile(std::span<char> storage, std::string_view section,
                  const char* key, const char* value);

}  // namespace port
#endif  // CNC_RED_ALERT_PORT_PROFILE_BUFFER_H_
