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

  game::TFixedIHeapClass<HouseTypeClass>& house()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }
  game::TFixedIHeapClass<BuildingTypeClass>& building()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  game::TFixedIHeapClass<AircraftTypeClass>& aircraft()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  game::TFixedIHeapClass<InfantryTypeClass>& infantry()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  game::TFixedIHeapClass<BulletTypeClass>& bullet()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  game::TFixedIHeapClass<AnimTypeClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  game::TFixedIHeapClass<UnitTypeClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  game::TFixedIHeapClass<VesselTypeClass>& vessel()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return vessel_;
  }
  game::TFixedIHeapClass<TemplateTypeClass>& tmplate()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return template_;
  }
  game::TFixedIHeapClass<TerrainTypeClass>& terrain()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  game::TFixedIHeapClass<OverlayTypeClass>& overlay()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  game::TFixedIHeapClass<SmudgeTypeClass>& smudge()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  game::TFixedIHeapClass<WeaponTypeClass>& weapon()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return weapon_;
  }
  game::TFixedIHeapClass<WarheadTypeClass>& warhead()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return warhead_;
  }

 private:
  game::TFixedIHeapClass<HouseTypeClass> house_;
  game::TFixedIHeapClass<BuildingTypeClass> building_;
  game::TFixedIHeapClass<AircraftTypeClass> aircraft_;
  game::TFixedIHeapClass<InfantryTypeClass> infantry_;
  game::TFixedIHeapClass<BulletTypeClass> bullet_;
  game::TFixedIHeapClass<AnimTypeClass> anim_;
  game::TFixedIHeapClass<UnitTypeClass> unit_;
  game::TFixedIHeapClass<VesselTypeClass> vessel_;
  game::TFixedIHeapClass<TemplateTypeClass> template_;
  game::TFixedIHeapClass<TerrainTypeClass> terrain_;
  game::TFixedIHeapClass<OverlayTypeClass> overlay_;
  game::TFixedIHeapClass<SmudgeTypeClass> smudge_;
  game::TFixedIHeapClass<WeaponTypeClass> weapon_;
  game::TFixedIHeapClass<WarheadTypeClass> warhead_;
};

// Returns the TypeHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline TypeHeaps& TheTypeHeaps() { return base::Installed<TypeHeaps>::Get(); }

}  // namespace ra

#endif  // CNC_RED_ALERT_RA_CORE_TYPE_HEAPS_H_
