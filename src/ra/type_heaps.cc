// How TypeHeaps binds the CCPtr heap pointers and tears the heaps down.

#include "ra/type_heaps.h"

#include "ra/ccptr.h"
#include "ra/heap.h"
#include "ra/type.h"

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
  // TFixedIHeapClass frees its buffer without running element destructors,
  // so the owning members of every type object are released by hand. This
  // ran in Prog_End() before the heaps moved here.
  //
  // TODO: it does not actually return the memory. LeakSanitizer reports the
  // same 26 KB of DimensionData and RadarIcon storage whether this loop runs
  // or not, and the vectors' capacity is unchanged after the assignment, so
  // something about reaching them through the heap is wrong. The leak
  // predates this file; see docs/GLOBALS_PLAN.md.
  const auto release = [](auto& heap) {
    for (int i = 0; i < heap.Count(); i++) {
      ObjectTypeClass* const object_type = heap.Ptr(i);
      object_type->DimensionData = {};
      object_type->RadarIcon = {};
      object_type->ClearImage();
    }
  };
  release(aircraft_);
  release(anim_);
  release(building_);
  release(bullet_);
  release(infantry_);
  release(overlay_);
  release(smudge_);
  release(template_);
  release(terrain_);
  release(unit_);
  release(vessel_);

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
