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

/* $Header: /CounterStrike/CCINI.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCINI.CPP *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/24/96 *
 *                                                                                             *
 *                  Last Update : November 1, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: * GetX/PutX -- Fetch and store the game's typed values (units,  *
 *   buildings, weapons, ...) in an INI database by their INI names.         *
 *=============================================================================================*/

#include "ra/data/ini_types.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "absl/log/check.h"
#include "absl/strings/match.h"
#include "base/fixed.h"
#include "base/numeric.h"
#include "base/strings/tokenizer.h"
#include "engine/file/ini_file.h"
#include "game/core/cell_size.h"
#include "game/sim/enums.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/ai/trigtype.h"
#include "ra/assets/audio.h"
#include "ra/assets/theme.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/data/adata.h"
#include "ra/objects/anim.h"
#include "ra/objects/type.h"
#include "ra/objects/warhead.h"
#include "ra/objects/weapon.h"

namespace ra {

static inline int Scale_To_256(int val) {
  val = std::min(val, 100);
  val = std::max(val, 0);
  val = val * 256 / 100;
  val = std::min(val, 255);
  return val;
}

/***********************************************************************************************
 * GetLepton -- Fetches a lepton value from the INI database. *
 *                                                                                             *
 *    This routine will fetch the lepton value as if it were expressed as cells.
 *Example;      * a value of 1 would mean 256 in leptons. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to look under. *
 *                                                                                             *
 *          entry    -- The entry identifier to find. *
 *                                                                                             *
 *          defvalue -- The default value to use if the specified section and
 *entry could      * not be located. *
 *                                                                                             *
 * OUTPUT:  Returns with the lepton value of the section & entry specified. If
 *not found, then * the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
int GetLepton(const engine::file::IniFile& ini, std::string_view section,
              std::string_view entry, int defvalue) {
  const Fixed result = ini.GetFixed(
      section, entry,
      Fixed(static_cast<uint16_t>(defvalue), game::kCellWidthLeptons));
  // An 8.8 Fixed times 256 is its raw 16-bit word, so this is 0..65535.
  return result * game::kCellWidthLeptons;
}

/***********************************************************************************************
 * PutLepton -- Stores a lepton value to the INI database. *
 *                                                                                             *
 *    This routine will store the lepton value as if it were expressed in cells.
 *Example;      * A lepton of 128 will be stored as ".5". *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the value under. *
 *                                                                                             *
 *          entry    -- The entry to store the lepton value at. *
 *                                                                                             *
 *          value    -- The lepton value to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the lepton value stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutLepton(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, int value) {
  return ini.PutFixed(
      section, entry,
      Fixed(static_cast<uint16_t>(value), game::kCellWidthLeptons));
}

/***********************************************************************************************
 * GetMPHType -- Fetches the speed value as a number from 0 to 100.
 **
 *                                                                                             *
 *    This routine will fetch the speed value as if it were expressed as a
 *number from 0       * to 100. The value of 100 would translate into a speed of
 *256 leptons per game frame.     *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to find. *
 *                                                                                             *
 *          defvalue -- The default speed value to use if the entry could not be
 *located.      *
 *                                                                                             *
 * OUTPUT:  Returns with the speed value. If no entry could be found, then the
 *default value   * will be returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
game::MPHType GetMPHType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         game::MPHType defvalue) {
  const int val =
      ini.GetInt(section, entry, static_cast<int>(defvalue) * 100 / 256);
  return static_cast<game::MPHType>(Scale_To_256(val));
}

/***********************************************************************************************
 * PutMPHType -- Stores the speed value to the section & entry
 *specified.         *
 *                                                                                             *
 *    Use this routine to store a speed value into the INI database. The number
 *stored will    * be in a 0..100 format. A speed of 256 leptons per tick would
 *be stored as 100.           *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to store the speed value to. *
 *                                                                                             *
 *          value    -- The speed value to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the speed value stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutMPHType(engine::file::IniFile& ini, std::string_view section,
                std::string_view entry, game::MPHType value) {
  return ini.PutInt(section, entry, static_cast<int>(value) * 100 / 256);
}

/***********************************************************************************************
 * GetOwners -- Fetch the owners (list of house bits). *
 *                                                                                             *
 *    Use this to fetch a house bit array value from the INI database. This
 *value will be      * various bit positions set (1 << house#) for each house
 *specified in the database.        * Houses can be specified in series by the
 *house name separated by commas or by the        * special group names of
 *"soviet", and "allies" to cover the houses that are members of    * these
 *groups. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to search for. *
 *                                                                                             *
 *          defvalue -- The default house bitfield to use if the entry could not
 *be found.     *
 *                                                                                             *
 * OUTPUT:  Returns with the house bitfield value. If the entry could not be
 *found, then the   * default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
uint32_t GetOwners(const engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, uint32_t defvalue) {
  uint32_t ownable = defvalue;
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    ownable = 0;
    base::Tokenizer tokens(value);
    for (std::string_view name = tokens.Next(); !name.empty();
         name = tokens.Next()) {
      ownable |= Owner_From_Name(name);
    }
  }
  return ownable;
}

/***********************************************************************************************
 * PutOwners -- Store the house bitfield to the INI database. *
 *                                                                                             *
 *    Use this routine to store the house bitfield data into the database. The
 *bitfield format * matches the format used by the Get_Owners function. Example;
 *if both England and         * Spain were specified in the bitfield, the entry
 *would be stored as "England,Spain".      *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier that is assigned the value. *
 *                                                                                             *
 *          value    -- The value to assign to the entry. *
 *                                                                                             *
 * OUTPUT:  bool; Was the entry stored in the INI database? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutOwners(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, uint32_t value) {
  std::string buffer;
  // Optimization: avoid repeated allocations for small strings
  buffer.reserve(128);

  // Helper lambda to safely append with comma separation
  const auto append = [&](const std::string_view str) {
    if (!buffer.empty()) {
      buffer += ',';
    }
    buffer += str;
  };

  if ((value & kHouseFlagAllies) == kHouseFlagAllies) {
    append("allies");
    value &= ~kHouseFlagAllies;
  }
  if ((value & kHouseFlagSoviet) == kHouseFlagSoviet) {
    append("soviet");
    value &= ~kHouseFlagSoviet;
  }

  // Iterate through House Types
  for (int i = 0; std::cmp_less(i, magic_enum::enum_count<HousesType>()); ++i) {
    if ((value & base::Bit<uint32_t>(i)) != 0) {
      const auto house = static_cast<HousesType>(i);
      append(HouseTypeClass::As_Reference(house).Name());
    }
  }

  // Only write to INI if we actually constructed a string
  if (!buffer.empty()) {
    return ini.PutString(section, entry, buffer);
  }

  return true;
}

/***********************************************************************************************
 * GetArmorType -- Fetches the armor type from the INI database. *
 *                                                                                             *
 *    This routine will fetch the armor type from the database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- Th identifier for the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the armor type specified in the INI database. If it
 *could not be      * found, then the default value is returned instead. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
game::ArmorType GetArmorType(const engine::file::IniFile& ini,
                             std::string_view section, std::string_view entry,
                             game::ArmorType defvalue) {
  return Armor_From_Name(ini.GetString(section, entry, ArmorName.at(defvalue)));
}

/***********************************************************************************************
 * PutArmorType -- Store the armor type to the INI database. *
 *                                                                                             *
 *    Use this routine to store the specified armor type to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry to store the value at. *
 *                                                                                             *
 *          value    -- The value to store in the database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the entry stored in the database? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutArmorType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, game::ArmorType value) {
  return ini.PutString(section, entry, ArmorName.at(value));
}

/***********************************************************************************************
 * GetVocType -- Fetch a voc (sound effect) from the INI database.
 **
 *                                                                                             *
 *    This routine will fetch a voc number from the database. The voc number
 *will either       * be a valid sound effect or VOC_NONE if no match could be
 *found.                          *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to return if the entry could not be
 *located.         *
 *                                                                                             *
 * OUTPUT:  Returns with the sound effect (VocType) from the INI database. If
 *the entry could  * not be located, then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
VocType GetVocType(const engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, VocType defvalue) {
  return VocFromName(ini.GetString(section, entry, VocName(defvalue)).c_str());
}

/***********************************************************************************************
 * PutVocType -- Store a sound effect identifier into the INI
 *database.           *
 *                                                                                             *
 *    Use this routine to store a voc identifier (stored a the text name of the
 *sound) into    * the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry to assign the value to. *
 *                                                                                             *
 *          value    -- The sound effect to store to the entry. *
 *                                                                                             *
 * OUTPUT:  bool; Was the sound effect entry stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutVocType(engine::file::IniFile& ini, std::string_view section,
                std::string_view entry, VocType value) {
  if (value == VOC_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry, VocName(value));
}

/***********************************************************************************************
 * GetAnimType -- Fetch an animation type number from the INI
 *database.           *
 *                                                                                             *
 *    This will fetch an AnimType number from the INI database. The anim is
 *stored as a text   * name of the art file used for that anim. *
 *                                                                                             *
 * INPUT:   section  -- The section to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          defvalue -- The default AnimType to use if the entry could not be
 *located.         *
 *                                                                                             *
 * OUTPUT:  Returns with the anim type specified in the database. If it could
 *not be found,    * then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
AnimType GetAnimType(const engine::file::IniFile& ini, std::string_view section,
                     std::string_view entry, AnimType defvalue) {
  return Anim_From_Name(ini.GetString(section, entry, Anim_Name(defvalue)));
}

/***********************************************************************************************
 * PutAnimType -- Stores the animation identifier to the INI
 *database.            *
 *                                                                                             *
 *    This routine will store the animation identifier (stored as the text name
 *of the art     * file it uses) to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to place the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to assign the animation number to.
 **
 *                                                                                             *
 *          value    -- The animation identifier to store with the entry. *
 *                                                                                             *
 * OUTPUT:  bool; Was the animation identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutAnimType(engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, AnimType value) {
  if (value == ANIM_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry, Anim_Name(value));
}

UnitType GetUnitType(const engine::file::IniFile& ini, std::string_view section,
                     std::string_view entry, UnitType defvalue) {
  const char* def = "<none>";
  if (defvalue != UNIT_NONE) {
    def = UnitTypeClass::As_Reference(defvalue).Name();
  }
  return UnitTypeClass::From_Name(ini.GetString(section, entry, def));
}

bool PutUnitType(engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, UnitType value) {
  if (value == UNIT_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry,
                       UnitTypeClass::As_Reference(value).Name());
}

/***********************************************************************************************
 * GetWeaponType -- Fetches the weapon type from the INI database.
 **
 *                                                                                             *
 *    This routine will fetch the weapon type from the INI database. The weapon
 *type is        * stored as a custom identifier string. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to search for. *
 *                                                                                             *
 *          defvalue -- The default weapon value to return if the entry could
 *not be located.  *
 *                                                                                             *
 * OUTPUT:  Returns with the weapon type specified by the entry. If the entry
 *could not be     * found then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
WeaponType GetWeaponType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         WeaponType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return Weapon_From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutWeaponType -- Store the weapon identifier to the INI
 *database.              *
 *                                                                                             *
 *    Store the weapon identifier (as custom string name) to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier to store the weapon identifier with. *
 *                                                                                             *
 *          value    -- The weapon identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the weapon identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutWeaponType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, WeaponType value) {
  if (value == WEAPON_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry,
                       WeaponTypeClass::As_Pointer(value)->Name());
}

/***********************************************************************************************
 * GetWarheadType -- Fetch the warhead type from the INI database.
 **
 *                                                                                             *
 *    Will fetch the warhead identifier from the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The identifier for the section to search for the entry
 *under.          *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          defvalue -- The default return value to use if the entry could not
 *be located.     *
 *                                                                                             *
 * OUTPUT:  Returns with the found warhead type. If the entry could not be found
 *then the      * default warhead value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
WarheadType GetWarheadType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           WarheadType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    for (const WarheadType wh : magic_enum::enum_values<WarheadType>()) {
      if (absl::EqualsIgnoreCase(WarheadTypeClass::As_Pointer(wh)->Name(),
                                 value)) {
        return wh;
      }
    }
  }
  return defvalue;
}

/***********************************************************************************************
 * PutWarheadType -- Stores the warhead identifier to the INI
 *database.           *
 *                                                                                             *
 *    This will store the weapon identifier specified to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry to store the warhead identifier. *
 *                                                                                             *
 *          value    -- The warhead identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the warhead identifier stored to the database? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutWarheadType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, WarheadType value) {
  if (value == WARHEAD_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry,
                       WarheadTypeClass::As_Pointer(value)->Name());
}

/***********************************************************************************************
 * GetOverlayType -- Fetch the overlay identifier from the INI
 *database.          *
 *                                                                                             *
 *    This routine will fetch the overlay identifier from the database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the overlay identifier found. If it could not be found,
 *then the      * default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
OverlayType GetOverlayType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           OverlayType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return OverlayTypeClass::From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutOverlayType -- Store the overlay identifier into the INI
 *database.          *
 *                                                                                             *
 *    Use this routine to store the overlay identifier into the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          value    -- The overlay type value to store with the entry. *
 *                                                                                             *
 * OUTPUT:  bool; Was the overlay value stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutOverlayType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, OverlayType value) {
  DCHECK(value != OVERLAY_NONE);
  return ini.PutString(section, entry,
                       OverlayTypeClass::As_Reference(value).Name());
}

/***********************************************************************************************
 * GetBulletType -- Fetch the bullet identifier from the INI
 *database.            *
 *                                                                                             *
 *    Use this routine to fetch the bullet type identifier from the INI
 *database.              *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry to search for. *
 *                                                                                             *
 *          defvalue -- The default bullet type value to return if the entry
 *could not be      * located. *
 *                                                                                             *
 * OUTPUT:  Returns with the bullet type identifier found. If the entry could
 *not be found     * then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
BulletType GetBulletType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         BulletType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    for (const BulletType proj : magic_enum::enum_values<BulletType>()) {
      if (absl::EqualsIgnoreCase(BulletTypeClass::As_Reference(proj).Name(),
                                 value)) {
        //			if
        //(absl::EqualsIgnoreCase(ProjectileNames[proj], buffer))
        //{
        return proj;
      }
    }
  }
  return defvalue;
}

/***********************************************************************************************
 * PutBulletType -- Store the projectile identifier into the INI
 *database.        *
 *                                                                                             *
 *    This routine will store the projectile name (as the identifier) to the INI
 *database.     *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to store the projectile value with.
 **
 *                                                                                             *
 *          value    -- The projectile identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the projectile identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutBulletType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, BulletType value) {
  if (value == BULLET_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry,
                       BulletTypeClass::As_Reference(value).Name());
  //	return(ini.PutString(section, entry, ProjectileNames[value]));
}

/***********************************************************************************************
 * GetHousesType -- Fetch a house identifier from the INI database.
 **
 *                                                                                             *
 *    Use this routine to fetch an individual house identifier from the INI
 *database. This is  * somewhat similar to the Get_Owners function but is
 *limited to a single house.            *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- Identifier for the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the house identifier if it was found. If not found,
 *then the default  * value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
HousesType GetHousesType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         HousesType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return HouseTypeClass::From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutHousesType -- Store a house identifier to the INI database. *
 *                                                                                             *
 *    Use this routine to store the specified house identifier to the INI
 *database.            *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier for the entry to search for. *
 *                                                                                             *
 *          value    -- The house identifier to store in the database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the house identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutHousesType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, HousesType value) {
  return ini.PutString(section, entry,
                       HouseTypeClass::As_Reference(value).Name());
}

/***********************************************************************************************
 * GetVQType -- Fetch the VQ movie identifier from the INI
 *database.              *
 *                                                                                             *
 *    Fetches the VQ movie name (identifier) from the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- Identifier for the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the VQ movie identifier found. If the entry could not
 *be located,     * then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
VQType GetVQType(const engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, VQType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    for (const VQType vq : magic_enum::enum_values<VQType>()) {
      if (absl::EqualsIgnoreCase(value, VQName.at(vq))) {
        return vq;
      }
    }
  }
  return defvalue;
}

/***********************************************************************************************
 * PutVQType -- Store the VQ movie identifier into the INI
 *database.              *
 *                                                                                             *
 *    Use this routine to store the VQ movie identifier into the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier for the entry to store. *
 *                                                                                             *
 *          value    -- The VQ movie identifier to store to the INI database. *
 *                                                                                             *
 * OUTPUT:  bool; Was the VQ identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutVQType(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, VQType value) {
  if (value == VQ_NONE) {
    return ini.PutString(section, entry, "<none>");
  }
  return ini.PutString(section, entry, VQName.at(value));
}

/***********************************************************************************************
 * GetTheaterType -- Fetch the theater type from the INI database.
 **
 *                                                                                             *
 *    This will fetch the theater identifier from the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search for the entry
 *under.              *
 *                                                                                             *
 *          entry    -- Identifier for the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the theater type found. If the entry could not be
 *found, then the     * default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
TheaterType GetTheaterType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           TheaterType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return Theater_From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutTheaterType -- Store the theater identifier to the INI
 *database.            *
 *                                                                                             *
 *    Use this routine to store the theater name to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier for the entry to store. *
 *                                                                                             *
 *          value    -- The theater identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the theater identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutTheaterType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TheaterType value) {
  return ini.PutString(section, entry, Theaters.at(value).Name);
}

/***********************************************************************************************
 * GetTriggerType -- Fetch the trigger type identifier from the INI
 *database.     *
 *                                                                                             *
 *    This routine will fetch the trigger type identifier from the INI database.
 **
 *                                                                                             *
 * INPUT:   section  -- The section to search for the entry under. *
 *                                                                                             *
 *          entry    -- Identifier of the entry to search for. *
 *                                                                                             *
 * OUTPUT:  Returns with the trigger type pointer if a match was found. No match
 *found will    * return a nullptr. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
TriggerType* GetTriggerType(const engine::file::IniFile& ini,
                            std::string_view section, std::string_view entry) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return TriggerType::From_Name(value);
  }
  return nullptr;
}

/***********************************************************************************************
 * PutTriggerType -- Store the trigger identifier to the INI
 *database.            *
 *                                                                                             *
 *    This routine will store the trigger (as its name) to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry name to store the trigger identifier to. *
 *                                                                                             *
 *          value    -- The trigger type to store. The trigger name will be
 *stored.            *
 *                                                                                             *
 * OUTPUT:  bool; Was the trigger name stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutTriggerType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TriggerType* value) {
  return ini.PutString(section, entry, value->Name());
}

/***********************************************************************************************
 * GetThemeType -- Fetch the theme identifier. *
 *                                                                                             *
 *    This routine will fetch the theme identifier from the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to search for the entry under. *
 *                                                                                             *
 *          entry    -- Identifier of the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default theme identifier to return if the entry
 *could not be found.*
 *                                                                                             *
 * OUTPUT:  Returns with the theme identifier if it was found. If not found,
 *then the default  * value is returned instead. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
ThemeType GetThemeType(const engine::file::IniFile& ini,
                       std::string_view section, std::string_view entry,
                       ThemeType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return ThemeClass::From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutThemeType -- Store the theme identifier to the INI database.
 **
 *                                                                                             *
 *    This routine will store the specified theme identifier to the INI
 *database.              *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier for the entry to store the value to. *
 *                                                                                             *
 *          value    -- The theme identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the theme identifier stored. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutThemeType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, ThemeType value) {
  return ini.PutString(section, entry, ThemeClass::Base_Name(value));
}

/***********************************************************************************************
 * GetSourceType -- Fetch the source (edge) type from the INI
 *database.           *
 *                                                                                             *
 *    This routine will fetch the source (reinforcement edge) identifier from
 *the INI          * database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section that the entry will be
 *searched under.      *
 *                                                                                             *
 *          entry    -- Identifier for the entry that will be searched for. *
 *                                                                                             *
 *          defvalue -- The default value to return if the entry could not be
 *located.         *
 *                                                                                             *
 * OUTPUT:  Returns with the source type of the entry if found. If not found,
 *then the         * default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
SourceType GetSourceType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         SourceType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return Source_From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutSourceType -- Store the source (edge) identifier to the INI
 *database.       *
 *                                                                                             *
 *    This will store the source type (reinforcement edge) to the INI database.
 **
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier of the entry to store the source identifier
 *to.             *
 *                                                                                             *
 *          value    -- The source (edge) value to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the source identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutSourceType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, SourceType value) {
  return ini.PutString(section, entry, SourceName.at(value));
}

/***********************************************************************************************
 * GetCrateType -- Fetches a crate type value from the INI
 *database.              *
 *                                                                                             *
 *    This will return with the crate type specified in the INI database. *
 *                                                                                             *
 * INPUT:   section  -- Identifier for the section to search under. *
 *                                                                                             *
 *          entry    -- The entry to find the matching crate value for. *
 *                                                                                             *
 *          defvalue -- The default crate value to return if the entry could not
 *be found.     *
 *                                                                                             *
 * OUTPUT:  Returns with the crate type identified with the specified entry. If
 *the entry      * could not be located, then the default value is returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
CrateType GetCrateType(const engine::file::IniFile& ini,
                       std::string_view section, std::string_view entry,
                       CrateType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return Crate_From_Name(value);
  }
  return defvalue;
}

/***********************************************************************************************
 * PutCrateType -- Stores the crate value in the section and entry
 *specified.     *
 *                                                                                             *
 *    This will store the specified crate value to the section and entry
 *specified.            *
 *                                                                                             *
 * INPUT:   section  -- The section identifier to store the entry under. *
 *                                                                                             *
 *          entry    -- The entry identifier to store the crate value with. *
 *                                                                                             *
 *          value    -- The crate value to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the crate value stored to the INI database? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 08/08/1996 JLB : Created. *
 *=============================================================================================*/
bool PutCrateType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, CrateType value) {
  return ini.PutString(section, entry, CrateNames.at(value));
}

/***********************************************************************************************
 * GetTerrainType -- Fetch the terrain type identifier from the INI
 *database.     *
 *                                                                                             *
 *    Fetches the terrain type number from the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to search for the entry under. *
 *                                                                                             *
 *          entry    -- Identifier for the entry to search for. *
 *                                                                                             *
 *          defvalue -- The default value to use if the entry could not be
 *located.            *
 *                                                                                             *
 * OUTPUT:  Returns with the terrain type if found. If the entry wasn't found,
 *then the        * default value will be returned. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
TerrainType GetTerrainType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           TerrainType defvalue) {
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    return TerrainTypeClass::From_Name(base::Tokenizer(value, ",").Next());
  }
  return defvalue;
}

/***********************************************************************************************
 * PutTerrainType -- Store the terrain type number to the INI
 *database.           *
 *                                                                                             *
 *    This will store the terrain type identifier to the INI database. *
 *                                                                                             *
 * INPUT:   section  -- The section to store the entry under. *
 *                                                                                             *
 *          entry    -- Identifier that the terrain number will be stored to. *
 *                                                                                             *
 *          value    -- The terrain type identifier to store. *
 *                                                                                             *
 * OUTPUT:  bool; Was the terrain identifier stored? *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/03/1996 JLB : Created. *
 *=============================================================================================*/
bool PutTerrainType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TerrainType value) {
  return ini.PutString(section, entry,
                       TerrainTypeClass::As_Reference(value).Name());
}

/***********************************************************************************************
 * GetBuildings -- Fetch a building bitfield from the INI database.
 **
 *                                                                                             *
 *    This routing will fetch the a list of buildings from the INI database. The
 *buildings     * are expressed as a comma separated list of building
 *identifiers. The return value is     * a composite of bits that represent
 *these buildings -- one bit per building type.         *
 *                                                                                             *
 * INPUT:   section  -- The section to search for the entry under. *
 *                                                                                             *
 *          entry    -- The entry to fetch the building list from. *
 *                                                                                             *
 *          defvalue -- The default value to return if the section and entry
 *could not be      * located. *
 *                                                                                             *
 * OUTPUT:  Returns with the building list (as a bitfield). If the entry could
 *not be          * found, the the default value is returned instead. *
 *                                                                                             *
 * WARNINGS:   none *
 *                                                                                             *
 * HISTORY: * 07/11/1996 JLB : Created. *
 *=============================================================================================*/
uint64_t GetBuildings(const engine::file::IniFile& ini,
                      std::string_view section, std::string_view entry,
                      uint64_t defvalue) {
  uint64_t pre = 0;
  const std::string value = ini.GetString(section, entry, "");
  if (!value.empty()) {
    pre = 0;
    base::Tokenizer tokens(value);
    for (std::string_view token = tokens.Next(); !token.empty();
         token = tokens.Next()) {
      const StructType building = BuildingTypeClass::From_Name(token);
      if (building != STRUCT_NONE) {
        pre |= base::Bit<uint64_t>(building);
      }
    }
  } else {
    pre = defvalue;
  }

  return pre;
}

/***********************************************************************************************
 * PutBuildings -- Store a building list to the INI database. *
 *                                                                                             *
 *    This will store a list of buildings to the INI database. The buildings are
 *listed by     * their identifier names separated by commas. *
 *                                                                                             *
 * INPUT:   section  -- The identifier for the section to store the entry under.
 **
 *                                                                                             *
 *          entry    -- The entry to store the building list to. *
 *                                                                                             *
 *          value    -- A list of buildings (in the form of a bit field -- one
 *bit per         * building type). *
 *                                                                                             *
 * OUTPUT:  Was the building list stored to the INI file? *
 *                                                                                             *
 * WARNINGS:   This is limited to the buildings that can be expressed in a
 *bitfield long.      * Which means, there can be only a maximum of 32 building
 *types listed and        * even then, the total line length generated must not
 *exceed 128 bytes.           *
 *                                                                                             *
 * HISTORY: * 07/11/1996 JLB : Created. *
 *=============================================================================================*/
bool PutBuildings(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, const uint64_t value) {
  // We reserve 256 bytes to prevent reallocations for typical lists.
  std::string buffer;
  buffer.reserve(256);

  // The mask matches Get_Buildings: one bit per StructType, so only the
  // first 64 types can be expressed.
  constexpr int limit =
      std::min(64, static_cast<int>(magic_enum::enum_count<StructType>()));

  const auto append = [&](const std::string_view name) {
    if (!buffer.empty()) {
      buffer += ',';
    }
    buffer += name;
  };

  for (int i = 0; i < limit; ++i) {
    if ((value & base::Bit<uint64_t>(i)) != 0) {
      const auto index = static_cast<StructType>(i);

      // Access the building name safely
      append(BuildingTypeClass::As_Reference(index).IniName);
    }
  }

  if (!buffer.empty()) {
    return ini.PutString(section, entry, buffer);
  }

  return true;
}

uint32_t Owner_From_Name(const std::string_view text) {
  uint32_t ownable = 0;
  if (absl::EqualsIgnoreCase(text, "soviet")) {
    ownable |= kHouseFlagSoviet;
  } else {
    if (absl::EqualsIgnoreCase(text, "allies") ||
        absl::EqualsIgnoreCase(text, "allied")) {
      ownable |= kHouseFlagAllies;
    } else {
      const HousesType h = HouseTypeClass::From_Name(text);
      if (h != HOUSE_NONE && (h < HOUSE_MULTI1 || h > HOUSE_MULTI8)) {
        ownable |= base::Bit<uint32_t>(h);
      }
    }
  }
  return ownable;
}

CrateType Crate_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const CrateType crate : magic_enum::enum_values<CrateType>()) {
      if (absl::EqualsIgnoreCase(name, CrateNames.at(crate))) {
        return crate;
      }
    }
  }
  return CRATE_MONEY;
}

TheaterType Theater_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const TheaterType index : magic_enum::enum_values<TheaterType>()) {
      if (absl::EqualsIgnoreCase(name, Theaters.at(index).Name)) {
        return index;
      }
    }
  }
  return THEATER_NONE;
}

SourceType Source_From_Name(const std::string_view name) {
  if (!name.empty()) {
    for (const SourceType source : magic_enum::enum_values<SourceType>()) {
      if (absl::EqualsIgnoreCase(SourceName.at(source), name)) {
        return source;
      }
    }
  }
  return SOURCE_NONE;
}

}  // namespace ra
