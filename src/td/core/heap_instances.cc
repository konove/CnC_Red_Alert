// Explicit heap instantiations, separate from the allocator for unit tests.

#include "engine/stream/archive.h"
#include "td/ai/team.h"
#include "td/ai/teamtype.h"
#include "td/ai/trigger.h"
#include "td/core/heap.h"
#include "td/objects/aircraft.h"
#include "td/objects/anim.h"
#include "td/objects/building.h"
#include "td/objects/bullet.h"
#include "td/objects/factory.h"
#include "td/objects/house.h"
#include "td/objects/infantry.h"
#include "td/objects/overlay.h"
#include "td/objects/smudge.h"
#include "td/objects/template.h"
#include "td/objects/terrain.h"
#include "td/objects/unit.h"

namespace td {

// Every object heap now declares its own field-wise serializer.
static_assert(Serializable<AircraftClass>);
static_assert(Serializable<BuildingClass>);
static_assert(Serializable<InfantryClass>);
static_assert(Serializable<UnitClass>);
static_assert(Serializable<AnimClass>);
static_assert(Serializable<BulletClass>);
static_assert(Serializable<TerrainClass>);
static_assert(Serializable<TemplateClass>);
static_assert(Serializable<OverlayClass>);
static_assert(Serializable<SmudgeClass>);

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

}  // namespace td
