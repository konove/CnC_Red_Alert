// File: One-way hash of the key phrases that unlock hidden game options.

#ifndef CNC_RED_ALERT_RA_OBFUSCATE_H_
#define CNC_RED_ALERT_RA_OBFUSCATE_H_

#include <cstdint>
#include <string_view>

// Returns the code a key phrase hashes to, for comparing a command-line option
// or chat message against a stored code without keeping the phrase itself in
// the binary. The hash ignores ASCII case, reads at most the first 127
// characters and stops at an embedded NUL. It must reproduce the original
// game's codes exactly, so the ones in defines.h and const.h keep working.
uint32_t Obfuscate(std::string_view string);

#endif  // CNC_RED_ALERT_RA_OBFUSCATE_H_
