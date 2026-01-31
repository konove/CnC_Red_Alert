#ifndef CNC_RED_ALERT_RA_COORD_H_
#define CNC_RED_ALERT_RA_COORD_H_

#include <algorithm>
#include <cstdint>

#include "ra/defines.h"
#include "ra/face.h"
#include "tech/rect.h"

// Returns a list of cell offsets that a dirty rectangle overlaps, relative to
// the cell containing `coord`. The list is REFRESH_EOL-terminated. If
// `no_center` is true, the center cell (offset 0) is excluded.
const short* Coord_Spillage_List(COORDINATE coord, const Rect& rect,
                                 bool no_center = true);

// Moves a point by `distance` in the given direction, compensating for the
// isometric tilt of the playfield (Y displacement is halved). Used for
// screen-space positioning of elements like turrets and fire offsets.
void Normal_Move_Point(short& x, short& y, DirType dir,
                       unsigned short distance);

// Moves a point by `distance` in the given direction without tilt correction.
void Move_Point(short& x, short& y, DirType dir, unsigned short distance);

// Moves `start` by `distance` leptons in the given `facing` direction.
COORDINATE Coord_Move(COORDINATE start, DirType facing,
                      unsigned short distance);

// Returns a random coordinate within `distance` pixels of `coord`. If `lock`
// is true, the result is snapped to the nearest cell center.
COORDINATE Coord_Scatter(COORDINATE coord, unsigned distance,
                         bool lock = false);

DirType Direction(COORDINATE coord1, COORDINATE coord2);
DirType Direction256(COORDINATE coord1, COORDINATE coord2);
DirType Direction8(COORDINATE coord1, COORDINATE coord2);

// Returns the lepton distance between two coordinates using the "Dragon
// Strike" approximation: max(dx,dy) + min(dx,dy)/2.
int Distance(COORDINATE coord1, COORDINATE coord2);

// Returns the lepton distance between two targets. Both targets must be valid.
int Distance(TARGET target1, TARGET target2);

// Returns a REFRESH_EOL-terminated list of cell offsets that an object of
// `maxsize` pixels overlaps. Limited to maxsize <= 48 for the lookup path;
// larger objects use a manually computed or prebuilt 5x5 table.
const short* Coord_Spillage_List(COORDINATE coord, int maxsize);

// Converts a coordinate to its cell number (map array index).
CELL Coord_Cell(COORDINATE coord);

constexpr uint32_t Cardinal_To_Fixed(const uint32_t base,
                                     const uint32_t cardinal) {
  if (base == 0) {
    return 0xFFFF;
  }
  return (cardinal << 8) / base;
}

constexpr uint32_t Fixed_To_Cardinal(const uint32_t base,
                                     const uint32_t fixed) {
  const uint32_t ret = base * fixed + 0x80;
  if (ret > 0x00FFFFFF) {
    return 0xFFFF;
  }
  return ret >> 8;
}

// Rescales a value from an old range to a new range with rounding.
// Maps: (value / old_base) * new_base
constexpr uint32_t Rescale(const uint32_t value, const uint32_t old_base,
                           const uint32_t new_base) {
  if (old_base == 0) {
    return 0xFFFF;  // Maintain existing sentinel behavior
  }

  // Use uint64_t for intermediate product to prevent 32-bit overflow.
  // This is critical for maintainability and correctness.
  const uint64_t intermediate = static_cast<uint64_t>(value) * new_base;
  const uint64_t rounded = (intermediate + old_base / 2) / old_base;

  // Cast back to uint32_t, ensuring we don't exceed the original sentinel
  // range.
  return static_cast<uint32_t>(std::min<uint64_t>(rounded, 0xFFFF));
}

#endif  // CNC_RED_ALERT_RA_COORD_H_
