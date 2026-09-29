// Bounded access to the legacy games' in-memory INI text, and moving that text
// between a game file and a fixed-capacity buffer it is edited in.
#ifndef CNC_RED_ALERT_ENGINE_FILE_PROFILE_BUFFER_H_
#define CNC_RED_ALERT_ENGINE_FILE_PROFILE_BUFFER_H_

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "engine/stream/byte_stream.h"

namespace engine::file {

// Reads a trimmed value, or the default when absent, into output. A null key
// enumerates complete key names separated by NUL, with a final extra NUL when
// output has room. Empty output performs lookup only. Returns the key-line
// offset (section-body offset for enumeration), or nullopt when absent.
// Section/key names compare without case. LF and CRLF lines are accepted.
std::optional<std::size_t> ReadProfile(std::string_view text,
                                     std::string_view section, const char* key,
                                     const char* default_value,
                                     std::span<char> output);

// Updates an INI document. A null key removes its section; a null value
// removes its key. New lines use CRLF. Removing what is not there changes
// nothing. value may point into profile.
void WriteProfile(std::string& profile, std::string_view section,
                  const char* key, const char* value);

// Returns the text of stream from its current position to its end, or up to
// its first NUL, which ends INI text for the legacy readers.
std::string ReadProfileText(ByteStream& stream);

// Returns the text of the game file name (see OpenGameFile and
// ReadProfileText), or nullopt if it is found nowhere.
std::optional<std::string> ReadProfileFile(std::string_view name);

// Writes profile to the loose file name. Returns false if the file cannot be
// opened or the write falls short.
bool WriteProfileFile(std::string_view name, std::string_view profile);

}  // namespace engine::file
#endif  // CNC_RED_ALERT_ENGINE_FILE_PROFILE_BUFFER_H_
