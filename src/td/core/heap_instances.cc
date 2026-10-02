// Explicit heap instantiations, separate from the allocator for unit tests.

#include "engine/stream/archive.h"
#include "game/core/object_heap.h"
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
template class game::ObjectHeap<td::AircraftClass>;
template class game::ObjectHeap<td::AnimClass>;
template class game::ObjectHeap<td::BuildingClass>;
template class game::ObjectHeap<td::BulletClass>;
template class game::ObjectHeap<td::FactoryClass>;
template class game::ObjectHeap<td::HouseClass>;
template class game::ObjectHeap<td::InfantryClass>;
template class game::ObjectHeap<td::OverlayClass>;
template class game::ObjectHeap<td::SmudgeClass>;
template class game::ObjectHeap<td::TeamClass>;
template class game::ObjectHeap<td::TemplateClass>;
template class game::ObjectHeap<td::TerrainClass>;
template class game::ObjectHeap<td::TriggerClass>;
template class game::ObjectHeap<td::UnitClass>;

template class game::ObjectHeap<td::TeamTypeClass>;
