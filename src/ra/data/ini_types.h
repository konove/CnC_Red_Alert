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

/* $Header: /CounterStrike/CCINI.H 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S
 ****
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer *
 *                                                                                             *
 *                    File Name : CCINI.H *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic *
 *                                                                                             *
 *                   Start Date : 05/24/96 *
 *                                                                                             *
 *                  Last Update : May 24, 1996 [JLB] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_RA_DATA_INI_TYPES_H_
#define CNC_RED_ALERT_RA_DATA_INI_TYPES_H_

#include <cstdint>
#include <string_view>

#include "engine/file/ini_file.h"
#include "ra/core/defines.h"

namespace ra {

class TriggerTypeClass;

// The game's typed values in an INI database, read and written by their INI
// names. A getter returns `defvalue` when the entry is missing (GetTriggerType
// has none and returns nullptr).
uint64_t GetBuildings(const engine::file::IniFile& ini,
                      std::string_view section, std::string_view entry,
                      uint64_t defvalue);
UnitType GetUnitType(const engine::file::IniFile& ini, std::string_view section,
                     std::string_view entry, UnitType defvalue);
AnimType GetAnimType(const engine::file::IniFile& ini, std::string_view section,
                     std::string_view entry, AnimType defvalue);
game::ArmorType GetArmorType(const engine::file::IniFile& ini,
                             std::string_view section, std::string_view entry,
                             game::ArmorType defvalue);
BulletType GetBulletType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         BulletType defvalue);
HousesType GetHousesType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         HousesType defvalue);
// Returns the entry, written in cells ("1.5"), as leptons in 0..65535, or
// `defvalue` when it is missing. Both keep their low 16 bits.
int GetLepton(const engine::file::IniFile& ini, std::string_view section,
              std::string_view entry, int defvalue);
game::MPHType GetMPHType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         game::MPHType defvalue);
OverlayType GetOverlayType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           OverlayType defvalue);
SourceType GetSourceType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         SourceType defvalue);
TerrainType GetTerrainType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           TerrainType defvalue);
TheaterType GetTheaterType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           TheaterType defvalue);
ThemeType GetThemeType(const engine::file::IniFile& ini,
                       std::string_view section, std::string_view entry,
                       ThemeType defvalue);
TriggerTypeClass* GetTriggerType(const engine::file::IniFile& ini,
                                 std::string_view section,
                                 std::string_view entry);
VQType GetVQType(const engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, VQType defvalue);
VocType GetVocType(const engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, VocType defvalue);
WarheadType GetWarheadType(const engine::file::IniFile& ini,
                           std::string_view section, std::string_view entry,
                           WarheadType defvalue);
WeaponType GetWeaponType(const engine::file::IniFile& ini,
                         std::string_view section, std::string_view entry,
                         WeaponType defvalue);
uint32_t GetOwners(const engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, uint32_t defvalue);
CrateType GetCrateType(const engine::file::IniFile& ini,
                       std::string_view section, std::string_view entry,
                       CrateType defvalue);
bool PutBuildings(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, uint64_t value);
bool PutAnimType(engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, AnimType value);
bool PutUnitType(engine::file::IniFile& ini, std::string_view section,
                 std::string_view entry, UnitType value);
bool PutArmorType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, game::ArmorType value);
bool PutBulletType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, BulletType value);
bool PutHousesType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, HousesType value);
// Writes `value`, in leptons, as the cell count GetLepton() reads (128 is
// stored as ".5"). It keeps its low 16 bits. Returns whether it was stored.
bool PutLepton(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, int value);
bool PutMPHType(engine::file::IniFile& ini, std::string_view section,
                std::string_view entry, game::MPHType value);
bool PutVQType(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, VQType value);
bool PutOverlayType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, OverlayType value);
bool PutOwners(engine::file::IniFile& ini, std::string_view section,
               std::string_view entry, uint32_t value);
bool PutSourceType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, SourceType value);
bool PutTerrainType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TerrainType value);
bool PutTheaterType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TheaterType value);
bool PutThemeType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, ThemeType value);
bool PutTriggerType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, TriggerTypeClass* value);
bool PutVocType(engine::file::IniFile& ini, std::string_view section,
                std::string_view entry, VocType value);
bool PutWarheadType(engine::file::IniFile& ini, std::string_view section,
                    std::string_view entry, WarheadType value);
bool PutWeaponType(engine::file::IniFile& ini, std::string_view section,
                   std::string_view entry, WeaponType value);
bool PutCrateType(engine::file::IniFile& ini, std::string_view section,
                  std::string_view entry, CrateType value);

// Converts an INI owner specification into a house bitfield, one bit per
// HousesType. The names "soviet", "allies" and "allied" expand to every house
// on that side rather than to a single house.
uint32_t Owner_From_Name(const char* text);

// Converts an ASCII crate name into a crate type, defaulting to CRATE_MONEY
// when the name is unrecognized.
CrateType Crate_From_Name(const char* name);

// Converts a theater name from the scenario INI into a theater number, or
// THEATER_NONE if it matches nothing.
TheaterType Theater_From_Name(const char* name);

// Converts a reinforcement source name from the scenario INI into a
// SourceType, or SOURCE_NONE if it matches nothing.
SourceType Source_From_Name(const char* name);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_DATA_INI_TYPES_H_
