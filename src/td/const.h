#ifndef CNC_RED_ALERT_TD_CONST_H_
#define CNC_RED_ALERT_TD_CONST_H_

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
extern const WeaponTypeClass Weapons[WEAPON_COUNT];
extern const WarheadTypeClass Warheads[WARHEAD_COUNT];
extern const char* SourceName[SOURCE_COUNT];
extern GroundType Ground[LAND_COUNT];
extern const TheaterDataType Theaters[THEATER_COUNT];
extern const unsigned char Facing32[256];
extern const unsigned char Facing8[256];
extern const unsigned char Pixel2Lepton[24];
extern const COORDINATE StoppingCoordAbs[5];
extern const CELL AdjacentCell[FACING_COUNT];
extern const COORDINATE AdjacentCoord[FACING_COUNT];

#endif  // CNC_RED_ALERT_TD_CONST_H_
