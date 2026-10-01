// Explicit heap instantiations, separate from the allocator for unit tests.

#include "engine/stream/archive.h"
#include "game/core/heap.h"
#include "td/ai/team.h"
#include "td/ai/teamtype.h"
#include "td/ai/trigger.h"
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

}  // namespace td

// An explicit instantiation of a game:: template must sit in a namespace
// that encloses game::, so these are at global scope.
template class game::TFixedIHeapClass<td::AircraftClass>;
template class game::TFixedIHeapClass<td::AnimClass>;
template class game::TFixedIHeapClass<td::BuildingClass>;
template class game::TFixedIHeapClass<td::BulletClass>;
template class game::TFixedIHeapClass<td::FactoryClass>;
template class game::TFixedIHeapClass<td::HouseClass>;
template class game::TFixedIHeapClass<td::InfantryClass>;
template class game::TFixedIHeapClass<td::OverlayClass>;
template class game::TFixedIHeapClass<td::SmudgeClass>;
template class game::TFixedIHeapClass<td::TeamClass>;
template class game::TFixedIHeapClass<td::TemplateClass>;
template class game::TFixedIHeapClass<td::TerrainClass>;
template class game::TFixedIHeapClass<td::TriggerClass>;
template class game::TFixedIHeapClass<td::UnitClass>;

template class game::TFixedIHeapClass<td::TeamTypeClass>;
