/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\profile.cpv   2.18   16 Oct 1995 16:51:14
 * JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S
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
 *   WWWritePrivateProfileInt -- Write a profile int to the profile data block.
 ** WWGetPrivateProfileString -- Fetch string from INI. *
 *   WWWritePrivateProfileString -- Write a string to the profile data block. *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#include "td/profile.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "absl/strings/str_format.h"
#include "port/profile_buffer.h"
#include "port/safe_string.h"
#include "td/defines.h"
#include "tech/number_parse.h"

bool Read_Private_Config_Struct(char* profile, NewConfigType* config) {
  config->DigitCard = WWGetPrivateProfileHex("Sound", "Card", profile);
  config->IRQ = static_cast<unsigned>(
      WWGetPrivateProfileInt("Sound", "IRQ", 0, profile));
  config->DMA = static_cast<unsigned>(
      WWGetPrivateProfileInt("Sound", "DMA", 0, profile));
  config->Port = WWGetPrivateProfileHex("Sound", "Port", profile);
  config->BitsPerSample = static_cast<unsigned>(
      WWGetPrivateProfileInt("Sound", "BitsPerSample", 0, profile));
  config->Channels = static_cast<unsigned>(
      WWGetPrivateProfileInt("Sound", "Channels", 0, profile));
  config->Reverse = WWGetPrivateProfileInt("Sound", "Reverse", 0, profile) != 0;
  config->Speed = static_cast<unsigned>(
      WWGetPrivateProfileInt("Sound", "Speed", 0, profile));
  WWGetPrivateProfileString(
      "Language", "Language", nullptr,
      std::span(config->Language).first(static_cast<std::size_t>(3)), profile);

  return config->DigitCard == 0 && config->IRQ == 0 && config->DMA == 0;
}

unsigned WWGetPrivateProfileHex(const char* section, const char* entry,
                                const char* profile) {
  char buffer[16];
  WWGetPrivateProfileString(section, entry, "0", buffer, profile);
  return tech::ParseHexOr<uint32_t>(buffer, 0);
}

int WWGetPrivateProfileInt(const char* section, const char* entry, int def,
                           const char* profile) {
  char buffer[16];
  absl::SNPrintF(buffer, sizeof(buffer), "%d", def);
  WWGetPrivateProfileString(section, entry, buffer, buffer, profile);
  return tech::ParseIntegerOr<int>(buffer, def);
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
