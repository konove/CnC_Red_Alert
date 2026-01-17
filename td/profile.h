#ifndef CNC_RED_ALERT_TD_PROFILE_H_
#define CNC_RED_ALERT_TD_PROFILE_H_

int WWGetPrivateProfileInt(char const* section, char const* entry, int def,
                           char* profile);
bool WWWritePrivateProfileInt(char const* section, char const* entry, int value,
                              char* profile);
bool WWWritePrivateProfileString(char const* section, char const* entry,
                                 char const* string, char* profile);
char* WWGetPrivateProfileString(char const* section, char const* entry,
                                char const* def, char* retbuffer, int retlen,
                                char* profile);
unsigned WWGetPrivateProfileHex(char const* section, char const* entry,
                                char* profile);

#endif  // CNC_RED_ALERT_TD_PROFILE_H_
