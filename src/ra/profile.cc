/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/* $Header: /CounterStrike/PROFILE.CPP 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : PROFILE.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : September 10, 1993 *
 *                                                                                             *
 *                  Last Update : September 10, 1993   [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * WWGetPrivateProfileInt -- Fetches integer value from INI. *
 *   WWGetPrivateProfileString -- Fetch string from INI. *
 *   WWWritePrivateProfileInt -- Write a profile int to the profile data block.
 ** WWWritePrivateProfileString -- Write a string to the profile data block. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "ra/profile.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "port/profile_buffer.h"
#include "port/safe_string.h"
#include "tech/number_parse.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "ra/defines.h"
#include "ra/ini.h"
#include "tech/file.h"

bool Read_Private_Config_Struct(File& file, NewConfigType* config) {
  INIClass ini;
  ini.Load(file);

  config->DigitCard = static_cast<unsigned>(ini.Get_Hex("Sound", "Card", 0));
  config->IRQ = static_cast<unsigned>(ini.Get_Int("Sound", "IRQ", 0));
  config->DMA = static_cast<unsigned>(ini.Get_Int("Sound", "DMA", 0));
  config->Port = static_cast<unsigned>(ini.Get_Hex("Sound", "Port", 0));
  config->BitsPerSample =
      static_cast<unsigned>(ini.Get_Int("Sound", "BitsPerSample", 0));
  config->Channels = static_cast<unsigned>(ini.Get_Int("Sound", "Channels", 0));
  config->Reverse = ini.Get_Int("Sound", "Reverse", 0) != 0;
  config->Speed = static_cast<unsigned>(ini.Get_Int("Sound", "Speed", 0));
  ini.Get_String("Language", "Language", nullptr, config->Language,
                 sizeof(config->Language));

  return config->DigitCard == 0 && config->IRQ == 0 && config->DMA == 0;
}

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile) {
  char buffer[16];
  WWGetPrivateProfileString(section, entry, "0", buffer, profile);
  return tech::ParseHex<uint32_t>(buffer).value_or(0);
}

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile) {
  char buffer[16];
  absl::SNPrintF(buffer, sizeof(buffer), "%d", def);
  WWGetPrivateProfileString(section, entry, buffer, buffer, profile);
  return tech::ParseInteger<int>(buffer).value_or(def);
}

bool WWWritePrivateProfileInt(const char* section, const char* entry, int value,
                              std::span<char> profile) {
  char buffer[16];
  absl::SNPrintF(buffer, sizeof(buffer), "%d", value);
  return WWWritePrivateProfileString(section, entry, buffer, profile);
}

const char* WWGetPrivateProfileString(const char* section, const char* key,
                                     const char* def, std::span<char> dest,
                                     const char* ini_data) {
  if (ini_data == nullptr || section == nullptr) {
    port::SafeCopy(dest, def);
    return dest.data();
  }
  const std::string_view text(ini_data);
  const auto found = port::ReadProfile(text, section, key, def, dest);
  return found ? text.substr(*found).data() : nullptr;
}

bool WWWritePrivateProfileString(const char* section, const char* entry,
                                 const char* string, std::span<char> profile) {
  if (profile.empty() || section == nullptr) {
    return true;
  }
  return port::WriteProfile(profile, section, entry, string);
}

namespace {
std::span<char> write_bin_buffer;
std::span<char> read_bin_buffer;
int write_bin_pos = 0;
int write_bin_max = 0;
int read_bin_pos = 0;
int read_bin_max = 0;
}  // namespace

char* Read_Bin_Buffer() { return read_bin_buffer.data(); }
char* Write_Bin_Buffer() { return write_bin_buffer.data(); }

bool Read_Bin_Init(std::span<char> buffer) {
  read_bin_buffer = buffer;
  read_bin_pos = 0;
  read_bin_max = 0;
  return true;
}
bool Write_Bin_Init(std::span<char> buffer) {
  write_bin_buffer = buffer;
  write_bin_pos = 0;
  write_bin_max = 0;
  return true;
}
int Read_Bin_Length(const char* buffer) {
  return buffer == read_bin_buffer.data() ? read_bin_max : -1;
}
int Write_Bin_Length(const char* buffer) {
  return buffer == write_bin_buffer.data() ? write_bin_max : -1;
}
int Read_Bin_Pos(const char* buffer) {
  return buffer == read_bin_buffer.data() ? read_bin_pos : -1;
}
int Write_Bin_Pos(const char* buffer) {
  return buffer == write_bin_buffer.data() ? write_bin_pos : -1;
}
int Read_Bin_PosSet(int pos, const char* buffer) {
  if (buffer != read_bin_buffer.data() || pos < 0 ||
      base::ToSize(pos) > read_bin_buffer.size()) {
    return -1;
  }
  read_bin_pos = pos;
  return pos;
}
int Write_Bin_PosSet(int pos, const char* buffer) {
  if (buffer != write_bin_buffer.data() || pos < 0 ||
      base::ToSize(pos) > write_bin_buffer.size()) {
    return -1;
  }
  write_bin_pos = pos;
  return pos;
}
bool Read_Bin_Num(std::span<std::byte> num, int length, const char* buffer) {
  if (buffer != read_bin_buffer.data() || length <= 0 || length > 4 ||
      base::ToSize(length) > num.size() ||
      base::ToSize(length) > read_bin_buffer.size() - base::ToSize(read_bin_pos)) {
    return false;
  }
  base::CopyBytes(num, std::as_bytes(read_bin_buffer).subspan(base::ToSize(read_bin_pos)), length);
  read_bin_pos += length;
  read_bin_max = std::max(read_bin_pos, read_bin_max);
  return true;
}
bool Write_Bin_Num(std::span<const std::byte> num, int length, const char* buffer) {
  if (buffer != write_bin_buffer.data() || length <= 0 || length > 4 ||
      base::ToSize(length) > num.size() ||
      base::ToSize(length) > write_bin_buffer.size() - base::ToSize(write_bin_pos)) {
    return false;
  }
  base::CopyBytes(std::as_writable_bytes(write_bin_buffer).subspan(base::ToSize(write_bin_pos)), num, length);
  write_bin_pos += length;
  write_bin_max = std::max(write_bin_pos, write_bin_max);
  return true;
}
bool Read_Bin_String(std::span<char> string, const char* buffer) {
  if (buffer != read_bin_buffer.data() || base::ToSize(read_bin_pos) >= read_bin_buffer.size()) {
    return false;
  }
  const auto remaining = read_bin_buffer.subspan(base::ToSize(read_bin_pos));
  const auto length = static_cast<unsigned char>(remaining.front());
  if (static_cast<std::size_t>(length) + 2 > remaining.size() ||
      static_cast<std::size_t>(length) + 1 > string.size() || remaining[static_cast<std::size_t>(length) + 1] != '\0') {
    return false;
  }
  std::ranges::copy(remaining.subspan(1, static_cast<std::size_t>(length) + 1), string.begin());
  read_bin_pos += length + 2;
  read_bin_max = std::max(read_bin_pos, read_bin_max);
  return true;
}
bool Write_Bin_String(std::string_view string, const char* buffer) {
  if (buffer != write_bin_buffer.data() || string.size() > 255 ||
      string.size() + 2 > write_bin_buffer.size() - base::ToSize(write_bin_pos)) {
    return false;
  }
  const auto remaining = write_bin_buffer.subspan(base::ToSize(write_bin_pos));
  remaining.front() = static_cast<char>(string.size());
  std::ranges::copy(string, remaining.subspan(1).begin());
  remaining[string.size() + 1] = '\0';
  write_bin_pos += static_cast<int>(string.size()) + 2;
  write_bin_max = std::max(write_bin_pos, write_bin_max);
  return true;
}
