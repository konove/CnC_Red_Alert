// Tiberian Dawn's reference to a game object, a team, a trigger or a map
// cell, packed into the 16-bit word that saves and network events carry.

#ifndef CNC_RED_ALERT_TD_CORE_TARGET_H_
#define CNC_RED_ALERT_TD_CORE_TARGET_H_

#include <cstdint>
#include <type_traits>

#include "td/core/cell.h"
#include "td/core/coordinate.h"

namespace td {

class AircraftClass;
class BuildingClass;
class ObjectClass;
class TechnoClass;
class UnitClass;

// What a target refers to. The value is a target's high 4 bits, so a packed
// word can also hold 13..15, which name no kind.
enum class KindType {
  KIND_NONE,
  KIND_CELL,
  KIND_UNIT,
  KIND_INFANTRY,
  KIND_BUILDING,
  KIND_TERRAIN,
  KIND_AIRCRAFT,
  KIND_TEMPLATE,
  KIND_BULLET,
  KIND_ANIMATION,
  KIND_TRIGGER,
  KIND_TEAM,
  KIND_TEAMTYPE
};
using enum KindType;

// A reference to anything a unit can be sent to or ordered against: an object
// in one of the object heaps, a team or trigger, or a map cell. The 16-bit
// word holds the kind in its high 4 bits over a 12-bit value: the heap slot
// for objects, teams and triggers, the cell index for KIND_CELL.
//
// Target{} (word 0) is "no target"; test for it with `!target`.
//
// Unlike Red Alert's Target, Make() does not mask the value: a value of 4096
// or more, or a negative one, is ORed over the kind bits.
//
// Example:
//   const Target target = Target::FromCellIndex(cell);
//   if (target && target.kind() == KIND_CELL) {
//     const Coordinate center = target.AsCoordinate();
//   }
class Target {
 public:
  // Packs a kind and a value. The value is not masked to 12 bits: its bits
  // are ORed over the kind's, and the result keeps its low 16 bits, so
  // Make(KIND_UNIT, -1) is the word 0xFFFF and Make(KIND_UNIT, 4096) is
  // Make(KIND_INFANTRY, 0). Make(KIND_NONE, 0) is Target{}.
  static constexpr Target Make(KindType kind, int value) noexcept {
    return FromPacked(
        static_cast<uint16_t>((static_cast<unsigned>(kind) << kValueBits) |
                              static_cast<unsigned>(value)));
  }

  // Wraps a packed word, as saves, events and radio messages hold it.
  static constexpr Target FromPacked(uint16_t packed) noexcept {
    Target target{};
    target.word_ = packed;
    return target;
  }

  // A cell target: the cell index itself as the value, unmasked like
  // Make(), so Cell::None() is the word 0xFFFF.
  static constexpr Target FromCellIndex(Cell cell) noexcept {
    return Make(KIND_CELL, cell.index());
  }

  // Returns the packed word, as saved, sent and logged.
  [[nodiscard]] constexpr uint16_t packed() const noexcept { return word_; }

  // Returns the kind: the word's high 4 bits, which may be 13..15, naming no
  // kind, for a word from FromPacked() or from Make() with an oversized value.
  [[nodiscard]] constexpr KindType kind() const noexcept {
    return static_cast<KindType>(static_cast<unsigned>(word_) >> kValueBits);
  }

  // Returns the value, the word's low 12 bits: a heap index or a cell index
  // by kind.
  [[nodiscard]] constexpr int value() const noexcept {
    return static_cast<int>(static_cast<unsigned>(word_) & kValueMask);
  }

  // True unless this is Target{}, "no target".
  constexpr explicit operator bool() const noexcept { return word_ != 0; }

  // Targets are equal when their packed words are.
  friend constexpr bool operator==(Target, Target) noexcept = default;

  // Saves and loads the packed word, 2 bytes little-endian.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(word_);
  }

  // Returns the live infantry, unit, building, aircraft, terrain, bullet or
  // animation this target names, or nullptr for any other kind or an object
  // that is no longer active.
  [[nodiscard]] ObjectClass* AsObject() const;

  // AsObject() narrowed to a TechnoClass: nullptr unless the object is a
  // live techno.
  [[nodiscard]] TechnoClass* AsTechno() const;

  // Each returns the heap slot this target names when it is of that kind,
  // without looking at IsActive, or nullptr for any other kind.
  [[nodiscard]] AircraftClass* AsAircraft() const;
  [[nodiscard]] BuildingClass* AsBuilding() const;
  [[nodiscard]] UnitClass* AsUnit() const;

  // Returns where this target is: the center of the cell for a cell target,
  // the object's Target_Coord() for an object. Coordinate{} for no target, or
  // an object that is not live.
  [[nodiscard]] Coordinate AsCoordinate() const;

  // Returns the cell AsCoordinate() lies in.
  [[nodiscard]] Cell AsCell() const;

  // Returns where a unit moving to this target heads: the center of the cell
  // for a cell target, the object's Docking_Coord() for an object.
  // Coordinate{} for no target, or an object that is not live.
  [[nodiscard]] Coordinate AsMovementCoordinate() const;

 private:
  // The value fills the low 12 bits, the kind the 4 above.
  static constexpr unsigned kValueBits = 12;
  static constexpr unsigned kValueMask = (1U << kValueBits) - 1;

  uint16_t word_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Target) == 2);
static_assert(std::is_trivially_copyable_v<Target>);
static_assert(std::is_trivially_default_constructible_v<Target>);
static_assert(std::is_standard_layout_v<Target>);

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_TARGET_H_
