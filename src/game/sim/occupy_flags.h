// Which parts of a map cell are taken or about to be taken.

#ifndef CNC_RED_ALERT_GAME_SIM_OCCUPY_FLAGS_H_
#define CNC_RED_ALERT_GAME_SIM_OCCUPY_FLAGS_H_

#include <cstdint>

#include "base/bit.h"

namespace game {

// One occupant of a cell; the value is its bit in OccupyFlags. The first five
// are the infantry spots, in the order of a spot index (the center, then the
// corners).
enum class Occupant : uint8_t {
  kCenter,
  kNorthWest,
  kNorthEast,
  kSouthWest,
  kSouthEast,
  kVehicle,   // a vehicle is on the cell or passing over it
  kMonolith,  // some immovable blockage, a tree or a wall
  kBuilding,  // a building, which usually blocks movement
};

// The cell's occupancy bits. For a vehicle, every cell it is passing over has
// kVehicle set. For infantry, the spot it stands at or is headed toward is
// set, but not the spots it passes over. A saved game stores bits(), one
// byte, the Occupant values counting from the low bit.
//
// Example:
//   cell.Flag.Set(Occupant::kVehicle, true);
//   if (cell.Flag.AllSpots()) ...  // no room for more infantry
class OccupyFlags {
 public:
  constexpr OccupyFlags() = default;

  // The flags whose bits() are `bits`.
  [[nodiscard]] static constexpr OccupyFlags FromBits(const uint8_t bits) {
    OccupyFlags flags;
    flags.bits_ = bits;
    return flags;
  }

  // Saved-game support: one byte.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(bits_);
  }

  [[nodiscard]] constexpr bool Has(const Occupant occupant) const {
    return (bits_ & base::Bit<uint8_t>(occupant)) != 0;
  }
  constexpr void Set(const Occupant occupant, const bool on) {
    if (on) {
      bits_ |= base::Bit<uint8_t>(occupant);
    } else {
      bits_ &= static_cast<uint8_t>(~base::Bit<uint8_t>(occupant));
    }
  }
  // Returns a copy with `occupant` set.
  [[nodiscard]] constexpr OccupyFlags With(const Occupant occupant) const {
    OccupyFlags flags = *this;
    flags.Set(occupant, true);
    return flags;
  }

  // True if infantry takes any of the five spots.
  [[nodiscard]] constexpr bool AnySpot() const { return (bits_ & kSpots) != 0; }
  // True if infantry takes all five spots, so no more fits.
  [[nodiscard]] constexpr bool AllSpots() const {
    return (bits_ & kSpots) == kSpots;
  }
  // True if anything at all occupies the cell.
  [[nodiscard]] constexpr bool any() const { return bits_ != 0; }
  [[nodiscard]] constexpr uint8_t bits() const { return bits_; }

 private:
  // The bits of the five infantry spots.
  static constexpr uint8_t kSpots = 0x1F;

  uint8_t bits_ = 0;
};

}  // namespace game

#endif  // CNC_RED_ALERT_GAME_SIM_OCCUPY_FLAGS_H_
