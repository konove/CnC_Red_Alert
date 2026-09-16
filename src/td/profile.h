#ifndef CNC_RED_ALERT_TD_PROFILE_H_
#define CNC_RED_ALERT_TD_PROFILE_H_

#include <cstddef>
#include <span>
#include <string_view>

#include "absl/base/attributes.h"
#include "td/defines.h"  // NewConfigType is an anonymous struct typedef.

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile);
bool WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              std::span<char> profile);
bool WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, std::span<char> profile);
// Reads a value into bounded output, or enumerates keys when key is nullptr.
// Returns the key-line pointer (section body for enumeration), or nullptr when
// absent. Null section/input initializes output from def and returns its data.
const char* WWGetPrivateProfileString(
    const char* section, const char* key, const char* def,
    std::span<char> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, const char* ini_data);

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile);

bool Read_Private_Config_Struct(char* profile, NewConfigType* config);

#endif  // CNC_RED_ALERT_TD_PROFILE_H_
