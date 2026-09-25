// File: TypeHeaps, the heaps that hold Red Alert's object type classes.

#ifndef CNC_RED_ALERT_RA_TYPE_HEAPS_H_
#define CNC_RED_ALERT_RA_TYPE_HEAPS_H_

#include "absl/base/attributes.h"
#include "engine/base/installed.h"
#include "ra/heap.h"
#include "ra/type.h"
#include "ra/warhead.h"
#include "ra/weapon.h"

// One heap per kind of object type: the descriptions of what a unit, a
// building or a warhead is, as opposed to the objects on the map. They are
// built once, adjusted by the rules files, and never saved -- loading a
// saved game reads the rules again. Game owns the one TypeHeaps;
// everything else reaches it through TheTypeHeaps().
//
// Constructing one binds the matching CCPtr<T>::Heap pointers, so a CCPtr to
// a type class only resolves while a TypeHeaps is installed.
//
// Example:
//   return TheTypeHeaps().building().Alloc();
class TypeHeaps {
 public:
  TypeHeaps();
  ~TypeHeaps();

  TypeHeaps(const TypeHeaps&) = delete;
  TypeHeaps& operator=(const TypeHeaps&) = delete;
  TypeHeaps(TypeHeaps&&) = delete;
  TypeHeaps& operator=(TypeHeaps&&) = delete;

  TFixedIHeapClass<HouseTypeClass>& house() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return house_;
  }
  TFixedIHeapClass<BuildingTypeClass>& building()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return building_;
  }
  TFixedIHeapClass<AircraftTypeClass>& aircraft()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return aircraft_;
  }
  TFixedIHeapClass<InfantryTypeClass>& infantry()
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return infantry_;
  }
  TFixedIHeapClass<BulletTypeClass>& bullet() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return bullet_;
  }
  TFixedIHeapClass<AnimTypeClass>& anim() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return anim_;
  }
  TFixedIHeapClass<UnitTypeClass>& unit() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return unit_;
  }
  TFixedIHeapClass<VesselTypeClass>& vessel() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return vessel_;
  }
  TFixedIHeapClass<TemplateTypeClass>& tmplate() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return template_;
  }
  TFixedIHeapClass<TerrainTypeClass>& terrain() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return terrain_;
  }
  TFixedIHeapClass<OverlayTypeClass>& overlay() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return overlay_;
  }
  TFixedIHeapClass<SmudgeTypeClass>& smudge() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return smudge_;
  }
  TFixedIHeapClass<WeaponTypeClass>& weapon() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return weapon_;
  }
  TFixedIHeapClass<WarheadTypeClass>& warhead() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return warhead_;
  }

 private:
  TFixedIHeapClass<HouseTypeClass> house_;
  TFixedIHeapClass<BuildingTypeClass> building_;
  TFixedIHeapClass<AircraftTypeClass> aircraft_;
  TFixedIHeapClass<InfantryTypeClass> infantry_;
  TFixedIHeapClass<BulletTypeClass> bullet_;
  TFixedIHeapClass<AnimTypeClass> anim_;
  TFixedIHeapClass<UnitTypeClass> unit_;
  TFixedIHeapClass<VesselTypeClass> vessel_;
  TFixedIHeapClass<TemplateTypeClass> template_;
  TFixedIHeapClass<TerrainTypeClass> terrain_;
  TFixedIHeapClass<OverlayTypeClass> overlay_;
  TFixedIHeapClass<SmudgeTypeClass> smudge_;
  TFixedIHeapClass<WeaponTypeClass> weapon_;
  TFixedIHeapClass<WarheadTypeClass> warhead_;
};

// Returns the TypeHeaps that Game installed. CHECK-fails outside a Game's
// lifetime unless a test installed its own.
inline TypeHeaps& TheTypeHeaps() { return base::Installed<TypeHeaps>::Get(); }

#endif  // CNC_RED_ALERT_RA_TYPE_HEAPS_H_
