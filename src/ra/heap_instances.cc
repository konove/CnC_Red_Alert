// Explicit instantiations of TFixedIHeapClass for every game object heap.
//
// Kept apart from heap.cc so the heap itself links without the game classes,
// which lets heap_test.cc exercise it with a small local element type.

#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
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
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vessel.h"
#include "ra/warhead.h"
#include "ra/weapon.h"

template class TFixedIHeapClass<AircraftClass>;
template class TFixedIHeapClass<AnimClass>;
template class TFixedIHeapClass<BuildingClass>;
template class TFixedIHeapClass<BulletClass>;
template class TFixedIHeapClass<FactoryClass>;
template class TFixedIHeapClass<HouseClass>;
template class TFixedIHeapClass<InfantryClass>;
template class TFixedIHeapClass<OverlayClass>;
template class TFixedIHeapClass<SmudgeClass>;
template class TFixedIHeapClass<TeamClass>;
template class TFixedIHeapClass<TemplateClass>;
template class TFixedIHeapClass<TerrainClass>;
template class TFixedIHeapClass<TriggerClass>;
template class TFixedIHeapClass<UnitClass>;
template class TFixedIHeapClass<VesselClass>;
template class TFixedIHeapClass<AircraftTypeClass>;
template class TFixedIHeapClass<AnimTypeClass>;
template class TFixedIHeapClass<BuildingTypeClass>;
template class TFixedIHeapClass<BulletTypeClass>;
template class TFixedIHeapClass<HouseTypeClass>;
template class TFixedIHeapClass<InfantryTypeClass>;
template class TFixedIHeapClass<OverlayTypeClass>;
template class TFixedIHeapClass<SmudgeTypeClass>;
template class TFixedIHeapClass<TeamTypeClass>;
template class TFixedIHeapClass<TemplateTypeClass>;
template class TFixedIHeapClass<TerrainTypeClass>;
template class TFixedIHeapClass<TriggerTypeClass>;
template class TFixedIHeapClass<UnitTypeClass>;
template class TFixedIHeapClass<VesselTypeClass>;
template class TFixedIHeapClass<WarheadTypeClass>;
template class TFixedIHeapClass<WeaponTypeClass>;
