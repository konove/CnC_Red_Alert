#include "td/saveload/serialize.h"

#include <bitset>
#include <concepts>
#include <cstdint>
#include <vector>

#include "base/array.h"
#include "base/numeric.h"
#include "game/core/object_heap.h"
#include "td/ai/teamtype.h"
#include "td/ai/trigger.h"
#include "td/core/defines.h"
#include "td/core/object_heaps.h"
#include "td/objects/aircraft.h"
#include "td/objects/anim.h"
#include "td/objects/building.h"
#include "td/objects/bullet.h"
#include "td/objects/cargo.h"
#include "td/objects/foot.h"
#include "td/objects/infantry.h"
#include "td/objects/object.h"
#include "td/objects/techno.h"
#include "td/objects/template.h"
#include "td/objects/terrain.h"
#include "td/objects/type.h"
#include "td/objects/unit.h"

namespace td {

namespace {

template <class T>
ObjectClass* Slot(game::ObjectHeap<T>& heap, int index, ArchiveReader& ar,
                  bool active_only) {
  if (index < 0 || index >= heap.Capacity()) {
    ar.Fail("saved object index outside its heap");
    return nullptr;
  }
  if (active_only && !heap.IsAllocated(index)) {
    ar.Fail("saved object list references an unallocated slot");
    return nullptr;
  }
  return heap.Slot(index);
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

ObjectClass* ResolveSavedObject(Target target, ArchiveReader& ar,
                                bool active_only) {
  if (target == Target{}) {
    return nullptr;
  }
  const int index = target.value();
  switch (target.kind()) {
    case KIND_INFANTRY:
      return Slot(TheObjectHeaps().infantry(), index, ar, active_only);
    case KIND_UNIT:
      return Slot(TheObjectHeaps().unit(), index, ar, active_only);
    case KIND_BUILDING:
      return Slot(TheObjectHeaps().building(), index, ar, active_only);
    case KIND_AIRCRAFT:
      return Slot(TheObjectHeaps().aircraft(), index, ar, active_only);
    case KIND_TERRAIN:
      return Slot(TheObjectHeaps().terrain(), index, ar, active_only);
    case KIND_BULLET:
      return Slot(TheObjectHeaps().bullet(), index, ar, active_only);
    case KIND_ANIMATION:
      return Slot(TheObjectHeaps().anim(), index, ar, active_only);
    case KIND_TEMPLATE:
      return Slot(TheObjectHeaps().tmplate(), index, ar, active_only);
    case KindType::KIND_NONE:
    case KindType::KIND_CELL:
    case KindType::KIND_TRIGGER:
    case KindType::KIND_TEAM:
    case KindType::KIND_TEAMTYPE:
    default:
      ar.Fail("saved target is not an object kind");
      return nullptr;
  }
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveWriter& ar) {
  Target target =
      ref_ != nullptr && ref_->IsActive ? ref_->As_Target() : Target{};
  ar(target);
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveReader& ar) {
  Target target = Target{};
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == Target{}) {
    return;
  }
  if (!KindFits<T>(target.kind())) {
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
  Target target =
      ref_ != nullptr && ref_->IsActive ? ref_->As_Target() : Target{};
  ar(target);
}

template <class T>
void TeamTypePtr<T>::Serialize(ArchiveReader& ar) {
  Target target = Target{};
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == Target{}) {
    return;
  }
  if (target.kind() != KIND_TEAMTYPE ||
      target.value() >= TheObjectHeaps().team_type().Capacity()) {
    ar.Fail("invalid saved team type target");
    return;
  }
  ref_ = TheObjectHeaps().team_type().Slot(target.value());
}

template class TeamTypePtr<TeamTypeClass>;
template class TeamTypePtr<const TeamTypeClass>;

void HousePtr::Serialize(ArchiveWriter& ar) {
  int32_t index =
      ref_ == nullptr ? -1 : TheObjectHeaps().house().SlotIndexOf(ref_);
  ar(index);
}
void HousePtr::Serialize(ArchiveReader& ar) {
  int32_t index = -1;
  ar(index);
  ref_ = nullptr;
  if (!ar.ok() || index == -1) {
    return;
  }
  if (index < 0 || index >= TheObjectHeaps().house().Capacity()) {
    ar.Fail("invalid saved house slot");
    return;
  }
  ref_ = TheObjectHeaps().house().Slot(index);
}

void TechnoTypePtr::Serialize(ArchiveWriter& ar) {
  Target target = Target{};
  if (ref_ != nullptr) {
    switch (ref_->What_Am_I()) {
      case RTTI_INFANTRYTYPE:
        target = Target::Make(
            KIND_INFANTRY,
            static_cast<int>(
                dynamic_cast<const InfantryTypeClass*>(ref_)->Type));
        break;
      case RTTI_UNITTYPE:
        target = Target::Make(
            KIND_UNIT,
            static_cast<int>(dynamic_cast<const UnitTypeClass*>(ref_)->Type));
        break;
      case RTTI_AIRCRAFTTYPE:
        target = Target::Make(
            KIND_AIRCRAFT,
            static_cast<int>(
                dynamic_cast<const AircraftTypeClass*>(ref_)->Type));
        break;
      case RTTI_BUILDINGTYPE:
        target = Target::Make(
            KIND_BUILDING,
            static_cast<int>(
                dynamic_cast<const BuildingTypeClass*>(ref_)->Type));
        break;
      case RTTIType::RTTI_NONE:
      case RTTIType::RTTI_INFANTRY:
      case RTTIType::RTTI_UNIT:
      case RTTIType::RTTI_AIRCRAFT:
      case RTTIType::RTTI_BUILDING:
      case RTTIType::RTTI_TERRAIN:
      case RTTIType::RTTI_ABSTRACTTYPE:
      case RTTIType::RTTI_ANIM:
      case RTTIType::RTTI_ANIMTYPE:
      case RTTIType::RTTI_BULLET:
      case RTTIType::RTTI_BULLETTYPE:
      case RTTIType::RTTI_OVERLAY:
      case RTTIType::RTTI_OVERLAYTYPE:
      case RTTIType::RTTI_SMUDGE:
      case RTTIType::RTTI_SMUDGETYPE:
      case RTTIType::RTTI_TEAM:
      case RTTIType::RTTI_TEMPLATE:
      case RTTIType::RTTI_TEMPLATETYPE:
      case RTTIType::RTTI_TERRAINTYPE:
      case RTTIType::RTTI_OBJECT:
      case RTTIType::RTTI_SPECIAL:
      default:
        break;
    }
  }
  ar(target);
}
void TechnoTypePtr::Serialize(ArchiveReader& ar) {
  Target target = Target{};
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == Target{}) {
    return;
  }
  const auto index = target.value();
  switch (target.kind()) {
    case KIND_INFANTRY:
      if (index < kInfantryCount) {
        ref_ =
            &InfantryTypeClass::As_Reference(static_cast<InfantryType>(index));
        return;
      }
      break;
    case KIND_UNIT:
      if (index < kUnitCount) {
        ref_ = &UnitTypeClass::As_Reference(static_cast<UnitType>(index));
        return;
      }
      break;
    case KIND_AIRCRAFT:
      if (index < kAircraftCount) {
        ref_ =
            &AircraftTypeClass::As_Reference(static_cast<AircraftType>(index));
        return;
      }
      break;
    case KIND_BUILDING:
      if (index < kStructCount) {
        ref_ = &BuildingTypeClass::As_Reference(static_cast<StructType>(index));
        return;
      }
      break;
    case KindType::KIND_NONE:
    case KindType::KIND_CELL:
    case KindType::KIND_TERRAIN:
    case KindType::KIND_TEMPLATE:
    case KindType::KIND_BULLET:
    case KindType::KIND_ANIMATION:
    case KindType::KIND_TRIGGER:
    case KindType::KIND_TEAM:
    case KindType::KIND_TEAMTYPE:
    default:
      break;
  }
  ar.Fail("invalid saved techno type target");
}

void TriggerPtr::Serialize(ArchiveWriter& ar) {
  Target target =
      ref_ != nullptr && ref_->IsActive ? ref_->As_Target() : Target{};
  ar(target);
}
void TriggerPtr::Serialize(ArchiveReader& ar) {
  Target target = Target{};
  ar(target);
  ref_ = nullptr;
  if (!ar.ok() || target == Target{}) {
    return;
  }
  if (target.kind() != KIND_TRIGGER ||
      target.value() >= TheObjectHeaps().trigger().Capacity()) {
    ar.Fail("invalid saved trigger target");
    return;
  }
  ref_ = TheObjectHeaps().trigger().Slot(target.value());
}

template <class Archive>
void SerializeObjectList(Archive& ar, std::vector<ObjectClass*>& objects) {
  auto count = static_cast<int32_t>(objects.size());
  ar(count);
  if constexpr (Archive::kIsReading) {
    if (!ar.ok() || count < 0 || count > 65536) {
      ar.Fail("invalid saved object list count");
      return;
    }
    objects.clear();
    objects.reserve(base::ToSize(count));
  }
  // set() runs only in the reading instantiation; misc-const-correctness
  // checks the writer, which discards that `if constexpr` branch.
  // NOLINTNEXTLINE(misc-const-correctness)
  std::bitset<65536> seen;
  for (int32_t i = 0; i < count; ++i) {
    ObjectClass* object = nullptr;
    Target target = Target{};
    if constexpr (!Archive::kIsReading) {
      object = base::At(objects, i);
      if (object != nullptr && object->IsActive) {
        target = object->As_Target();
      }
    }
    ar(target);
    if constexpr (Archive::kIsReading) {
      if (!ar.ok()) {
        return;
      }
      object = ResolveSavedObject(target, ar, true);
      if (!ar.ok() || object == nullptr || !object->IsActive) {
        ar.Fail("invalid saved object list reference");
        return;
      }
      if (seen.test(target.packed())) {
        ar.Fail("duplicate saved object list reference");
        return;
      }
      seen.set(target.packed());
      objects.push_back(object);
    }
  }
}
template void SerializeObjectList(ArchiveWriter&, std::vector<ObjectClass*>&);
template void SerializeObjectList(ArchiveReader&, std::vector<ObjectClass*>&);

}  // namespace td
