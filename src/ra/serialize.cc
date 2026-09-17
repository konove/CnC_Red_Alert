#include "ra/serialize.h"

#include <concepts>
#include <cstdint>

#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/defines.h"
#include "ra/externs.h"
#include "ra/foot.h"
#include "ra/heap.h"
#include "ra/infantry.h"
#include "ra/object.h"
#include "ra/overlay.h"
#include "ra/radio.h"
#include "ra/smudge.h"
#include "ra/target.h"
#include "ra/techno.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vector_dynamic.h"
#include "ra/vessel.h"
#include "tech/archive.h"

namespace {

// Returns the slot for `index` in `heap`, or nullptr with an error recorded
// when the index is outside it. Deliberately does not consult the slot's
// IsActive flag: the object there may not have been loaded yet.
template <class T>
ObjectClass* Slot(TFixedIHeapClass<T>& heap, int index, ArchiveReader& ar) {
  if (index < 0 || index >= heap.Length()) {
    ar.Fail("saved object index outside its heap");
    return nullptr;
  }
  return heap.Raw_Ptr(index);
}

bool IsTechnoKind(RTTIType kind) {
  return kind == RTTI_BUILDING || kind == RTTI_UNIT || kind == RTTI_INFANTRY ||
         kind == RTTI_VESSEL || kind == RTTI_AIRCRAFT;
}

bool IsFootKind(RTTIType kind) {
  return kind == RTTI_UNIT || kind == RTTI_INFANTRY || kind == RTTI_VESSEL ||
         kind == RTTI_AIRCRAFT;
}

// Whether a TARGET of `kind` may be stored in a T*. Mirrors the RTTI tests
// ObjectClass::Is_Techno and Is_Foot make on live objects.
template <class T>
bool KindFits(RTTIType kind) {
  if constexpr (std::derived_from<T, FootClass>) {
    return IsFootKind(kind);
  } else if constexpr (std::derived_from<T, TechnoClass>) {
    return IsTechnoKind(kind);
  } else {
    return true;
  }
}

}  // namespace

ObjectClass* ResolveSavedObject(int32_t target, ArchiveReader& ar) {
  if (target == kTargetNone) {
    return nullptr;
  }
  const int index = Target_Value(target);
  switch (Target_Kind(target)) {
    case RTTI_INFANTRY:
      return Slot(Infantry, index, ar);
    case RTTI_UNIT:
      return Slot(Units, index, ar);
    case RTTI_VESSEL:
      return Slot(Vessels, index, ar);
    case RTTI_BUILDING:
      return Slot(Buildings, index, ar);
    case RTTI_AIRCRAFT:
      return Slot(Aircraft, index, ar);
    case RTTI_TERRAIN:
      return Slot(Terrains, index, ar);
    case RTTI_BULLET:
      return Slot(Bullets, index, ar);
    case RTTI_ANIM:
      return Slot(Anims, index, ar);
    case RTTI_OVERLAY:
      return Slot(Overlays, index, ar);
    case RTTI_SMUDGE:
      return Slot(Smudges, index, ar);
    case RTTI_TEMPLATE:
      return Slot(Templates, index, ar);
    case RTTIType::RTTI_NONE:
    case RTTIType::RTTI_AIRCRAFTTYPE:
    case RTTIType::RTTI_ANIMTYPE:
    case RTTIType::RTTI_BUILDINGTYPE:
    case RTTIType::RTTI_BULLETTYPE:
    case RTTIType::RTTI_CELL:
    case RTTIType::RTTI_FACTORY:
    case RTTIType::RTTI_HOUSE:
    case RTTIType::RTTI_HOUSETYPE:
    case RTTIType::RTTI_INFANTRYTYPE:
    case RTTIType::RTTI_OVERLAYTYPE:
    case RTTIType::RTTI_SMUDGETYPE:
    case RTTIType::RTTI_SPECIAL:
    case RTTIType::RTTI_TEAM:
    case RTTIType::RTTI_TEAMTYPE:
    case RTTIType::RTTI_TEMPLATETYPE:
    case RTTIType::RTTI_TERRAINTYPE:
    case RTTIType::RTTI_TRIGGER:
    case RTTIType::RTTI_TRIGGERTYPE:
    case RTTIType::RTTI_UNITTYPE:
    case RTTIType::RTTI_VESSELTYPE:
    default:
      ar.Fail("saved object target is not an object kind");
      return nullptr;
  }
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveWriter& ar) {
  TARGET target = kTargetNone;
  if (ref_ != nullptr && ref_->IsActive) {
    target = ref_->As_Target();
  }
  ar(target);
}

template <class T>
void ObjectPtr<T>::Serialize(ArchiveReader& ar) {
  TARGET target = kTargetNone;
  ar(target);
  ref_ = nullptr;
  if (target == kTargetNone) {
    return;
  }
  if (!KindFits<T>(Target_Kind(target))) {
    ar.Fail("saved object target has the wrong kind for its field");
    return;
  }
  // The kind check above makes the downcast sound: the slot holds (or will
  // hold once its heap loads) an object of a type derived from T.
  ref_ = static_cast<T*>(ResolveSavedObject(target, ar));
}

template class ObjectPtr<ObjectClass>;
template class ObjectPtr<RadioClass>;
template class ObjectPtr<TechnoClass>;
template class ObjectPtr<FootClass>;

void TechnoTypePtr::Serialize(ArchiveWriter& ar) {
  TARGET target = ref_ != nullptr ? ref_->As_Target() : kTargetNone;
  ar(target);
}

void TechnoTypePtr::Serialize(ArchiveReader& ar) {
  TARGET target = kTargetNone;
  ar(target);
  ref_ = nullptr;
  if (target == kTargetNone) {
    return;
  }
  ref_ = As_TechnoType(target);
  if (ref_ == nullptr) {
    ar.Fail("saved techno type target does not name a type");
  }
}

template <class Archive>
void SerializeObjectList(Archive& ar, DynamicVectorClass<ObjectClass*>& objects) {
  auto count = static_cast<int32_t>(objects.Count());
  ar(count);
  if constexpr (Archive::kIsReading) {
    const int capacity = Aircraft.Length() + Anims.Length() + Buildings.Length() +
        Bullets.Length() + Infantry.Length() + Overlays.Length() + Smudges.Length() +
        Templates.Length() + Terrains.Length() + Units.Length() + Vessels.Length();
    if (!ar.ok() || count < 0 || count > capacity) {
      ar.Fail("invalid saved object list count");
      return;
    }
    objects.Clear();
    for (int i = 0; i < count; ++i) {
      ObjectClass* object = nullptr;
      ar(ObjectPtr(object));
      if (!ar.ok() || object == nullptr) {
        ar.Fail("invalid saved object list entry");
        return;
      }
      objects.Add(object);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      ar(ObjectPtr(objects.at(i)));
    }
  }
}
template void SerializeObjectList(ArchiveWriter&, DynamicVectorClass<ObjectClass*>&);
template void SerializeObjectList(ArchiveReader&, DynamicVectorClass<ObjectClass*>&);
