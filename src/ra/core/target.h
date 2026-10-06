// Red Alert's reference to a game object, a type, a cell or a spot on the map,
// packed into the one word that saves, network events and the multiplayer CRC
// carry.

#ifndef CNC_RED_ALERT_RA_CORE_TARGET_H_
#define CNC_RED_ALERT_RA_CORE_TARGET_H_

#include <cstdint>
#include <type_traits>

#include "ra/core/cell.h"
#include "ra/core/coordinate.h"
#include "ra/core/rtti.h"

namespace ra {

class AbstractClass;
class AircraftClass;
class AnimClass;
class BuildingClass;
class BulletClass;
class InfantryClass;
class ObjectClass;
class TeamClass;
class TeamType;
class TechnoClass;
class TechnoTypeClass;
class TriggerClass;
class TriggerType;
class UnitClass;
class VesselClass;

// A reference to anything the game can point at: an object in one of the
// object heaps, a type, a team or trigger, or a place on the map. The word
// holds the RTTI kind in its high 8 bits over a 24-bit value: the heap slot
// for objects, teams and triggers, the type index for types.
//
// Target{} (word 0) is "no target"; test for it with `!target`. is_valid()
// asks something else, whether the kind is not RTTI_NONE. The two disagree
// only for NoneWithAllValueBits(), which is true as a bool but not valid.
//
// RTTI_CELL has two value encodings, and nothing in the word says which one a
// target holds:
//   - the cell index, Cell::index() itself (FromCellIndex(); read by
//     cell_index() and AsMovementCoordinate());
//   - a sub-cell spot, 12 bits per axis in units of 16 leptons, Y over X
//     (FromCell(), FromCoordinate(), CellClass::As_Target(); read by
//     AsCoordinate() and AsCell()).
//
// The decoders come in two families. AsObject() and AsTechno() return only
// live objects; AsObjectInSlot() and AsTechnoInSlot() return whatever the
// heap slot holds, live or not, so the caller can tell a dead object from no
// object. The single-kind decoders (AsUnit(), AsTeam(), ...) return the slot.
//
// Example:
//   const Target target = Target::Of(unit);
//   if (target && target.kind() == RTTI_UNIT) {
//     UnitClass* same = target.AsUnit();
//   }
class Target {
 public:
  // Packs a kind and a value. The value keeps its low 24 bits, so -1 becomes
  // 0xFFFFFF. Make(RTTI_NONE, 0) is Target{}.
  static constexpr Target Make(RTTIType kind, int value) noexcept {
    return FromPacked(
        static_cast<int32_t>((static_cast<uint32_t>(kind) << kValueBits) |
                             (static_cast<uint32_t>(value) & kValueMask)));
  }

  // Wraps a packed word, as saves, scenario files and recordings hold it.
  static constexpr Target FromPacked(int32_t packed) noexcept {
    Target target{};
    target.word_ = packed;
    return target;
  }

  // Kind RTTI_NONE with every value bit set: what a default-constructed
  // target was before this class. It is not Target{}: it is true as a bool
  // but not is_valid().
  static constexpr Target NoneWithAllValueBits() noexcept {
    return Make(RTTI_NONE, -1);
  }

  // The cell index encoding: the cell number itself as the value, keeping
  // its low 24 bits, so cell -1 becomes 0xFFFFFF.
  static constexpr Target FromCellIndex(Cell cell) noexcept {
    return Make(RTTI_CELL, cell.index());
  }

  // The sub-cell encoding of a cell: the 16-lepton unit at its center.
  // FromCell(cell) == FromCoordinate(cell.Center()) for every cell.
  static constexpr Target FromCell(Cell cell) noexcept {
    return FromSubCell(
        (static_cast<uint32_t>(cell.x()) * kUnitsPerCell) + kHalfCellUnits,
        (static_cast<uint32_t>(cell.y()) * kUnitsPerCell) + kHalfCellUnits);
  }

  // The sub-cell encoding of a coordinate: the 16-lepton unit it lies in.
  static constexpr Target FromCoordinate(Coordinate coord) noexcept {
    return FromSubCell(static_cast<uint32_t>(coord.x()) / kLeptonsPerUnit,
                       static_cast<uint32_t>(coord.y()) / kLeptonsPerUnit);
  }

  // The object's kind and heap index, or Target{} for nullptr. Equal to
  // object->As_Target() otherwise.
  static Target Of(const AbstractClass* object);

  // Returns the packed word, as saved, sent, logged and CRC'd.
  [[nodiscard]] constexpr int32_t packed() const noexcept { return word_; }

  // Returns the kind of thing this target refers to.
  [[nodiscard]] constexpr RTTIType kind() const noexcept {
    return static_cast<RTTIType>(static_cast<uint32_t>(word_) >> kValueBits);
  }

  // Returns the value, in 0..0xFFFFFF: a heap index, type index or cell by
  // kind.
  [[nodiscard]] constexpr int value() const noexcept {
    return static_cast<int>(static_cast<uint32_t>(word_) & kValueMask);
  }

  // Reads the value as a cell index, the encoding FromCellIndex() writes:
  // its low 16 bits, so 0xFFFFFF reads back as cell -1.
  [[nodiscard]] constexpr Cell cell_index() const noexcept {
    return Cell::FromIndex(value());
  }

  // True when the kind is not RTTI_NONE. See the class comment for how this
  // differs from the bool conversion.
  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return kind() != RTTI_NONE;
  }

  // True when the kind is one of the map objects that can be attacked:
  // terrain, unit, vessel, infantry, building or aircraft.
  [[nodiscard]] constexpr bool is_object() const noexcept {
    const RTTIType k = kind();
    return k == RTTI_TERRAIN || k == RTTI_UNIT || k == RTTI_VESSEL ||
           k == RTTI_INFANTRY || k == RTTI_BUILDING || k == RTTI_AIRCRAFT;
  }

  // True unless this is Target{}, "no target".
  constexpr explicit operator bool() const noexcept { return word_ != 0; }

  // Targets are equal when their packed words are.
  friend constexpr bool operator==(Target, Target) noexcept = default;

  // Saves and loads the packed word, 4 bytes little-endian.
  template <class Archive>
  void Serialize(Archive& ar) {
    ar(word_);
  }

  // Returns the live infantry, unit, vessel, building, aircraft, terrain,
  // bullet or animation this target names, or nullptr for any other kind or
  // an object that is no longer active.
  [[nodiscard]] ObjectClass* AsObject() const;

  // AsObject() narrowed to a TechnoClass: nullptr unless the object is a
  // live techno.
  [[nodiscard]] TechnoClass* AsTechno() const;

  // Returns the heap slot of the terrain, smudge, overlay, bullet,
  // animation, unit, vessel, building, infantry or aircraft this target
  // names, whether or not it still holds an active object; nullptr for any
  // other kind. Unlike AsObject() this includes smudges and overlays and
  // does not look at IsActive.
  [[nodiscard]] ObjectClass* AsObjectInSlot() const;

  // Returns the heap slot of the unit, vessel, building, infantry or
  // aircraft this target names, whether or not it still holds an active
  // object; nullptr for any other kind.
  [[nodiscard]] TechnoClass* AsTechnoInSlot() const;

  // Each returns the heap slot this target names when it is of that kind,
  // without looking at IsActive, or nullptr for any other kind.
  [[nodiscard]] AircraftClass* AsAircraft() const;
  [[nodiscard]] AnimClass* AsAnimation() const;
  [[nodiscard]] BuildingClass* AsBuilding() const;
  [[nodiscard]] BulletClass* AsBullet() const;
  [[nodiscard]] InfantryClass* AsInfantry() const;
  [[nodiscard]] TeamClass* AsTeam() const;
  [[nodiscard]] TeamType* AsTeamType() const;
  [[nodiscard]] TriggerClass* AsTrigger() const;
  [[nodiscard]] TriggerType* AsTriggerType() const;
  [[nodiscard]] UnitClass* AsUnit() const;
  [[nodiscard]] VesselClass* AsVessel() const;

  // Returns the infantry, unit, vessel, aircraft or building type this
  // target names, or nullptr for any other kind.
  [[nodiscard]] const TechnoTypeClass* AsTechnoType() const;

  // Returns where this target is: for a cell target, the center of its
  // 16-lepton unit, reading the sub-cell encoding; for an object, its
  // Target_Coord(). Coordinate{} for no target, or an object that is not
  // live.
  [[nodiscard]] Coordinate AsCoordinate() const;

  // Returns the cell AsCoordinate() lies in.
  [[nodiscard]] Cell AsCell() const;

  // Returns where a unit moving to this target heads: for a cell target, the
  // center of the cell, reading the cell index encoding; for an object, its
  // Docking_Coord(). Coordinate{} for no target, or an object that is not
  // live.
  [[nodiscard]] Coordinate AsMovementCoordinate() const;

 private:
  // The value fills the low 24 bits, the kind the 8 above.
  static constexpr unsigned kValueBits = 24;
  static constexpr uint32_t kValueMask = (uint32_t{1} << kValueBits) - 1;

  // The sub-cell encoding: 12 bits per axis, Y above X, each counting units
  // of 16 leptons, 16 units to a cell.
  static constexpr unsigned kSubCellAxisBits = 12;
  static constexpr uint32_t kSubCellAxisMask =
      (uint32_t{1} << kSubCellAxisBits) - 1;
  static constexpr uint32_t kLeptonsPerUnit = 16;
  static constexpr uint32_t kUnitsPerCell = 16;
  static constexpr uint32_t kHalfCellUnits = kUnitsPerCell / 2;

  // Packs sub-cell X and Y units into an RTTI_CELL target. Neither is
  // masked: callers pass values that fit 12 bits.
  static constexpr Target FromSubCell(uint32_t x, uint32_t y) noexcept {
    return Make(RTTI_CELL, static_cast<int>((y << kSubCellAxisBits) | x));
  }

  int32_t word_;  // No initializer: EventClass's union needs a trivial type.
};

static_assert(sizeof(Target) == 4);
static_assert(std::is_trivially_copyable_v<Target>);
static_assert(std::is_trivially_default_constructible_v<Target>);
static_assert(std::is_standard_layout_v<Target>);

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_TARGET_H_
