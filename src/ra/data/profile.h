#ifndef CNC_RED_ALERT_RA_DATA_PROFILE_H_
#define CNC_RED_ALERT_RA_DATA_PROFILE_H_

#include <cstddef>
#include <span>
#include <string_view>

namespace ra {

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

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_DATA_PROFILE_H_
