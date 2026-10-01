#ifndef CNC_RED_ALERT_TD_DATA_PROFILE_H_
#define CNC_RED_ALERT_TD_DATA_PROFILE_H_

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "absl/base/attributes.h"
#include "td/core/defines.h"  // NewConfigType.

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile);
// Sets entry in section of the INI text profile, which grows as needed. A
// null entry removes the section, a null string the entry; a null section
// does nothing.
void WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              std::string& profile);
void WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, std::string& profile);
// Reads a value into bounded output, or enumerates keys when key is nullptr.
// Returns the key-line pointer (section body for enumeration), or nullptr when
// absent. Null section/input initializes output from def and returns its data.
const char* WWGetPrivateProfileString(
    const char* section, const char* key, const char* def,
    std::span<char> dest ABSL_ATTRIBUTE_LIFETIME_BOUND, const char* ini_data);

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile);

bool Read_Private_Config_Struct(char* profile, NewConfigType* config);

#endif  // CNC_RED_ALERT_TD_DATA_PROFILE_H_
