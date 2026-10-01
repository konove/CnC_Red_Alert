// Explicit instantiations of TFixedIHeapClass for every game object heap.
//
// Kept apart from heap.cc so the heap itself links without the game classes,
// which lets heap_test.cc exercise it with a small local element type.

#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigtype.h"
#include "ra/core/heap.h"
#include "ra/objects/aircraft.h"
#include "ra/objects/anim.h"
#include "ra/objects/building.h"
#include "ra/objects/bullet.h"
#include "ra/objects/factory.h"
#include "ra/objects/house.h"
#include "ra/objects/infantry.h"
#include "ra/objects/overlay.h"
#include "ra/objects/smudge.h"
#include "ra/objects/template.h"
#include "ra/objects/terrain.h"
#include "ra/objects/type.h"
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"
#include "ra/objects/warhead.h"
#include "ra/objects/weapon.h"

namespace ra {

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

}  // namespace ra
