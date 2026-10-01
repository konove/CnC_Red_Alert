// Explicit instantiations of TFixedIHeapClass for every game object heap.
//
// Kept apart from heap.cc so the heap itself links without the game classes,
// which lets heap_test.cc exercise it with a small local element type.

#include "game/core/heap.h"
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
template class game::TFixedIHeapClass<ra::AircraftClass>;
template class game::TFixedIHeapClass<ra::AnimClass>;
template class game::TFixedIHeapClass<ra::BuildingClass>;
template class game::TFixedIHeapClass<ra::BulletClass>;
template class game::TFixedIHeapClass<ra::FactoryClass>;
template class game::TFixedIHeapClass<ra::HouseClass>;
template class game::TFixedIHeapClass<ra::InfantryClass>;
template class game::TFixedIHeapClass<ra::OverlayClass>;
template class game::TFixedIHeapClass<ra::SmudgeClass>;
template class game::TFixedIHeapClass<ra::TeamClass>;
template class game::TFixedIHeapClass<ra::TemplateClass>;
template class game::TFixedIHeapClass<ra::TerrainClass>;
template class game::TFixedIHeapClass<ra::TriggerClass>;
template class game::TFixedIHeapClass<ra::UnitClass>;
template class game::TFixedIHeapClass<ra::VesselClass>;
template class game::TFixedIHeapClass<ra::AircraftTypeClass>;
template class game::TFixedIHeapClass<ra::AnimTypeClass>;
template class game::TFixedIHeapClass<ra::BuildingTypeClass>;
template class game::TFixedIHeapClass<ra::BulletTypeClass>;
template class game::TFixedIHeapClass<ra::HouseTypeClass>;
template class game::TFixedIHeapClass<ra::InfantryTypeClass>;
template class game::TFixedIHeapClass<ra::OverlayTypeClass>;
template class game::TFixedIHeapClass<ra::SmudgeTypeClass>;
template class game::TFixedIHeapClass<ra::TeamTypeClass>;
template class game::TFixedIHeapClass<ra::TemplateTypeClass>;
template class game::TFixedIHeapClass<ra::TerrainTypeClass>;
template class game::TFixedIHeapClass<ra::TriggerTypeClass>;
template class game::TFixedIHeapClass<ra::UnitTypeClass>;
template class game::TFixedIHeapClass<ra::VesselTypeClass>;
template class game::TFixedIHeapClass<ra::WarheadTypeClass>;
template class game::TFixedIHeapClass<ra::WeaponTypeClass>;
