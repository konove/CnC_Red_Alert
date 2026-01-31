#ifndef CNC_RED_ALERT_TD_PROFILE_H_
#define CNC_RED_ALERT_TD_PROFILE_H_

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           char* profile);
bool WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              char* profile);
bool WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, char* profile);
// Reads a string value from an INI buffer. If key is nullptr, writes all key
// names in the section to dest (null-separated, double-null terminated).
// Returns pointer to the value in ini_data, or nullptr if not found.
char* WWGetPrivateProfileString(const char* section, const char* key,
                                const char* def, char* dest, int dest_len,
                                char* ini_data);
unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                char* profile);

#endif  // CNC_RED_ALERT_TD_PROFILE_H_
