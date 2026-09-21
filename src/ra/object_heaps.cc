// How ObjectHeaps binds the CCPtr heap pointers.

#include "ra/object_heaps.h"

#include <string>
#include <utility>

#include "absl/strings/str_cat.h"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/ccptr.h"
#include "ra/factory.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/overlay.h"
#include "ra/smudge.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/unit.h"
#include "ra/vessel.h"

// The storage for each heap pointer. ObjectHeaps binds them; they are null
// outside its lifetime.
template <>
FixedIHeapClass* CCPtr<AircraftClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<AnimClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<BuildingClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<BulletClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<FactoryClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<HouseClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<InfantryClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<OverlayClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<SmudgeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TeamClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TeamTypeClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TemplateClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TerrainClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TriggerClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<UnitClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<VesselClass>::Heap = nullptr;
template <>
FixedIHeapClass* CCPtr<TriggerTypeClass>::Heap = nullptr;

ObjectHeaps::ObjectHeaps() {
  // A CCPtr<T> finds its object by index in the heap this points at, which
  // is how a saved game stores an object reference.
  CCPtr<AircraftClass>::BindHeap(&aircraft_);
  CCPtr<AnimClass>::BindHeap(&anim_);
  CCPtr<BuildingClass>::BindHeap(&building_);
  CCPtr<BulletClass>::BindHeap(&bullet_);
  CCPtr<FactoryClass>::BindHeap(&factory_);
  CCPtr<HouseClass>::BindHeap(&house_);
  CCPtr<InfantryClass>::BindHeap(&infantry_);
  CCPtr<OverlayClass>::BindHeap(&overlay_);
  CCPtr<SmudgeClass>::BindHeap(&smudge_);
  CCPtr<TeamClass>::BindHeap(&team_);
  CCPtr<TeamTypeClass>::BindHeap(&team_type_);
  CCPtr<TemplateClass>::BindHeap(&tmplate_);
  CCPtr<TerrainClass>::BindHeap(&terrain_);
  CCPtr<TriggerClass>::BindHeap(&trigger_);
  CCPtr<UnitClass>::BindHeap(&unit_);
  CCPtr<VesselClass>::BindHeap(&vessel_);
  CCPtr<TriggerTypeClass>::BindHeap(&trigger_type_);
}

ObjectHeaps::~ObjectHeaps() {
  CCPtr<AircraftClass>::BindHeap(nullptr);
  CCPtr<AnimClass>::BindHeap(nullptr);
  CCPtr<BuildingClass>::BindHeap(nullptr);
  CCPtr<BulletClass>::BindHeap(nullptr);
  CCPtr<FactoryClass>::BindHeap(nullptr);
  CCPtr<HouseClass>::BindHeap(nullptr);
  CCPtr<InfantryClass>::BindHeap(nullptr);
  CCPtr<OverlayClass>::BindHeap(nullptr);
  CCPtr<SmudgeClass>::BindHeap(nullptr);
  CCPtr<TeamClass>::BindHeap(nullptr);
  CCPtr<TeamTypeClass>::BindHeap(nullptr);
  CCPtr<TemplateClass>::BindHeap(nullptr);
  CCPtr<TerrainClass>::BindHeap(nullptr);
  CCPtr<TriggerClass>::BindHeap(nullptr);
  CCPtr<UnitClass>::BindHeap(nullptr);
  CCPtr<VesselClass>::BindHeap(nullptr);
  CCPtr<TriggerTypeClass>::BindHeap(nullptr);
}

std::string ObjectHeaps::Validate() const {
  // Listed in the order the save file walks them, so a report reads the same
  // way as a save game does.
  const std::pair<const char*, std::string> reports[] = {
      {"aircraft", aircraft_.Validate()},
      {"anim", anim_.Validate()},
      {"building", building_.Validate()},
      {"bullet", bullet_.Validate()},
      {"factory", factory_.Validate()},
      {"house", house_.Validate()},
      {"infantry", infantry_.Validate()},
      {"overlay", overlay_.Validate()},
      {"smudge", smudge_.Validate()},
      {"team", team_.Validate()},
      {"team_type", team_type_.Validate()},
      {"tmplate", tmplate_.Validate()},
      {"terrain", terrain_.Validate()},
      {"trigger", trigger_.Validate()},
      {"unit", unit_.Validate()},
      {"vessel", vessel_.Validate()},
      {"trigger_type", trigger_type_.Validate()},
  };

  for (const auto& [name, trouble] : reports) {
    if (!trouble.empty()) {
      return absl::StrCat(name, ": ", trouble);
    }
  }
  return {};
}
