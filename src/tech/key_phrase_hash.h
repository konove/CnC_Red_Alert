// File: One-way hash of the key phrases that unlock hidden game options.

#ifndef CNC_RED_ALERT_TECH_KEY_PHRASE_HASH_H_
#define CNC_RED_ALERT_TECH_KEY_PHRASE_HASH_H_

#include <cstdint>
#include <string_view>

// Returns the code a key phrase hashes to, for comparing a command-line option
// INI option or chat message against a stored code without keeping the phrase
// itself in the binary. The hash ignores ASCII case, reads at most the first
// 127 characters and stops at an embedded NUL. It must reproduce the original
// games' codes exactly, so the ones in ra/defines.h, ra/const.h and
// td/defines.h keep working.
uint32_t HashKeyPhrase(std::string_view phrase);

#endif  // CNC_RED_ALERT_TECH_KEY_PHRASE_HASH_H_
