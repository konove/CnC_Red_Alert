// Checked pointer proxies for TD's field-wise save format.

#ifndef CNC_RED_ALERT_TD_SERIALIZE_H_
#define CNC_RED_ALERT_TD_SERIALIZE_H_

#include <cstdint>

#include "td/defines.h"
#include "tech/archive.h"

class ObjectClass;

// Resolves an object TARGET by kind and heap bounds without dereferencing the
// slot. A later heap may not have constructed the referenced object yet.
ObjectClass* ResolveSavedObject(TARGET target, ArchiveReader& ar);

// Saves active objects as TARGETs, null/inactive objects as kTargetNone.
// Readers reject kinds that cannot be stored in T*. Defined for ObjectClass,
// TechnoClass and FootClass; the reference must outlive the proxy.
template <class T>
class ObjectPtr {
 public:
  explicit ObjectPtr(T*& ref) : ref_(ref) {}
  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  T*& ref_;
};

template <class T>
ObjectPtr(T*&) -> ObjectPtr<T>;

namespace td_save_detail {

// Each supported type enum indexes a static table, with no holes below COUNT.
constexpr int TypeCount(HousesType) { return HOUSE_COUNT; }
constexpr int TypeCount(StructType) { return STRUCT_COUNT; }
constexpr int TypeCount(UnitType) { return UNIT_COUNT; }
constexpr int TypeCount(InfantryType) { return INFANTRY_COUNT; }
constexpr int TypeCount(BulletType) { return BULLET_COUNT; }
constexpr int TypeCount(TerrainType) { return TERRAIN_COUNT; }
constexpr int TypeCount(TemplateType) { return TEMPLATE_COUNT; }
constexpr int TypeCount(AnimType) { return ANIM_COUNT; }
constexpr int TypeCount(AircraftType) { return AIRCRAFT_COUNT; }
constexpr int TypeCount(OverlayType) { return OVERLAY_COUNT; }
constexpr int TypeCount(SmudgeType) { return SMUDGE_COUNT; }

}  // namespace td_save_detail

// Saves a concrete type pointer as its enum ID (int32, -1 for null). Valid IDs
// resolve through T::As_Reference only after bounds checks. T exposes Type, or
// House for HouseTypeClass; its static table must exist before loading.
// Example: ar(TypePtr(Class));
template <class T>
class TypePtr {
 public:
  explicit TypePtr(const T*& ref) : ref_(ref) {}

  template <class Archive>
  void Serialize(Archive& ar) {
    using Enum = decltype(Id(*ref_));
    int32_t id = -1;
    if constexpr (!Archive::kIsReading) {
      if (ref_ != nullptr) {
        id = static_cast<int32_t>(Id(*ref_));
      }
    }
    ar(id);
    if constexpr (Archive::kIsReading) {
      ref_ = nullptr;
      if (!ar.ok() || id == -1) {
        return;
      }
      if (id < 0 || id >= td_save_detail::TypeCount(Enum{})) {
        ar.Fail("saved type ID outside its table");
        return;
      }
      ref_ = &T::As_Reference(static_cast<Enum>(id));
    }
  }

 private:
  static auto Id(const T& value) {
    if constexpr (requires { value.Type; }) {
      return value.Type;
    } else {
      return value.House;
    }
  }

  const T*& ref_;
};

template <class T>
TypePtr(const T*&) -> TypePtr<T>;

#endif  // CNC_RED_ALERT_TD_SERIALIZE_H_
