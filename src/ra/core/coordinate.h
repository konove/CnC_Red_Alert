// Red Alert's world position: a coordinate in leptons, 256 to a cell.

#ifndef CNC_RED_ALERT_RA_CORE_COORDINATE_H_
#define CNC_RED_ALERT_RA_CORE_COORDINATE_H_

#include <cstdint>
#include <type_traits>

namespace ra {

// A world position in leptons. X and Y are 16-bit words packed into one
// 32-bit word, X low and Y high; the high byte of each is the cell and the
// low byte the position within it. Components are plain ints at the interface
// and wrap at 16 bits on the way in, so a coordinate can also carry a negative
// offset for operator+ and operator-.
//
// Coordinate{} is the zero coordinate, which the game uses as "no
// coordinate"; test for it with `!coord`.
//
// Example:
//   const Coordinate center = Coordinate::FromLeptons(0x0580, 0x0380);
//   const Coordinate corner = center.Whole();  // Cell (5, 3), offset 0.
class Coordinate {
 public:
  // Combines X and Y lepton components. Each keeps its low 16 bits, so a
  // negative component becomes its two's complement word.
  static constexpr Coordinate FromLeptons(int x, int y) noexcept {
    return FromPacked((uint32_t{static_cast<uint16_t>(y)} << 16U) |
                      static_cast<uint16_t>(x));
  }

  // Wraps a packed word: X lepton word in the low 16 bits, Y in the high 16.
  static constexpr Coordinate FromPacked(uint32_t packed) noexcept {
    Coordinate coord{};
    coord.packed_ = packed;
    return coord;
  }

  // Returns the packed word, as saved, sent and logged.
  constexpr uint32_t packed() const noexcept { return packed_; }

  // Returns the X lepton component, in 0..65535.
  constexpr int x() const noexcept { return static_cast<uint16_t>(packed_); }

  // Returns the Y lepton component, in 0..65535.
  constexpr int y() const noexcept {
    return static_cast<uint16_t>(packed_ >> 16U);
  }

  // Returns the X cell component: the X word's high byte.
  constexpr int cell_x() const noexcept { return x() >> 8; }

  // Returns the Y cell component: the Y word's high byte.
  constexpr int cell_y() const noexcept { return y() >> 8; }

  // Returns the X position within its cell, 0 at the cell's left edge.
  constexpr int sub_x() const noexcept { return x() & 0xFF; }

  // Returns the Y position within its cell, 0 at the cell's top edge.
  constexpr int sub_y() const noexcept { return y() & 0xFF; }

  // Returns the center of the cell this coordinate lies in.
  constexpr Coordinate Snapped() const noexcept {
    return FromPacked((packed_ & kWholeMask) | kCenterFraction);
  }

  // Returns the upper left corner of the cell this coordinate lies in.
  constexpr Coordinate Whole() const noexcept {
    return FromPacked(packed_ & kWholeMask);
  }

  // Returns only the position within the cell, with both cell components 0.
  constexpr Coordinate Fraction() const noexcept {
    return FromPacked(packed_ & ~kWholeMask);
  }

  // Returns the midpoint of two coordinates, averaging the unsigned
  // components and rounding down.
  static constexpr Coordinate Midpoint(Coordinate a, Coordinate b) noexcept {
    return FromLeptons((a.x() + b.x()) / 2, (a.y() + b.y()) / 2);
  }

  // True unless this is the zero coordinate.
  constexpr explicit operator bool() const noexcept { return packed_ != 0; }

  // Adds two coordinates component by component; each component wraps at 16
  // bits, and no carry passes from X to Y.
  friend constexpr Coordinate operator+(Coordinate a, Coordinate b) noexcept {
    return FromLeptons(a.x() + b.x(), a.y() + b.y());
  }

  // Subtracts b from a component by component, wrapping at 16 bits.
  friend constexpr Coordinate operator-(Coordinate a, Coordinate b) noexcept {
    return FromLeptons(a.x() - b.x(), a.y() - b.y());
  }

  // Coordinates are equal when their packed words are.
  friend constexpr bool operator==(Coordinate, Coordinate) noexcept = default;

  // Saves and loads the packed word, 4 bytes little-endian.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(packed_);
  }

 private:
  // The cell bytes of both words.
  static constexpr uint32_t kWholeMask = 0xFF00FF00U;
  // The sub-cell position of a cell's center, half a cell on each axis.
  static constexpr uint32_t kCenterFraction = 0x00800080U;

  uint32_t packed_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Coordinate) == 4);
static_assert(std::is_trivially_copyable_v<Coordinate>);
static_assert(std::is_trivially_default_constructible_v<Coordinate>);
static_assert(std::is_standard_layout_v<Coordinate>);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_COORDINATE_H_
