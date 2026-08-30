// Layout tripwire for the raw-byte save format.
//
// Saved games are not serialized field by field. TFixedIHeapClass<T>::Save and
// ::Load (ra/heap.cc) write and read sizeof(T) raw bytes per object and then
// repair the vtable pointer with a placement-new of T(NoInitClass()). The same
// shape appears in ra/iomap.cc (CellClass, MouseClass), ra/saveload.cc
// (ScenarioClass, ScoreClass, CarryoverClass, SpecialClass, GameOptionsClass),
// ra/vortex.cc and ra/session.cc.
//
// Nothing in the type system enforces that a T survives that round trip.
// AbstractClass declares a virtual destructor, so every serialized type fails
// both is_trivially_copyable_v and is_trivially_destructible_v and no standard
// trait can express the real contract. clang-tidy cannot help either: the byte
// copy goes through Pipe::Put(const void*, int) and Straw::Get(void*, int), so
// bugprone-raw-memory-call-on-non-trivial-type and
// bugprone-undefined-memory-manipulation never see a class-typed pointer.
//
// sizeof(T) is the closest observable proxy. Adding a std::string, std::vector,
// std::optional or any other member that owns storage or points into itself
// changes it, and this test names the type that changed.
//
// If a case here fails:
//   1. Confirm the new member really can survive a memcpy and a placement-new
//      that does not initialize it. If it cannot, the type must not be
//      byte-serialized.
//   2. Update the expected size below.
//   3. Check SAVEGAME_VERSION in ra/saveload.cc. It already sums sizeof() over
//      most of these types, so a size change invalidates existing saves
//      automatically -- but the types marked "not in SAVEGAME_VERSION" below
//      are absent from that sum, and for those this test is the only guard.

#include <cstddef>

#include "base/types.h"
#include "gtest/gtest.h"
#include "ra/aircraft.h"
#include "ra/anim.h"
#include "ra/base.h"
#include "ra/building.h"
#include "ra/bullet.h"
#include "ra/carry.h"
#include "ra/cell.h"
#include "ra/event.h"
#include "ra/factory.h"
#include "ra/goptions.h"
#include "ra/house.h"
#include "ra/infantry.h"
#include "ra/layer.h"
#include "ra/mouse.h"
#include "ra/overlay.h"
#include "ra/scenario.h"
#include "ra/score.h"
#include "ra/session.h"
#include "ra/smudge.h"
#include "ra/special.h"
#include "ra/team.h"
#include "ra/teamtype.h"
#include "ra/template.h"
#include "ra/terrain.h"
#include "ra/trigger.h"
#include "ra/trigtype.h"
#include "ra/type.h"
#include "ra/unit.h"
#include "ra/vessel.h"
#include "ra/vortex.h"
#include "ra/warhead.h"
#include "ra/weapon.h"

namespace {

struct LayoutCase {
  const char* name;
  base::ssize expected;
  base::ssize actual;
};

#define LAYOUT_CASE(type, size) \
  LayoutCase { #type, size, static_cast<base::ssize>(sizeof(type)) }

// Sizes are pinned for the Itanium C++ ABI on a 64-bit target; see the skip
// below.
constexpr LayoutCase kSerializedTypes[] = {
    // Game objects, saved by TFixedIHeapClass<T>::Save/Load.
    LAYOUT_CASE(AircraftClass, 656),
    LAYOUT_CASE(AnimClass, 104),
    LAYOUT_CASE(BuildingClass, 520),
    LAYOUT_CASE(BulletClass, 96),
    LAYOUT_CASE(FactoryClass, 80),
    LAYOUT_CASE(HouseClass, 11256),
    LAYOUT_CASE(InfantryClass, 648),
    LAYOUT_CASE(OverlayClass, 56),
    LAYOUT_CASE(SmudgeClass, 56),
    LAYOUT_CASE(TeamClass, 144),
    LAYOUT_CASE(TeamTypeClass, 320),
    LAYOUT_CASE(TemplateClass, 56),
    LAYOUT_CASE(TerrainClass, 96),
    LAYOUT_CASE(TriggerClass, 96),
    LAYOUT_CASE(TriggerTypeClass, 128),
    LAYOUT_CASE(UnitClass, 712),
    LAYOUT_CASE(VesselClass, 712),

    // Static type heaps. Objects reference these by index, so their layout is
    // part of the save contract even though the heaps themselves come from
    // rules.ini.
    LAYOUT_CASE(AircraftTypeClass, 512),
    LAYOUT_CASE(AnimTypeClass, 416),
    LAYOUT_CASE(BuildingTypeClass, 632),
    LAYOUT_CASE(BulletTypeClass, 384),
    LAYOUT_CASE(HouseTypeClass, 328),
    LAYOUT_CASE(InfantryTypeClass, 528),
    LAYOUT_CASE(OverlayTypeClass, 384),
    LAYOUT_CASE(SmudgeTypeClass, 376),
    LAYOUT_CASE(TemplateTypeClass, 368),
    LAYOUT_CASE(TerrainTypeClass, 392),
    LAYOUT_CASE(UnitTypeClass, 512),

    // Not in SAVEGAME_VERSION -- this test is the only guard.
    LAYOUT_CASE(VesselTypeClass, 512),
    LAYOUT_CASE(WarheadTypeClass, 40),
    LAYOUT_CASE(WeaponTypeClass, 64),

    // Whole-object byte I/O outside the heaps.
    LAYOUT_CASE(BaseClass, 64),
    LAYOUT_CASE(CellClass, 128),
    LAYOUT_CASE(ChronalVortexClass, 4440),
    LAYOUT_CASE(LayerClass, 48),
    LAYOUT_CASE(MouseClass, 11752),
    LAYOUT_CASE(ScenarioClass, 2032),

    // Also not in SAVEGAME_VERSION.
    LAYOUT_CASE(BaseNodeClass, 8),
    LAYOUT_CASE(CarryoverClass, 48),
    LAYOUT_CASE(EventClass, 32),
    LAYOUT_CASE(GameOptionsClass, 264),
    LAYOUT_CASE(NodeNameType, 40),
    LAYOUT_CASE(ScoreClass, 80),
    LAYOUT_CASE(SpecialClass, 4),
};

#undef LAYOUT_CASE

TEST(HeapLayoutTest, SerializedTypeSizesAreStable) {
#ifdef _MSC_VER
  GTEST_SKIP() << "Sizes are pinned for the Itanium C++ ABI.";
#else
  if constexpr (sizeof(void*) != 8) {
    GTEST_SKIP() << "Sizes are pinned for 64-bit targets.";
  } else {
    for (const LayoutCase& layout : kSerializedTypes) {
      EXPECT_EQ(layout.expected, layout.actual)
          << layout.name
          << " changed size, so the raw-byte save format changed. See the "
             "comment at the top of this file before updating the value.";
    }
  }
#endif
}

}  // namespace
