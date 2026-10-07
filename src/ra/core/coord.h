#ifndef CNC_RED_ALERT_RA_CORE_COORD_H_
#define CNC_RED_ALERT_RA_CORE_COORD_H_

#include <algorithm>
#include <cstdint>
#include <span>

#include "engine/gfx/rect.h"
#include "game/sim/direction.h"
#include "ra/core/const.h"
#include "ra/core/defines.h"
#include "ra/core/packed.h"  // IWYU pragma: export

namespace ra {

// Returns a list of cell offsets that a dirty rectangle overlaps, relative to
// the cell containing `coord`. The list is kRefreshEol-terminated. If
// `no_center` is true, the center cell (offset 0) is excluded.
std::span<const int16_t> Coord_Spillage_List(Coordinate coord, const Rect& rect,
                                             bool no_center = true);

// Moves `start` by `distance` leptons in the given `facing` direction.
Coordinate Coord_Move(Coordinate start, game::DirType facing,
                      uint16_t distance);

// Returns a random coordinate within `distance` pixels of `coord`. If `lock`
// is true, the result is snapped to the nearest cell center.
Coordinate Coord_Scatter(Coordinate coord, int distance, bool lock = false);

// Returns the coordinate `x` and `y` pixels right of and below the map's upper
// left corner, each rounded to the nearest lepton. Mouse positions come
// through here; the caller adds the tactical view's own coordinate to get a
// position in the world.
inline Coordinate CoordFromPixels(const int x, const int y) noexcept {
  return Coordinate::FromLeptons(Pixel_To_Lepton(x), Pixel_To_Lepton(y));
}

// Returns the direction from `from` to `to` at the full 256-step
// resolution.
inline game::DirType Direction(const Coordinate from, const Coordinate to) {
  return game::Direction256Toward(from.x(), from.y(), to.x(), to.y());
}

// Returns the direction from `from` to `to` as one of the eight compass
// points, which is cheaper to work out than Direction().
inline game::DirType Direction8(const Coordinate from, const Coordinate to) {
  return game::Direction8Toward(from.x(), from.y(), to.x(), to.y());
}

// Returns the center of the cell next to the one `coord` lies in, in direction
// `dir`. A `dir` outside the eight facings, FacingType::kNone included, wraps
// into them. No map edge check: the result may lie off the map.
inline Coordinate AdjacentCell(const Coordinate coord,
                               const game::FacingType dir) {
  return (kAdjacentCoordOffsets.at(game::AsFacing(static_cast<int>(dir))) +
          coord)
      .Snapped();
}

// As above, with `dir` rounded to the nearest of the eight facings.
inline Coordinate AdjacentCell(const Coordinate coord,
                               const game::DirType dir) {
  return AdjacentCell(coord, game::NearestFacing(dir));
}

// Returns the cell next to `cell` in direction `dir`. A `dir` outside the eight
// facings, FacingType::kNone included, wraps into them. No map edge check:
// stepping off the east or west edge lands on the opposite edge of a
// neighbouring row.
inline Cell AdjacentCell(const Cell cell, const game::FacingType dir) {
  // Wrapped so that FacingType::kNone (-1) does not index before the start of
  // the table.
  return cell.Offset(
      kAdjacentCellOffsets.at(game::AsFacing(static_cast<int>(dir))));
}

// As above, with `dir` rounded to the nearest of the eight facings.
inline Cell AdjacentCell(const Cell cell, const game::DirType dir) {
  return AdjacentCell(cell, game::NearestFacing(dir));
}

// Calculates the DirType from one cell to another (8-direction accuracy).
inline game::DirType Direction(Cell cell1, Cell cell2) {
  return game::Direction8Toward(cell1.x(), cell1.y(), cell2.x(), cell2.y());
}

// Returns the lepton distance between two coordinates using the "Dragon
// Strike" approximation: max(dx,dy) + min(dx,dy)/2.
int Distance(Coordinate coord1, Coordinate coord2);

// Returns the lepton distance between two targets. Both targets must be valid.
int Distance(Target target1, Target target2);

// Returns a kRefreshEol-terminated list of cell offsets that an object of
// `maxsize` pixels overlaps. Limited to maxsize <= 48 for the lookup path;
// larger objects use a manually computed or prebuilt 5x5 table.
std::span<const int16_t> Coord_Spillage_List(Coordinate coord, int maxsize);

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

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_COORD_H_
