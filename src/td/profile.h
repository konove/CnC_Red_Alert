#ifndef CNC_RED_ALERT_TD_PROFILE_H_
#define CNC_RED_ALERT_TD_PROFILE_H_

int WWGetPrivateProfileInt(char const* section, char const* entry, int def,
                           char* profile);
bool WWWritePrivateProfileInt(char const* section, char const* entry, int value,
                              char* profile);
bool WWWritePrivateProfileString(char const* section, char const* entry,
                                 char const* string, char* profile);
// Reads a string value from an INI buffer. If key is nullptr, writes all key
// names in the section to dest (null-separated, double-null terminated).
// Returns pointer to the value in ini_data, or nullptr if not found.
char* WWGetPrivateProfileString(char const* section, char const* key,
                                char const* def, char* dest, int dest_len,
                                char* ini_data);
unsigned WWGetPrivateProfileHex(char const* section, char const* entry,
                                char* profile);

#endif  // CNC_RED_ALERT_TD_PROFILE_H_
