#include "td/serialize.h"

#include <concepts>

#include "td/aircraft.h"
#include "td/anim.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cargo.h"
#include "td/externs.h"
#include "td/foot.h"
#include "td/heap.h"
#include "td/infantry.h"
#include "td/object.h"
#include "td/target.h"
#include "td/teamtype.h"
#include "td/techno.h"
#include "td/terrain.h"
#include "td/type.h"
#include "td/unit.h"

namespace {

template <class T>
ObjectClass* Slot(TFixedIHeapClass<T>& heap, int index, ArchiveReader& ar) {
  if (index < 0 || index >= heap.Length()) {
    ar.Fail("saved object index outside its heap");
    return nullptr;
  }
  return heap.Raw_Ptr(index);
}

template <class T>
bool KindFits(KindType kind) {
  if constexpr (std::derived_from<T, FootClass>) {
    return kind == KIND_UNIT || kind == KIND_INFANTRY || kind == KIND_AIRCRAFT;
  } else if constexpr (std::derived_from<T, TechnoClass>) {
    return kind == KIND_UNIT || kind == KIND_INFANTRY ||
           kind == KIND_AIRCRAFT || kind == KIND_BUILDING;
  } else {
    return true;
  }
}

}  // namespace

ObjectClass* ResolveSavedObject(TARGET target, ArchiveReader& ar) {
  if (target == kTargetNone) {
    return nullptr;
  }
  const int index = static_cast<int>(Target_Value(target));
  switch (Target_Kind(target)) {
    case KIND_INFANTRY:
      return Slot(Infantry, index, ar);
    case KIND_UNIT:
      return Slot(Units, index, ar);
    case KIND_BUILDING:
      return Slot(Buildings, index, ar);
    case KIND_AIRCRAFT:
      return Slot(Aircraft, index, ar);
    case KIND_TERRAIN:
      return Slot(Terrains, index, ar);
    case KIND_BULLET:
      return Slot(Bullets, index, ar);
    case KIND_ANIMATION:
      return Slot(Anims, index, ar);
    default:
      ar.Fail("saved target is not an object kind");
      return nullptr;
  }
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveWriter& ar) {
  TARGET target =
      ref_ != nullptr && ref_->IsActive ? ref_->As_Target() : kTargetNone;
  ar(target);
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveReader& ar) {
  TARGET target = kTargetNone;
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == kTargetNone) {
    return;
  }
  if (!KindFits<T>(Target_Kind(target))) {
    ar.Fail("saved target has the wrong kind for its field");
    return;
  }
  ref_ = static_cast<T*>(ResolveSavedObject(target, ar));
}

template class ObjectPtr<ObjectClass>;
template class ObjectPtr<TechnoClass>;
template class ObjectPtr<FootClass>;

template <class Archive>
void CargoClass::Serialize(Archive& ar) {
  ar(Quantity, ObjectPtr(CargoHold));
}
template void CargoClass::Serialize(ArchiveWriter&);
template void CargoClass::Serialize(ArchiveReader&);

// Instantiate every production type-table binding while the heap migration
// is still pending, so their enum/member/resolver contracts are checked now.
#define INSTANTIATE_TYPE_PTR(Type)                        \
  template void TypePtr<Type>::Serialize(ArchiveWriter&); \
  template void TypePtr<Type>::Serialize(ArchiveReader&)
INSTANTIATE_TYPE_PTR(HouseTypeClass);
INSTANTIATE_TYPE_PTR(BuildingTypeClass);
INSTANTIATE_TYPE_PTR(UnitTypeClass);
INSTANTIATE_TYPE_PTR(InfantryTypeClass);
INSTANTIATE_TYPE_PTR(BulletTypeClass);
INSTANTIATE_TYPE_PTR(TerrainTypeClass);
INSTANTIATE_TYPE_PTR(TemplateTypeClass);
INSTANTIATE_TYPE_PTR(AnimTypeClass);
INSTANTIATE_TYPE_PTR(AircraftTypeClass);
INSTANTIATE_TYPE_PTR(OverlayTypeClass);
INSTANTIATE_TYPE_PTR(SmudgeTypeClass);
#undef INSTANTIATE_TYPE_PTR

template <class T>
void TeamTypePtr<T>::Serialize(ArchiveWriter& ar) {
  TARGET target =
      ref_ != nullptr && ref_->IsActive ? ref_->As_Target() : kTargetNone;
  ar(target);
}

template <class T>
void TeamTypePtr<T>::Serialize(ArchiveReader& ar) {
  TARGET target = kTargetNone;
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == kTargetNone) {
    return;
  }
  if (Target_Kind(target) != KIND_TEAMTYPE ||
      Target_Value(target) >= static_cast<unsigned>(TeamTypes.Length())) {
    ar.Fail("invalid saved team type target");
    return;
  }
  ref_ = TeamTypes.Raw_Ptr(static_cast<int>(Target_Value(target)));
}

template class TeamTypePtr<TeamTypeClass>;
template class TeamTypePtr<const TeamTypeClass>;

void HousePtr::Serialize(ArchiveWriter& ar) {
  int32_t index = ref_ == nullptr ? -1 : Houses.ID(ref_);
  ar(index);
}
void HousePtr::Serialize(ArchiveReader& ar) {
  int32_t index = -1;
  ar(index);
  ref_ = nullptr;
  if (!ar.ok() || index == -1) {
    return;
  }
  if (index < 0 || index >= Houses.Length()) {
    ar.Fail("invalid saved house slot");
    return;
  }
  ref_ = Houses.Raw_Ptr(index);
}

void TechnoTypePtr::Serialize(ArchiveWriter& ar) {
  TARGET target = kTargetNone;
  if (ref_ != nullptr) {
    switch (ref_->What_Am_I()) {
      case RTTI_INFANTRYTYPE:
        target = Build_Target(
            KIND_INFANTRY, dynamic_cast<const InfantryTypeClass*>(ref_)->Type);
        break;
      case RTTI_UNITTYPE:
        target = Build_Target(KIND_UNIT,
                              dynamic_cast<const UnitTypeClass*>(ref_)->Type);
        break;
      case RTTI_AIRCRAFTTYPE:
        target = Build_Target(
            KIND_AIRCRAFT, dynamic_cast<const AircraftTypeClass*>(ref_)->Type);
        break;
      case RTTI_BUILDINGTYPE:
        target = Build_Target(
            KIND_BUILDING, dynamic_cast<const BuildingTypeClass*>(ref_)->Type);
        break;
      default:
        break;
    }
  }
  ar(target);
}
void TechnoTypePtr::Serialize(ArchiveReader& ar) {
  TARGET target = kTargetNone;
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == kTargetNone) {
    return;
  }
  const auto index = Target_Value(target);
  switch (Target_Kind(target)) {
    case KIND_INFANTRY:
      if (index < INFANTRY_COUNT) {
        ref_ =
            &InfantryTypeClass::As_Reference(static_cast<InfantryType>(index));
        return;
      }
      break;
    case KIND_UNIT:
      if (index < UNIT_COUNT) {
        ref_ = &UnitTypeClass::As_Reference(static_cast<UnitType>(index));
        return;
      }
      break;
    case KIND_AIRCRAFT:
      if (index < AIRCRAFT_COUNT) {
        ref_ =
            &AircraftTypeClass::As_Reference(static_cast<AircraftType>(index));
        return;
      }
      break;
    case KIND_BUILDING:
      if (index < STRUCT_COUNT) {
        ref_ = &BuildingTypeClass::As_Reference(static_cast<StructType>(index));
        return;
      }
      break;
    default:
      break;
  }
  ar.Fail("invalid saved techno type target");
}
