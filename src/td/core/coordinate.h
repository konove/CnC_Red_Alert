// Tiberian Dawn's world position: a coordinate in leptons, 256 to a cell.

#ifndef CNC_RED_ALERT_TD_CORE_COORDINATE_H_
#define CNC_RED_ALERT_TD_CORE_COORDINATE_H_

#include <compare>
#include <cstdint>
#include <type_traits>

namespace td {

// A world position in leptons. X and Y are 16-bit words packed into one
// 32-bit word, X low and Y high; the high byte of each is the cell and the
// low byte the position within it. Components are plain ints at the interface
// and wrap at 16 bits on the way in, so a coordinate can also carry a negative
// offset for operator+ and operator-.
//
// x() and y() read each word as a signed 16-bit value, -32768..32767, because
// Tiberian Dawn's original code read them through a short; Red Alert's
// Coordinate reads the same words unsigned, 0..65535. The two agree for every
// on-map position (below 0x8000) and differ only for a wrapped negative
// offset. Midpoint() still averages the unsigned words, as TD always did.
//
// Coordinate{} is the zero coordinate, which the game uses as "no
// coordinate"; test for it with `!coord`. NoPosition() is a second, older
// sentinel with every bit set.
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

  // Returns the offset of a cell's center from its upper left corner: half a
  // cell, 0x80 leptons, on each axis (packed 0x00800080). As a position it
  // is the center of cell (0, 0).
  static constexpr Coordinate CellCenterOffset() noexcept {
    return FromLeptons(0x80, 0x80);
  }

  // Returns the "no position" sentinel, every bit set (0xFFFFFFFF, off the
  // map): for example where an object starts before it is placed, and the
  // default position that tells SmudgeClass's constructor not to place
  // itself. It is distinct from Coordinate{}, the zero coordinate, and unlike
  // it is true as a bool.
  static constexpr Coordinate NoPosition() noexcept {
    return FromPacked(0xFFFFFFFFU);
  }

  // Returns the packed word, as saved, sent and logged.
  [[nodiscard]] constexpr uint32_t packed() const noexcept { return packed_; }

  // Returns the X lepton component read signed, in -32768..32767.
  [[nodiscard]] constexpr int x() const noexcept {
    return static_cast<int16_t>(XWord());
  }

  // Returns the Y lepton component read signed, in -32768..32767.
  [[nodiscard]] constexpr int y() const noexcept {
    return static_cast<int16_t>(YWord());
  }

  // Returns the X cell component: the X word's high byte, 0..255.
  [[nodiscard]] constexpr int cell_x() const noexcept {
    return static_cast<int>((packed_ >> 8U) & 0xFFU);
  }

  // Returns the Y cell component: the Y word's high byte, 0..255.
  [[nodiscard]] constexpr int cell_y() const noexcept {
    return static_cast<int>(packed_ >> 24U);
  }

  // Returns the X position within its cell, 0 at the cell's left edge.
  [[nodiscard]] constexpr int sub_x() const noexcept {
    return static_cast<int>(packed_ & 0xFFU);
  }

  // Returns the Y position within its cell, 0 at the cell's top edge.
  [[nodiscard]] constexpr int sub_y() const noexcept {
    return static_cast<int>((packed_ >> 16U) & 0xFFU);
  }

  // Returns the center of the cell this coordinate lies in.
  [[nodiscard]] constexpr Coordinate Snapped() const noexcept {
    return FromPacked((packed_ & kWholeMask) | CellCenterOffset().packed());
  }

  // Returns the upper left corner of the cell this coordinate lies in.
  [[nodiscard]] constexpr Coordinate Whole() const noexcept {
    return FromPacked(packed_ & kWholeMask);
  }

  // Returns only the position within the cell, with both cell components 0.
  [[nodiscard]] constexpr Coordinate Fraction() const noexcept {
    return FromPacked(packed_ & ~kWholeMask);
  }

  // Returns the midpoint of two coordinates, averaging the unsigned 16-bit
  // words (not the signed x() and y()) and rounding down.
  static constexpr Coordinate Midpoint(Coordinate a, Coordinate b) noexcept {
    return FromLeptons((a.XWord() + b.XWord()) / 2,
                       (a.YWord() + b.YWord()) / 2);
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

  // Orders by the packed word: by Y, then by X, both unsigned. The display
  // sorts objects back to front by this order (ObjectClass::Sort_Y()).
  friend constexpr std::strong_ordering operator<=>(Coordinate a,
                                                    Coordinate b) noexcept {
    return a.packed_ <=> b.packed_;
  }

  // Saves and loads the packed word, 4 bytes little-endian.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(packed_);
  }

 private:
  // The cell bytes of both words.
  static constexpr uint32_t kWholeMask = 0xFF00FF00U;

  // The X and Y lepton words, unsigned.
  [[nodiscard]] constexpr int XWord() const noexcept {
    return static_cast<uint16_t>(packed_);
  }
  [[nodiscard]] constexpr int YWord() const noexcept {
    return static_cast<uint16_t>(packed_ >> 16U);
  }

  uint32_t packed_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Coordinate) == 4);
static_assert(std::is_trivially_copyable_v<Coordinate>);
static_assert(std::is_trivially_default_constructible_v<Coordinate>);
static_assert(std::is_standard_layout_v<Coordinate>);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_COORDINATE_H_
