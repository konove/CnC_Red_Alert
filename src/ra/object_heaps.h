// File: ObjectHeaps, the heaps that hold Red Alert's game objects.

#ifndef CNC_RED_ALERT_RA_OBJECT_HEAPS_H_
#define CNC_RED_ALERT_RA_OBJECT_HEAPS_H_

#include <string>

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/factory.h"
#include "ra/heap.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/overlay.h"
#include "ra/smudge.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/unit.h"
#include "ra/vessel.h"

// One heap per kind of object on the map, plus the team and trigger types
// that belong to the scenario. Unlike TypeHeaps, everything here is part of
// the scenario and is saved: the save file walks each heap in turn.
//
// Constructing one binds the matching CCPtr<T>::Heap pointers, so a CCPtr to
// a game object only resolves while an ObjectHeaps is installed. Game owns
// the one instance; everything else reaches it through TheObjectHeaps().
//
// Example:
//   for (int i = 0; i < TheObjectHeaps().unit().Count(); i++) ...
class ObjectHeaps {
 public:
  ObjectHeaps();
  ~ObjectHeaps();

  ObjectHeaps(const ObjectHeaps&) = delete;
  ObjectHeaps& operator=(const ObjectHeaps&) = delete;
  ObjectHeaps(ObjectHeaps&&) = delete;
  ObjectHeaps& operator=(ObjectHeaps&&) = delete;

  TFixedIHeapClass<AircraftClass>& aircraft() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  TFixedIHeapClass<AnimClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  TFixedIHeapClass<BuildingClass>& building() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  TFixedIHeapClass<BulletClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  TFixedIHeapClass<FactoryClass>& factory() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return factory_;
  }
  TFixedIHeapClass<HouseClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }
  TFixedIHeapClass<InfantryClass>& infantry() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  TFixedIHeapClass<OverlayClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  TFixedIHeapClass<SmudgeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  TFixedIHeapClass<TeamClass>& team() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_;
  }
  TFixedIHeapClass<TeamTypeClass>& team_type() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return team_type_;
  }
  TFixedIHeapClass<TemplateClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return tmplate_;
  }
  TFixedIHeapClass<TerrainClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  TFixedIHeapClass<TriggerClass>& trigger() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_;
  }
  TFixedIHeapClass<UnitClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  TFixedIHeapClass<VesselClass>& vessel() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return vessel_;
  }
  TFixedIHeapClass<TriggerTypeClass>& trigger_type()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return trigger_type_;
  }

  // Names the first heap that disagrees with itself and says how, or returns
  // "" when all of them are sound. -CHECKHEAPS calls this once a frame to
  // catch the frame corruption appears in, rather than the later frame whose
  // DCHECK_HEAP_SLOT happens to step on the damaged object.
  [[nodiscard]] std::string Validate() const;

 private:
  TFixedIHeapClass<AircraftClass> aircraft_;
  TFixedIHeapClass<AnimClass> anim_;
  TFixedIHeapClass<BuildingClass> building_;
  TFixedIHeapClass<BulletClass> bullet_;
  TFixedIHeapClass<FactoryClass> factory_;
  TFixedIHeapClass<HouseClass> house_;
  TFixedIHeapClass<InfantryClass> infantry_;
  TFixedIHeapClass<OverlayClass> overlay_;
  TFixedIHeapClass<SmudgeClass> smudge_;
  TFixedIHeapClass<TeamClass> team_;
  TFixedIHeapClass<TeamTypeClass> team_type_;
  TFixedIHeapClass<TemplateClass> tmplate_;
  TFixedIHeapClass<TerrainClass> terrain_;
  TFixedIHeapClass<TriggerClass> trigger_;
  TFixedIHeapClass<UnitClass> unit_;
  TFixedIHeapClass<VesselClass> vessel_;
  TFixedIHeapClass<TriggerTypeClass> trigger_type_;
};

// Returns the ObjectHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline ObjectHeaps& TheObjectHeaps() {
  return base::Installed<ObjectHeaps>::Get();
}

#endif  // CNC_RED_ALERT_RA_OBJECT_HEAPS_H_
