// File: ObjectHeaps, the heaps that hold Tiberian Dawn's game objects.

#ifndef CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_
#define CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
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
  ~ObjectHeaps() { house_.Destroy_All(); }

  ObjectHeaps(const ObjectHeaps&) = delete;
  ObjectHeaps& operator=(const ObjectHeaps&) = delete;
  ObjectHeaps(ObjectHeaps&&) = delete;
  ObjectHeaps& operator=(ObjectHeaps&&) = delete;

  TFixedIHeapClass<UnitClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  TFixedIHeapClass<FactoryClass>& factory() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return factory_;
  }
  TFixedIHeapClass<TerrainClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  TFixedIHeapClass<TemplateClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return tmplate_;
  }
  TFixedIHeapClass<SmudgeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  TFixedIHeapClass<OverlayClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  TFixedIHeapClass<InfantryClass>& infantry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  TFixedIHeapClass<BulletClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  TFixedIHeapClass<BuildingClass>& building() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  TFixedIHeapClass<AnimClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  TFixedIHeapClass<AircraftClass>& aircraft() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  TFixedIHeapClass<TriggerClass>& trigger() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_;
  }
  TFixedIHeapClass<TeamTypeClass>& team_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_type_;
  }
  TFixedIHeapClass<TeamClass>& team() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_;
  }
  TFixedIHeapClass<HouseClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }

 private:
  TFixedIHeapClass<UnitClass> unit_;
  TFixedIHeapClass<FactoryClass> factory_;
  TFixedIHeapClass<TerrainClass> terrain_;
  TFixedIHeapClass<TemplateClass> tmplate_;
  TFixedIHeapClass<SmudgeClass> smudge_;
  TFixedIHeapClass<OverlayClass> overlay_;
  TFixedIHeapClass<InfantryClass> infantry_;
  TFixedIHeapClass<BulletClass> bullet_;
  TFixedIHeapClass<BuildingClass> building_;
  TFixedIHeapClass<AnimClass> anim_;
  TFixedIHeapClass<AircraftClass> aircraft_;
  TFixedIHeapClass<TriggerClass> trigger_;
  TFixedIHeapClass<TeamTypeClass> team_type_;
  TFixedIHeapClass<TeamClass> team_;
  TFixedIHeapClass<HouseClass> house_;
};

// Returns the ObjectHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline ObjectHeaps& TheObjectHeaps() {
  return base::Installed<ObjectHeaps>::Get();
}

#endif  // CNC_RED_ALERT_TD_CORE_OBJECT_HEAPS_H_
