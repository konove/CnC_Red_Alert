// Checked pointer proxies for TD's field-wise save format.

#ifndef CNC_RED_ALERT_TD_SERIALIZE_H_
#define CNC_RED_ALERT_TD_SERIALIZE_H_

#include <cstdint>

#include "td/defines.h"
#include "tech/archive.h"

class ObjectClass;
template <class T> class DynamicVectorClass;
// Ordered active object references, loaded only after all object heaps.
template <class Archive>
void SerializeObjectList(Archive& ar, DynamicVectorClass<ObjectClass*>& objects);

// Resolves an object TARGET by kind and heap bounds without dereferencing the
// slot. A later heap may not have constructed the referenced object yet.
ObjectClass* ResolveSavedObject(TARGET target, ArchiveReader& ar, bool active_only = false);

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

class TriggerClass;
// Trigger TARGETs refer to a fixed heap; validate the kind and slot bounds.
class TriggerPtr {
 public:
  explicit TriggerPtr(TriggerClass*& ref) : ref_(ref) {}
  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  TriggerClass*& ref_;
};

class TeamTypeClass;

// A team definition lives in a heap, rather than a static type table. Preserve
// its TARGET and validate kind/bounds before returning an unconstructed slot.
template <class T>
class TeamTypePtr {
 public:
  explicit TeamTypePtr(T*& ref) : ref_(ref) {}
  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  T*& ref_;
};

template <class T>
TeamTypePtr(T*&) -> TeamTypePtr<T>;

class HouseClass;
// House heap indices stay usable while HouseClass::Class is pointer-coded.
class HousePtr {
 public:
  explicit HousePtr(HouseClass*& ref) : ref_(ref) {}
  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  HouseClass*& ref_;
};

class TechnoTypeClass;
// A heterogeneous static type reference: kind plus checked table index.
class TechnoTypePtr {
 public:
  explicit TechnoTypePtr(const TechnoTypeClass*& ref) : ref_(ref) {}
  void Serialize(ArchiveWriter& ar);
  void Serialize(ArchiveReader& ar);

 private:
  const TechnoTypeClass*& ref_;
};

namespace td_save_detail {

// Each supported type enum indexes a static table, with no holes below COUNT.
constexpr int TypeCount(HousesType /*unused*/) { return HOUSE_COUNT; }
constexpr int TypeCount(StructType /*unused*/) { return STRUCT_COUNT; }
constexpr int TypeCount(UnitType /*unused*/) { return UNIT_COUNT; }
constexpr int TypeCount(InfantryType /*unused*/) { return INFANTRY_COUNT; }
constexpr int TypeCount(BulletType /*unused*/) { return BULLET_COUNT; }
constexpr int TypeCount(TerrainType /*unused*/) { return TERRAIN_COUNT; }
constexpr int TypeCount(TemplateType /*unused*/) { return TEMPLATE_COUNT; }
constexpr int TypeCount(AnimType /*unused*/) { return ANIM_COUNT; }
constexpr int TypeCount(AircraftType /*unused*/) { return AIRCRAFT_COUNT; }
constexpr int TypeCount(OverlayType /*unused*/) { return OVERLAY_COUNT; }
constexpr int TypeCount(SmudgeType /*unused*/) { return SMUDGE_COUNT; }

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

extern template void SerializeObjectList<ArchiveWriter>(ArchiveWriter&, DynamicVectorClass<ObjectClass*>&);
extern template void SerializeObjectList<ArchiveReader>(ArchiveReader&, DynamicVectorClass<ObjectClass*>&);

class TechnoClass;
class RadioClass;
class FootClass;

extern template class ObjectPtr<ObjectClass>;
extern template class ObjectPtr<TechnoClass>;
extern template class ObjectPtr<RadioClass>;
extern template class ObjectPtr<FootClass>;

class HouseTypeClass;
class BuildingTypeClass;
class UnitTypeClass;
class InfantryTypeClass;
class BulletTypeClass;
class TerrainTypeClass;
class TemplateTypeClass;
class AnimTypeClass;
class AircraftTypeClass;
class OverlayTypeClass;
class SmudgeTypeClass;
extern template class TeamTypePtr<TeamTypeClass>;
extern template class TeamTypePtr<const TeamTypeClass>;
extern template void TypePtr<HouseTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<HouseTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<BuildingTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<BuildingTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<UnitTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<UnitTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<InfantryTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<InfantryTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<BulletTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<BulletTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<TerrainTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<TerrainTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<TemplateTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<TemplateTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<AnimTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<AnimTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<AircraftTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<AircraftTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<OverlayTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<OverlayTypeClass>::Serialize(ArchiveReader&);
extern template void TypePtr<SmudgeTypeClass>::Serialize(ArchiveWriter&);
extern template void TypePtr<SmudgeTypeClass>::Serialize(ArchiveReader&);

#endif  // CNC_RED_ALERT_TD_SERIALIZE_H_
