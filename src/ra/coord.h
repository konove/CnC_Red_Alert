#ifndef CNC_RED_ALERT_RA_COORD_H_
#define CNC_RED_ALERT_RA_COORD_H_

#include <algorithm>
#include <cstdint>
#include <span>

#include "ra/defines.h"
#include "ra/face.h"
#include "tech/rect.h"

// Returns a list of cell offsets that a dirty rectangle overlaps, relative to
// the cell containing `coord`. The list is kRefreshEol-terminated. If
// `no_center` is true, the center cell (offset 0) is excluded.
std::span<const int16_t> Coord_Spillage_List(COORDINATE coord, const Rect& rect,
                                             bool no_center = true);

// Moves `start` by `distance` leptons in the given `facing` direction.
COORDINATE Coord_Move(COORDINATE start, DirType facing, uint16_t distance);

// Returns a random coordinate within `distance` pixels of `coord`. If `lock`
// is true, the result is snapped to the nearest cell center.
COORDINATE Coord_Scatter(COORDINATE coord, int distance,
                         bool lock = false);

DirType Direction(COORDINATE coord1, COORDINATE coord2);
DirType Direction256(COORDINATE coord1, COORDINATE coord2);
DirType Direction8(COORDINATE coord1, COORDINATE coord2);

// Returns the lepton distance between two coordinates using the "Dragon
// Strike" approximation: max(dx,dy) + min(dx,dy)/2.
int Distance(COORDINATE coord1, COORDINATE coord2);

// Returns the lepton distance between two targets. Both targets must be valid.
int Distance(TARGET target1, TARGET target2);

// Returns a kRefreshEol-terminated list of cell offsets that an object of
// `maxsize` pixels overlaps. Limited to maxsize <= 48 for the lookup path;
// larger objects use a manually computed or prebuilt 5x5 table.
std::span<const int16_t> Coord_Spillage_List(COORDINATE coord, int maxsize);

// Converts a coordinate to its cell number (map array index).
CELL Coord_Cell(COORDINATE coord);

// Returns "cardinal" as a fixed-point fraction of "base", where 0x100 is one;
// 0xFFFF if "base" is zero.
constexpr int Cardinal_To_Fixed(const int base, const int cardinal) {
  if (base == 0) {
    return 0xFFFF;
  }
  // Unsigned 32-bit arithmetic keeps the original results, including the
  // wraparound for inputs outside the game's range; the simulation is
  // deterministic across peers only if these stay bit-identical.
  return static_cast<int>((static_cast<uint32_t>(cardinal) << 8) /
                          static_cast<uint32_t>(base));
}

// Returns the rounded "fixed" fraction of "base", where "fixed" is 0x100 for
// one; 0xFFFF if the product does not fit in 24 bits.
constexpr int Fixed_To_Cardinal(const int base, const int fixed) {
  // Unsigned 32-bit arithmetic for the same reason as Cardinal_To_Fixed.
  const uint32_t ret =
      (static_cast<uint32_t>(base) * static_cast<uint32_t>(fixed)) + 0x80;
  if (ret > 0x00FFFFFF) {
    return 0xFFFF;
  }
  return static_cast<int>(ret >> 8);
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
  const uint64_t rounded = (intermediate + (old_base / 2)) / old_base;

  // Cast back to uint32_t, ensuring we don't exceed the original sentinel
  // range.
  return static_cast<uint32_t>(std::min<uint64_t>(rounded, 0xFFFF));
}

// Copies a cell offset list, stopping early at the kRefreshEol terminator.
// At most len elements are written, so dest must have room for that many.
void List_Copy(std::span<const int16_t> source, int len,
               std::span<int16_t> dest);

#endif  // CNC_RED_ALERT_RA_COORD_H_
