// File: TypeHeaps, the heaps that hold Red Alert's object type classes.

#ifndef CNC_RED_ALERT_RA_CORE_TYPE_HEAPS_H_
#define CNC_RED_ALERT_RA_CORE_TYPE_HEAPS_H_

#include "absl/base/attributes.h"
#include "base/installed.h"
#include "game/core/heap.h"
#include "ra/objects/type.h"
#include "ra/objects/warhead.h"
#include "ra/objects/weapon.h"

namespace ra {

// One heap per kind of object type: the descriptions of what a unit, a
// building or a warhead is, as opposed to the objects on the map. They are
// built once, adjusted by the rules files, and never saved -- loading a
// saved game reads the rules again. Game owns the one TypeHeaps;
// everything else reaches it through TheTypeHeaps().
//
// Example:
//   return TheTypeHeaps().building().Alloc();
class TypeHeaps {
 public:
  TypeHeaps() = default;
  ~TypeHeaps();

  TypeHeaps(const TypeHeaps&) = delete;
  TypeHeaps& operator=(const TypeHeaps&) = delete;
  TypeHeaps(TypeHeaps&&) = delete;
  TypeHeaps& operator=(TypeHeaps&&) = delete;

  game::ObjectHeap<HouseTypeClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }
  game::ObjectHeap<BuildingTypeClass>& building()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  game::ObjectHeap<AircraftTypeClass>& aircraft()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  game::ObjectHeap<InfantryTypeClass>& infantry()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  game::ObjectHeap<BulletTypeClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  game::ObjectHeap<AnimTypeClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  game::ObjectHeap<UnitTypeClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  game::ObjectHeap<VesselTypeClass>& vessel() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return vessel_;
  }
  game::ObjectHeap<TemplateTypeClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return template_;
  }
  game::ObjectHeap<TerrainTypeClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  game::ObjectHeap<OverlayTypeClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  game::ObjectHeap<SmudgeTypeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  game::ObjectHeap<WeaponTypeClass>& weapon() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return weapon_;
  }
  game::ObjectHeap<WarheadTypeClass>& warhead() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return warhead_;
  }

 private:
  game::ObjectHeap<HouseTypeClass> house_;
  game::ObjectHeap<BuildingTypeClass> building_;
  game::ObjectHeap<AircraftTypeClass> aircraft_;
  game::ObjectHeap<InfantryTypeClass> infantry_;
  game::ObjectHeap<BulletTypeClass> bullet_;
  game::ObjectHeap<AnimTypeClass> anim_;
  game::ObjectHeap<UnitTypeClass> unit_;
  game::ObjectHeap<VesselTypeClass> vessel_;
  game::ObjectHeap<TemplateTypeClass> template_;
  game::ObjectHeap<TerrainTypeClass> terrain_;
  game::ObjectHeap<OverlayTypeClass> overlay_;
  game::ObjectHeap<SmudgeTypeClass> smudge_;
  game::ObjectHeap<WeaponTypeClass> weapon_;
  game::ObjectHeap<WarheadTypeClass> warhead_;
};

// Returns the TypeHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline TypeHeaps& TheTypeHeaps() { return base::Installed<TypeHeaps>::Get(); }

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_TYPE_HEAPS_H_
