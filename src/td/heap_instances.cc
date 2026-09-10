// Explicit heap instantiations, separate from the allocator for unit tests.

#include "td/aircraft.h"
#include "td/anim.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/factory.h"
#include "td/heap.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/overlay.h"
#include "td/smudge.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/trigger.h"
#include "td/unit.h"

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

template class TFixedIHeapClass<TeamTypeClass>;
