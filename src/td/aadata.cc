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

// Aircraft type definitions and AircraftTypeClass method implementations.

#include <filesystem>
#include <string>

#include "port/ex_string.h"
#include "sdllib/shape.h"
#include "td/aircraft.h"
#include "td/building.h"
#include "td/conquer.h"
#include "td/const.h"
#include "td/defines.h"
#include "td/externs.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/jshell.h"
#include "td/mapedit.h"
#include "td/object.h"
#include "td/type.h"

const void* AircraftTypeClass::LRotorData = nullptr;
const void* AircraftTypeClass::RRotorData = nullptr;

// A-10 attack plane
static const AircraftTypeClass AttackPlane(
    AIRCRAFT_A10,  // What kind of aircraft is this.
    TXT_A10,       // Translated text number for aircraft.
    "A10",         // INI name of aircraft.
    99,            // Build level.
    STRUCTF_NONE,  // Building prerequisite.
    false,         // Is a leader type?
    false,         // Does it fire a pair of shots in quick succession?
    false,         // Is this a typical transport vehicle?
    true,          // Fixed wing aircraft?
    false,         // Equipped with a rotor?
    false,         // Custom rotor sets for each facing?
    false,         // Can this aircraft land on clear terrain?
    false,         // Can the aircraft be crushed by a tracked vehicle?
    true,          // Is it invisible on radar?
    false,         // Can the player select it to give it orders?
    true,          // Can it be assigned as a target for attack.
    false,         // Is it insignificant (won't be announced)?
    false,         // Is it immune to normal combat damage?
    false,         // Theater specific graphic image?
    false,         // Can it be repaired in a repair facility?
    false,         // Can the player construct or order this unit?
    true,          // Is there a crew inside?
    3,             // Number of shots it has (default).
    60,            // The strength of this unit.
    0,             // The range that it reveals terrain around itself.
    800,           // Credit cost to construct.
    0,             // The scenario this becomes available.
    10, 1,         // Risk, reward when calculating AI.
    HOUSEF_MULTI1 | HOUSEF_MULTI2 | HOUSEF_MULTI3 | HOUSEF_MULTI4 |
        HOUSEF_MULTI5 | HOUSEF_MULTI6 | HOUSEF_JP | HOUSEF_GOOD |
        HOUSEF_BAD,  // Who can own this aircraft type.
    WEAPON_NAPALM, WEAPON_NONE,
    ARMOR_ALUMINUM,  // Armor type of this aircraft.
    MPH_FAST,        // Maximum speed of aircraft.
    5,               // Rate of turn.
    MISSION_HUNT     // Default mission for aircraft.
);

// Transport helicopter.
static const AircraftTypeClass TransportHeli(
    AIRCRAFT_TRANSPORT,  // What kind of aircraft is this.
    TXT_TRANS,           // Translated text number for aircraft.
    "TRAN",              // INI name of aircraft.
    6,                   // Build level.
    STRUCTF_HELIPAD,     // Building prerequisite.
    false,               // Is a leader type?
    false,               // Does it fire a pair of shots in quick succession?
    true,                //	Is this a typical transport vehicle?
    false,               // Fixed wing aircraft?
    true,                // Equipped with a rotor?
    true,                // Custom rotor sets for each facing?
    true,                // Can this aircraft land on clear terrain?
    false,               // Can the aircraft be crushed by a tracked vehicle?
    true,                // Is it invisible on radar?
    true,                // Can the player select it so as to give it orders?
    true,                // Can it be assigned as a target for attack.
    false,               // Is it insignificant (won't be announced)?
    false,               // Is it immune to normal combat damage?
    false,               // Theater specific graphic image?
    false,               // Can it be repaired in a repair facility?
    true,                // Can the player construct or order this unit?
    true,                // Is there a crew inside?
    0,                   // Number of shots it has (default).
    90,                  // The strength of this unit.
    0,                   // The range that it reveals terrain around itself.
    1500,                // Credit cost to construct.
    98,                  // The scenario this becomes available.
    10, 80,              // Risk, reward when calculating AI.
    HOUSEF_MULTI1 | HOUSEF_MULTI2 | HOUSEF_MULTI3 | HOUSEF_MULTI4 |
        HOUSEF_MULTI5 | HOUSEF_MULTI6 | HOUSEF_JP | HOUSEF_BAD |
        HOUSEF_GOOD,  // Who can own this aircraft type.
    WEAPON_NONE, WEAPON_NONE,
    ARMOR_ALUMINUM,   // Armor type of this aircraft.
    MPH_MEDIUM_FAST,  // Maximum speed of aircraft.
    5,                // Rate of turn.
    MISSION_HUNT      // Default mission for aircraft.
);

// Apache attack helicopter.
static const AircraftTypeClass AttackHeli(
    AIRCRAFT_HELICOPTER,  // What kind of aircraft is this.
    TXT_HELI,             // Translated text number for aircraft.
    "HELI",               // INI name of aircraft.
    6,                    // Build level.
    STRUCTF_HELIPAD,      // Building prerequisite.
    true,                 // Is a leader type?
    true,                 // Does it fire a pair of shots in quick succession?
    false,                //	Is this a typical transport vehicle?
    false,                // Fixed wing aircraft?
    true,                 // Equipped with a rotor?
    false,                // Custom rotor sets for each facing?
    false,                // Can this aircraft land on clear terrain?
    false,                // Can the aircraft be crushed by a tracked vehicle?
    true,                 // Is it invisible on radar?
    true,                 // Can the player select it so as to give it orders?
    true,                 // Can it be assigned as a target for attack.
    false,                // Is it insignificant (won't be announced)?
    false,                // Is it immune to normal combat damage?
    false,                // Theater specific graphic image?
    false,                // Can it be repaired in a repair facility?
    true,                 // Can the player construct or order this unit?
    true,                 // Is there a crew inside?
    15,                   // Number of shots it has (default).
    125,                  // The strength of this unit.
    0,                    // The range that it reveals terrain around itself.
    1200,                 // Credit cost to construct.
    10,                   // The scenario this becomes available.
    10, 80,               // Risk, reward when calculating AI.
    HOUSEF_MULTI1 | HOUSEF_MULTI2 | HOUSEF_MULTI3 | HOUSEF_MULTI4 |
        HOUSEF_MULTI5 | HOUSEF_MULTI6 | HOUSEF_JP |
        HOUSEF_BAD,  // Who can own this aircraft type.
    WEAPON_CHAIN_GUN, WEAPON_NONE,
    ARMOR_STEEL,  // Armor type of this aircraft.
    MPH_FAST,     // Maximum speed of aircraft.
    4,            // Rate of turn.
    MISSION_HUNT  // Default mission for aircraft.
);

// Orca attack helicopter.
static const AircraftTypeClass OrcaHeli(
    AIRCRAFT_ORCA,    // What kind of aircraft is this.
    TXT_ORCA,         // Translated text number for aircraft.
    "ORCA",           // INI name of aircraft.
    6,                // Build level.
    STRUCTF_HELIPAD,  // Building prerequisite.
    true,             // Is a leader type?
    true,             // Does it fire a pair of shots in quick succession?
    false,            //	Is this a typical transport vehicle?
    false,            // Fixed wing aircraft?
    false,            // Equipped with a rotor?
    false,            // Custom rotor sets for each facing?
    false,            // Can this aircraft land on clear terrain?
    false,            // Can the aircraft be crushed by a tracked vehicle?
    true,             // Is it invisible on radar?
    true,             // Can the player select it so as to give it orders?
    true,             // Can it be assigned as a target for attack.
    false,            // Is it insignificant (won't be announced)?
    false,            // Is it immune to normal combat damage?
    false,            // Theater specific graphic image?
    false,            // Can it be repaired in a repair facility?
    true,             // Can the player construct or order this unit?
    true,             // Is there a crew inside?
    6,                // Number of shots it has (default).
    125,              // The strength of this unit.
    0,                // The range that it reveals terrain around itself.
    1200,             // Credit cost to construct.
    10,               // The scenario this becomes available.
    10, 80,           // Risk, reward when calculating AI.
    HOUSEF_MULTI1 | HOUSEF_MULTI2 | HOUSEF_MULTI3 | HOUSEF_MULTI4 |
        HOUSEF_MULTI5 | HOUSEF_MULTI6 | HOUSEF_JP |
        HOUSEF_GOOD,  // Who can own this aircraft type.
    WEAPON_DRAGON, WEAPON_NONE,
    ARMOR_STEEL,  // Armor type of this aircraft.
    MPH_FAST,     // Maximum speed of aircraft.
    4,            // Rate of turn.
    MISSION_HUNT  // Default mission for aircraft.
);

// C-17 transport plane.
static const AircraftTypeClass CargoPlane(
    AIRCRAFT_CARGO,  // What kind of aircraft is this.
    TXT_C17,         // Translated text number for aircraft.
    "C17",           // INI name of aircraft.
    99,              // Build level.
    STRUCTF_NONE,    // Building prerequisite.
    false,           // Is a leader type?
    false,           // Does it fire a pair of shots in quick succession?
    true,            //	Is this a typical transport vehicle?
    true,            // Fixed wing aircraft?
    false,           // Equipped with a rotor?
    false,           // Custom rotor sets for each facing?
    false,           // Can this aircraft land on clear terrain?
    false,           // Can the aircraft be crushed by a tracked vehicle?
    true,            // Is it invisible on radar?
    false,           // Can the player select it so as to give it orders?
    false,           // Can it be assigned as a target for attack.
    false,           // Is it insignificant (won't be announced)?
    false,           // Is it immune to normal combat damage?
    false,           // Theater specific graphic image?
    false,           // Can it be repaired in a repair facility?
    false,           // Can the player construct or order this unit?
    true,            // Is there a crew inside?
    0,               // Number of shots it has (default).
    25,              // The strength of this unit.
    0,               // The range that it reveals terrain around itself.
    800,             // Credit cost to construct.
    0,               // The scenario this becomes available.
    10, 1,           // Risk, reward when calculating AI.
    HOUSEF_MULTI1 | HOUSEF_MULTI2 | HOUSEF_MULTI3 | HOUSEF_MULTI4 |
        HOUSEF_MULTI5 | HOUSEF_MULTI6 | HOUSEF_JP | HOUSEF_GOOD |
        HOUSEF_BAD,  // Who can own this aircraft type.
    WEAPON_NONE, WEAPON_NONE,
    ARMOR_ALUMINUM,  // Armor type of this aircraft.
    MPH_FAST,        // Maximum speed of aircraft.
    5,               // Rate of turn.
    MISSION_HUNT     // Default mission for aircraft.
);

const AircraftTypeClass* const AircraftTypeClass::Pointers[AIRCRAFT_COUNT] = {
    &TransportHeli, &AttackPlane, &AttackHeli, &CargoPlane, &OrcaHeli,
};

AircraftTypeClass::AircraftTypeClass(
    AircraftType airtype, int name, const char* ininame, unsigned char level,
    long pre, bool is_leader, bool is_twoshooter, bool is_transporter,
    bool is_fixedwing, bool is_rotorequipped, bool is_rotorcustom,
    bool is_landable, bool is_crushable, bool is_stealthy, bool is_selectable,
    bool is_legal_target, bool is_insignificant, bool is_immune,
    bool is_theater, bool is_repairable, bool is_buildable, bool is_crew,
    int ammo, unsigned short strength, int sightrange, int cost, int scenario,
    int risk, int reward, int ownable, WeaponType primary, WeaponType secondary,
    ArmorType armor, MPHType maxspeed, int rot, MissionType deforder)
    : TechnoTypeClass(name, ininame, level, pre, is_leader, false, false,
                      is_transporter, false, is_crushable, is_stealthy,
                      is_selectable, is_legal_target, is_insignificant,
                      is_immune, is_theater, is_twoshooter, false,
                      is_repairable, is_buildable, is_crew, ammo, strength,
                      maxspeed, sightrange, cost, scenario, risk, reward,
                      ownable, primary, secondary, armor) {
  IsRotorEquipped = is_rotorequipped;
  IsRotorCustom = is_rotorcustom;
  IsLandable = is_landable;
  IsFixedWing = is_fixedwing;
  Type = airtype;
  ROT = rot;
  Mission = deforder;
}

AircraftType AircraftTypeClass::From_Name(const char* name) {
  if (name) {
    for (AircraftType classid = AIRCRAFT_FIRST; classid < AIRCRAFT_COUNT;
         classid++) {
      if (stricmp(Pointers[classid]->IniName, name) == 0) {
        return classid;
      }
    }
  }
  return AIRCRAFT_NONE;
}

void AircraftTypeClass::One_Time() {
  AircraftType index;

  for (index = AIRCRAFT_FIRST; index < AIRCRAFT_COUNT; index++) {
    const AircraftTypeClass& uclass = As_Reference(index);

    // Load the sidebar cameo icon (hi-res "ICNH" or lo-res "ICON").
    std::string filename;
    if (Get_Resolution_Factor()) {
      filename = std::string(uclass.IniName) + "ICNH";
    } else {
      filename = std::string(uclass.IniName) + "ICON";
    }
    auto fullname =
        std::filesystem::path(filename).replace_extension(".SHP").string();
    (const void*&)uclass.CameoData = MFCD::Retrieve(fullname);

    // Load the main sprite sheet (shared across all houses).
    fullname = std::filesystem::path(uclass.IniName)
                   .replace_extension(".SHP")
                   .string();

    (const void*&)uclass.ImageData = MFCD::Retrieve(fullname);
  }

  LRotorData = MFCD::Retrieve("LROTOR.SHP");
  RRotorData = MFCD::Retrieve("RROTOR.SHP");
}

ObjectClass* AircraftTypeClass::Create_One_Of(HouseClass* house) const {
  return new AircraftClass(Type, house->Class->House);
}

void AircraftTypeClass::Prep_For_Add() {
  for (AircraftType index = AIRCRAFT_FIRST; index < AIRCRAFT_COUNT; ++index) {
    if (As_Reference(index).Get_Image_Data()) {
      Map.Add_To_List(&As_Reference(index));
    }
  }
}

void AircraftTypeClass::Display(int x, int y, WindowNumberType window,
                                HousesType house) const {
  int shape = 0;
  const void* ptr = Get_Cameo_Data();
  if (!ptr) {
    // Fall back to the main sprite sheet; frame 5 is the south-facing pose.
    ptr = Get_Image_Data();
    shape = 5;
  }
  CC_Draw_Shape(ptr, shape, x, y, window,
                SHAPE_CENTER | SHAPE_WIN_REL | SHAPE_FADING,
                HouseClass::As_Pointer(house)->Remap_Table(false, true));
}

const short* AircraftTypeClass::Occupy_List(bool) const {
  static const short _list[] = {0, REFRESH_EOL};
  return _list;
}

// All 8 surrounding cells when landed.
const short* AircraftTypeClass::Overlap_List() const {
  static const short _list[] = {
      -(MAP_CELL_W - 1), -MAP_CELL_W, -(MAP_CELL_W + 1), -1,         1,
      (MAP_CELL_W - 1),  MAP_CELL_W,  (MAP_CELL_W + 1),  REFRESH_EOL};
  return _list;
}

BuildingClass* AircraftTypeClass::Who_Can_Build_Me(bool, bool legal,
                                                   HousesType house) const {
  BuildingClass* anybuilding = nullptr;
  for (int index = 0; index < Buildings.Count(); index++) {
    BuildingClass* building = Buildings.Ptr(index);

    if (building && !building->IsInLimbo &&
        building->House->Class->House == house &&
        building->Mission != MISSION_DECONSTRUCTION &&
        1L << building->ActLike & Ownable &&
        (!legal || building->House->Can_Build(Type, building->ActLike)) &&
        building->Class->ToBuild == RTTI_AIRCRAFTTYPE) {
      if (building->IsLeader) {
        return building;
      }
      anybuilding = building;
    }
  }
  return anybuilding;
}

int AircraftTypeClass::Repair_Cost() const {
  return Fixed_To_Cardinal(Cost / (MaxStrength / REPAIR_STEP), REPAIR_PERCENT);
}

int AircraftTypeClass::Repair_Step() const { return REPAIR_STEP; }

int AircraftTypeClass::Max_Pips() const {
  if (IsTransporter) {
    return Max_Passengers();
  }
  if (Primary != WEAPON_NONE) {
    return 5;
  }
  return 0;
}

bool AircraftTypeClass::Create_And_Place(CELL, HousesType) const {
  return false;
}

void AircraftTypeClass::Init(TheaterType theater) {
  if (theater != LastTheater) {
    if (Get_Resolution_Factor()) {
      for (AircraftType index = AIRCRAFT_FIRST; index < AIRCRAFT_COUNT;
           ++index) {
        const AircraftTypeClass& uclass = As_Reference(index);

        (const void*&)uclass.CameoData = nullptr;

        const auto filename = std::string(uclass.IniName).substr(0, 4) + "ICNH";

        auto fullname = std::filesystem::path(filename)
                            .replace_extension(Theaters[theater].Suffix)
                            .string();

        const void* cameo_ptr = MFCD::Retrieve(fullname);
        if (cameo_ptr) {
          (const void*&)uclass.CameoData = cameo_ptr;
        }
      }
    }
  }
}

void AircraftTypeClass::Dimensions(int& width, int& height) const {
  width = 21;
  height = 20;
}

RTTIType AircraftTypeClass::What_Am_I() const { return RTTI_AIRCRAFTTYPE; };
