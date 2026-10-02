// Explicit instantiations of ObjectHeap for every game object heap.
//
// Kept apart from game/core/object_heap.cc so the heap links without the game
// classes, which lets object_heap_test.cc exercise it with a local type.

#include "game/core/object_heap.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigtype.h"
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

// An explicit instantiation of a game:: template must sit in a namespace
// that encloses game::, so these are at global scope.
template class game::ObjectHeap<ra::AircraftClass>;
template class game::ObjectHeap<ra::AnimClass>;
template class game::ObjectHeap<ra::BuildingClass>;
template class game::ObjectHeap<ra::BulletClass>;
template class game::ObjectHeap<ra::FactoryClass>;
template class game::ObjectHeap<ra::HouseClass>;
template class game::ObjectHeap<ra::InfantryClass>;
template class game::ObjectHeap<ra::OverlayClass>;
template class game::ObjectHeap<ra::SmudgeClass>;
template class game::ObjectHeap<ra::TeamClass>;
template class game::ObjectHeap<ra::TemplateClass>;
template class game::ObjectHeap<ra::TerrainClass>;
template class game::ObjectHeap<ra::TriggerClass>;
template class game::ObjectHeap<ra::UnitClass>;
template class game::ObjectHeap<ra::VesselClass>;
template class game::ObjectHeap<ra::AircraftTypeClass>;
template class game::ObjectHeap<ra::AnimTypeClass>;
template class game::ObjectHeap<ra::BuildingTypeClass>;
template class game::ObjectHeap<ra::BulletTypeClass>;
template class game::ObjectHeap<ra::HouseTypeClass>;
template class game::ObjectHeap<ra::InfantryTypeClass>;
template class game::ObjectHeap<ra::OverlayTypeClass>;
template class game::ObjectHeap<ra::SmudgeTypeClass>;
template class game::ObjectHeap<ra::TeamTypeClass>;
template class game::ObjectHeap<ra::TemplateTypeClass>;
template class game::ObjectHeap<ra::TerrainTypeClass>;
template class game::ObjectHeap<ra::TriggerTypeClass>;
template class game::ObjectHeap<ra::UnitTypeClass>;
template class game::ObjectHeap<ra::VesselTypeClass>;
template class game::ObjectHeap<ra::WarheadTypeClass>;
template class game::ObjectHeap<ra::WeaponTypeClass>;
