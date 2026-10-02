// File: ObjectHeaps, the heaps that hold Tiberian Dawn's game objects.

#ifndef CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_
#define CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
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

// One heap per kind of object on the map, plus the team types that belong
// to the scenario. Everything here is part of the scenario and is saved:
// the save file walks each heap in turn. Game owns the one instance;
// everything else reaches it through TheObjectHeaps().
//
// Example:
//   for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) ...
class ObjectHeaps {
 public:
  ObjectHeaps() = default;
  // The heaps free their buffers without running element destructors, and
  // the houses own their unit trackers, so they are destroyed here. The units'
  // and buildings' destructors reach into the game through its globals, so
  // they stay unrun.
  ~ObjectHeaps() { house_.DestroyAll(); }

  ObjectHeaps(const ObjectHeaps&) = delete;
  ObjectHeaps& operator=(const ObjectHeaps&) = delete;
  ObjectHeaps(ObjectHeaps&&) = delete;
  ObjectHeaps& operator=(ObjectHeaps&&) = delete;

  game::ObjectHeap<UnitClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  game::ObjectHeap<FactoryClass>& factory() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return factory_;
  }
  game::ObjectHeap<TerrainClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  game::ObjectHeap<TemplateClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return tmplate_;
  }
  game::ObjectHeap<SmudgeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  game::ObjectHeap<OverlayClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  game::ObjectHeap<InfantryClass>& infantry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  game::ObjectHeap<BulletClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  game::ObjectHeap<BuildingClass>& building() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  game::ObjectHeap<AnimClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  game::ObjectHeap<AircraftClass>& aircraft() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  game::ObjectHeap<TriggerClass>& trigger() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_;
  }
  game::ObjectHeap<TeamTypeClass>& team_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_type_;
  }
  game::ObjectHeap<TeamClass>& team() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_;
  }
  game::ObjectHeap<HouseClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }

 private:
  game::ObjectHeap<UnitClass> unit_;
  game::ObjectHeap<FactoryClass> factory_;
  game::ObjectHeap<TerrainClass> terrain_;
  game::ObjectHeap<TemplateClass> tmplate_;
  game::ObjectHeap<SmudgeClass> smudge_;
  game::ObjectHeap<OverlayClass> overlay_;
  game::ObjectHeap<InfantryClass> infantry_;
  game::ObjectHeap<BulletClass> bullet_;
  game::ObjectHeap<BuildingClass> building_;
  game::ObjectHeap<AnimClass> anim_;
  game::ObjectHeap<AircraftClass> aircraft_;
  game::ObjectHeap<TriggerClass> trigger_;
  game::ObjectHeap<TeamTypeClass> team_type_;
  game::ObjectHeap<TeamClass> team_;
  game::ObjectHeap<HouseClass> house_;
};

// Returns the ObjectHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline ObjectHeaps& TheObjectHeaps() {
  return base::Installed<ObjectHeaps>::Get();
}

}  // namespace td

#endif  // CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_
