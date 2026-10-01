// How TypeHeaps binds the CCPtr heap pointers and tears the heaps down.

#include "ra/core/type_heaps.h"

#include "ra/core/ccptr.h"
#include "ra/core/heap.h"
#include "ra/objects/type.h"

namespace ra {

// The storage for each type's heap pointer. TypeHeaps binds them; they are
// null outside its lifetime.
template <>
FixedIHeapClass* CCPtr<HouseTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<BuildingTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<AircraftTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<InfantryTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<BulletTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<AnimTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<UnitTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<VesselTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TemplateTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TerrainTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<OverlayTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<SmudgeTypeClass>::Heap = nullptr;

TypeHeaps::TypeHeaps() {
  // A CCPtr<T> finds its object by index in the heap this points at. There
  // is one heap per type for the life of the process, so binding them here
  // is what the static initializers in globals.cc used to do.
  CCPtr<HouseTypeClass>::BindHeap(&house_);
  CCPtr<BuildingTypeClass>::BindHeap(&building_);
  CCPtr<AircraftTypeClass>::BindHeap(&aircraft_);
  CCPtr<InfantryTypeClass>::BindHeap(&infantry_);
  CCPtr<BulletTypeClass>::BindHeap(&bullet_);
  CCPtr<AnimTypeClass>::BindHeap(&anim_);
  CCPtr<UnitTypeClass>::BindHeap(&unit_);
  CCPtr<VesselTypeClass>::BindHeap(&vessel_);
  CCPtr<TemplateTypeClass>::BindHeap(&template_);
  CCPtr<TerrainTypeClass>::BindHeap(&terrain_);
  CCPtr<OverlayTypeClass>::BindHeap(&overlay_);
  CCPtr<SmudgeTypeClass>::BindHeap(&smudge_);
}

TypeHeaps::~TypeHeaps() {
  // The heaps free their buffers without running element destructors, so
  // the type objects are destroyed here: they own their image dimensions and
  // radar icons.
  house_.Destroy_All();
  building_.Destroy_All();
  aircraft_.Destroy_All();
  infantry_.Destroy_All();
  bullet_.Destroy_All();
  anim_.Destroy_All();
  unit_.Destroy_All();
  vessel_.Destroy_All();
  template_.Destroy_All();
  terrain_.Destroy_All();
  overlay_.Destroy_All();
  smudge_.Destroy_All();

  CCPtr<HouseTypeClass>::BindHeap(nullptr);
  CCPtr<BuildingTypeClass>::BindHeap(nullptr);
  CCPtr<AircraftTypeClass>::BindHeap(nullptr);
  CCPtr<InfantryTypeClass>::BindHeap(nullptr);
  CCPtr<BulletTypeClass>::BindHeap(nullptr);
  CCPtr<AnimTypeClass>::BindHeap(nullptr);
  CCPtr<UnitTypeClass>::BindHeap(nullptr);
  CCPtr<VesselTypeClass>::BindHeap(nullptr);
  CCPtr<TemplateTypeClass>::BindHeap(nullptr);
  CCPtr<TerrainTypeClass>::BindHeap(nullptr);
  CCPtr<OverlayTypeClass>::BindHeap(nullptr);
  CCPtr<SmudgeTypeClass>::BindHeap(nullptr);
}

}  // namespace ra
