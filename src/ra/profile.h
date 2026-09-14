#ifndef CNC_RED_ALERT_RA_PROFILE_H_
#define CNC_RED_ALERT_RA_PROFILE_H_

#include "absl/base/attributes.h"

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile);
bool WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              char* profile);
bool WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, char* profile);

// Reads a string value from an INI buffer. If key is nullptr, writes all key
// names in the section to dest (null-separated, double-null terminated).
// Returns pointer to the value in ini_data, or nullptr if not found.
const char* WWGetPrivateProfileString(const char* section, const char* key,
                                      const char* def,
                                      char* dest ABSL_ATTRIBUTE_LIFETIME_BOUND,
                                      int dest_len, const char* ini_data);

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile);

char* Read_Bin_Buffer();
bool Read_Bin_Init(char* buffer, int length);
int Read_Bin_Length(const char* buffer);
bool Read_Bin_Num(void* num, int length, const char* buffer);
int Read_Bin_Pos(const char* buffer);
int Read_Bin_PosSet(int pos, const char* buffer);
bool Read_Bin_String(char* string, const char* buffer);

char* Write_Bin_Buffer();
bool Write_Bin_Init(char* buffer, int length);
int Write_Bin_Length(const char* buffer);
bool Write_Bin_Num(const void* num, int length, const char* buffer);
int Write_Bin_Pos(const char* buffer);
int Write_Bin_PosSet(int pos, const char* buffer);
bool Write_Bin_String(const char* string, int length, const char* buffer);

class File;
struct NewConfigType;

bool Read_Private_Config_Struct(File& file, NewConfigType* config);

#endif  // CNC_RED_ALERT_RA_PROFILE_H_
