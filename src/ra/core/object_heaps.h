// File: ObjectHeaps, the heaps that hold Red Alert's game objects.

#ifndef CNC_RED_ALERT_RA_CORE_OBJECT_HEAPS_H_
#define CNC_RED_ALERT_RA_CORE_OBJECT_HEAPS_H_

#include <string>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "game/core/object_heap.h"
#include "ra/ai/team.h"
#include "ra/ai/teamtype.h"
#include "ra/ai/trigger.h"
#include "ra/ai/trigger_type.h"
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
#include "ra/objects/unit.h"
#include "ra/objects/vessel.h"

namespace ra {

// One heap per kind of object on the map, plus the team and trigger types
// that belong to the scenario. Unlike TypeHeaps, everything here is part of
// the scenario and is saved: the save file walks each heap in turn.
//
// Game owns the one instance; everything else reaches it through
// TheObjectHeaps().
//
// Example:
//   for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) ...
class ObjectHeaps {
 public:
  ObjectHeaps() = default;
  ~ObjectHeaps();

  ObjectHeaps(const ObjectHeaps&) = delete;
  ObjectHeaps& operator=(const ObjectHeaps&) = delete;
  ObjectHeaps(ObjectHeaps&&) = delete;
  ObjectHeaps& operator=(ObjectHeaps&&) = delete;

  game::ObjectHeap<AircraftClass>& aircraft() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  game::ObjectHeap<AnimClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  game::ObjectHeap<BuildingClass>& building() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  game::ObjectHeap<BulletClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  game::ObjectHeap<FactoryClass>& factory() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return factory_;
  }
  game::ObjectHeap<HouseClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }
  game::ObjectHeap<InfantryClass>& infantry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  game::ObjectHeap<OverlayClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  game::ObjectHeap<SmudgeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  game::ObjectHeap<TeamClass>& team() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_;
  }
  game::ObjectHeap<TeamTypeClass>& team_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_type_;
  }
  game::ObjectHeap<TemplateClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return tmplate_;
  }
  game::ObjectHeap<TerrainClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  game::ObjectHeap<TriggerClass>& trigger() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_;
  }
  game::ObjectHeap<UnitClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  game::ObjectHeap<VesselClass>& vessel() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return vessel_;
  }
  game::ObjectHeap<TriggerType>& trigger_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_type_;
  }

  // Names the first heap that disagrees with itself and says how, or returns
  // "" when all of them are sound. -CHECKHEAPS calls this once a frame to
  // catch the frame corruption appears in, rather than the later frame whose
  // DCHECK_HEAP_SLOT happens to step on the damaged object.
  [[nodiscard]] std::string Validate() const;

 private:
  game::ObjectHeap<AircraftClass> aircraft_;
  game::ObjectHeap<AnimClass> anim_;
  game::ObjectHeap<BuildingClass> building_;
  game::ObjectHeap<BulletClass> bullet_;
  game::ObjectHeap<FactoryClass> factory_;
  game::ObjectHeap<HouseClass> house_;
  game::ObjectHeap<InfantryClass> infantry_;
  game::ObjectHeap<OverlayClass> overlay_;
  game::ObjectHeap<SmudgeClass> smudge_;
  game::ObjectHeap<TeamClass> team_;
  game::ObjectHeap<TeamTypeClass> team_type_;
  game::ObjectHeap<TemplateClass> tmplate_;
  game::ObjectHeap<TerrainClass> terrain_;
  game::ObjectHeap<TriggerClass> trigger_;
  game::ObjectHeap<UnitClass> unit_;
  game::ObjectHeap<VesselClass> vessel_;
  game::ObjectHeap<TriggerType> trigger_type_;
};

// Returns the ObjectHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline ObjectHeaps& TheObjectHeaps() {
  return base::Installed<ObjectHeaps>::Get();
}

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_OBJECT_HEAPS_H_
