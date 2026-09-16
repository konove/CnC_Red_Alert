#ifndef CNC_RED_ALERT_TD_CONST_H_
#define CNC_RED_ALERT_TD_CONST_H_

#include "base/enum_array.h"
#include "td/defines.h"

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
extern base::EnumArray<LandType, GroundType, kLandCount> Ground;
extern const base::EnumArray<TheaterType, TheaterDataType, kTheaterCount>
    Theaters;
extern const unsigned char Facing32[256];
extern const unsigned char Facing8[256];
extern const unsigned char Pixel2Lepton[24];
extern const COORDINATE StoppingCoordAbs[5];
extern const base::EnumArray<FacingType, CELL, kFacingCount> AdjacentCell;
extern const base::EnumArray<FacingType, COORDINATE, kFacingCount>
    AdjacentCoord;

#endif  // CNC_RED_ALERT_TD_CONST_H_
