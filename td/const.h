#ifndef CNC_RED_ALERT_TD_CONST_H_
#define CNC_RED_ALERT_TD_CONST_H_

#include "td/defines.h"

class WeaponTypeClass;   // IWYU pragma: keep
class WarheadTypeClass;  // IWYU pragma: keep

extern unsigned char const RemapGreen[256];
extern unsigned char const RemapBlue[256];
extern unsigned char const RemapOrange[256];
extern unsigned char const RemapNone[256];
extern unsigned char const RemapYellow[256];
extern unsigned char const RemapRed[256];
extern unsigned char const RemapBlueGreen[256];
extern WeaponTypeClass const Weapons[WEAPON_COUNT];
extern WarheadTypeClass const Warheads[WARHEAD_COUNT];
extern char const *SourceName[SOURCE_COUNT];
extern GroundType Ground[LAND_COUNT];
extern TheaterDataType const Theaters[THEATER_COUNT];
extern unsigned char const Facing32[256];
extern unsigned char const Facing8[256];
extern unsigned char const Pixel2Lepton[24];
extern COORDINATE const StoppingCoordAbs[5];
extern CELL const AdjacentCell[FACING_COUNT];
extern COORDINATE const AdjacentCoord[FACING_COUNT];

#endif  // CNC_RED_ALERT_TD_CONST_H_
