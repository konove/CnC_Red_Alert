#ifndef CNC_RED_ALERT_TD_CORE_CONST_H_
#define CNC_RED_ALERT_TD_CORE_CONST_H_

#include "base/enum_array.h"
#include "td/core/defines.h"

namespace td {

class WeaponTypeClass;   // IWYU pragma: keep
class WarheadTypeClass;  // IWYU pragma: keep

extern const unsigned char RemapGreen[256];
extern const unsigned char RemapBlue[256];
extern const unsigned char RemapOrange[256];
extern const unsigned char RemapNone[256];
extern const unsigned char RemapYellow[256];
extern const unsigned char RemapRed[256];
extern const unsigned char RemapBlueGreen[256];
extern const base::EnumArray<WeaponType, WeaponTypeClass, kWeaponCount> Weapons;
extern const base::EnumArray<WarheadType, WarheadTypeClass, kWarheadCount>
    Warheads;
extern const base::EnumArray<SourceType, const char*, kSourceCount> SourceName;
// What each kind of terrain costs to cross, whether it can be built on,
// and the colour it shows as on the map editor's radar.
extern const base::EnumArray<LandType, GroundType, kLandCount> Ground;
extern const base::EnumArray<TheaterType, TheaterDataType, kTheaterCount>
    Theaters;
extern const unsigned char Pixel2Lepton[24];
extern const Coordinate StoppingCoordAbs[5];
extern const base::EnumArray<game::FacingType, CELL> AdjacentCell;
extern const base::EnumArray<game::FacingType, Coordinate> AdjacentCoord;

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_CONST_H_
