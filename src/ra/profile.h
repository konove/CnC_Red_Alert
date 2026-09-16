#ifndef CNC_RED_ALERT_RA_PROFILE_H_
#define CNC_RED_ALERT_RA_PROFILE_H_

#include <cstddef>
#include <span>

#include "absl/base/attributes.h"
#include <string_view>

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile);
bool WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              std::span<char> profile);
bool WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, std::span<char> profile);

// Reads a value into bounded output, or enumerates keys when key is nullptr.
// Returns the key-line pointer (section body for enumeration), or nullptr when
// absent. Null section/input initializes output from def and returns its data.
const char* WWGetPrivateProfileString(const char* section, const char* key,
                                     const char* def, std::span<char> dest ABSL_ATTRIBUTE_LIFETIME_BOUND,
                                     const char* ini_data);

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile);

char* Read_Bin_Buffer();
bool Read_Bin_Init(std::span<char> buffer);
int Read_Bin_Length(const char* buffer);
bool Read_Bin_Num(std::span<std::byte> num, int length, const char* buffer);
int Read_Bin_Pos(const char* buffer);
int Read_Bin_PosSet(int pos, const char* buffer);
bool Read_Bin_String(std::span<char> string, const char* buffer);

char* Write_Bin_Buffer();
bool Write_Bin_Init(std::span<char> buffer);
int Write_Bin_Length(const char* buffer);
bool Write_Bin_Num(std::span<const std::byte> num, int length, const char* buffer);
int Write_Bin_Pos(const char* buffer);
int Write_Bin_PosSet(int pos, const char* buffer);
bool Write_Bin_String(std::string_view string, const char* buffer);

class File;
struct NewConfigType;

bool Read_Private_Config_Struct(File& file, NewConfigType* config);

#endif  // CNC_RED_ALERT_RA_PROFILE_H_
