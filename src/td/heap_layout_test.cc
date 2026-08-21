// Layout tripwire for the raw-byte save format.
//
// Saved games are not serialized field by field. TFixedIHeapClass<T>::Load
// (td/heap.cc) reads sizeof(T) raw bytes per object and then repairs the vtable
// pointer with a placement-new of T(NoInitClass()). td/ioobj.cc does the same
// for every game object through Read_Object/Write_Object (td/saveload.cc),
// which even patches the vtable pointer by hand, and td/iomap.cc does it for
// CellClass and MouseClass.
//
// Nothing in the type system enforces that a T survives that round trip.
// AbstractClass declares a virtual destructor, so every serialized type fails
// both is_trivially_copyable_v and is_trivially_destructible_v and no standard
// trait can express the real contract. clang-tidy cannot help either: the byte
// copy goes through FileClass::Read/Write and Read_Object/Write_Object, all of
// which take void*, so bugprone-raw-memory-call-on-non-trivial-type and
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
//   3. Check SAVEGAME_VERSION in td/saveload.cc. It already sums sizeof() over
//      most of these types, so a size change invalidates existing saves
//      automatically -- but the types marked "not in SAVEGAME_VERSION" below
//      are absent from that sum, and for those this test is the only guard.

#include <cstddef>

#include "base/types.h"
#include "gtest/gtest.h"
#include "td/aircraft.h"
#include "td/anim.h"
#include "td/base.h"
#include "td/building.h"
#include "td/bullet.h"
#include "td/cell.h"
#include "td/event.h"
#include "td/factory.h"
#include "td/house.h"
#include "td/infantry.h"
#include "td/layer.h"
#include "td/mouse.h"
#include "td/overlay.h"
#include "td/score.h"
#include "td/smudge.h"
#include "td/team.h"
#include "td/teamtype.h"
#include "td/template.h"
#include "td/terrain.h"
#include "td/trigger.h"
#include "td/type.h"
#include "td/unit.h"

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
    // Game objects, saved by TFixedIHeapClass<T>::Load and td/ioobj.cc.
    LAYOUT_CASE(AircraftClass, 384),
    LAYOUT_CASE(AnimClass, 88),
    LAYOUT_CASE(BuildingClass, 296),
    LAYOUT_CASE(BulletClass, 104),
    LAYOUT_CASE(FactoryClass, 48),
    LAYOUT_CASE(HouseClass, 3368),
    LAYOUT_CASE(InfantryClass, 360),
    LAYOUT_CASE(OverlayClass, 48),
    LAYOUT_CASE(SmudgeClass, 48),
    LAYOUT_CASE(TeamClass, 112),
    LAYOUT_CASE(TeamTypeClass, 256),
    LAYOUT_CASE(TemplateClass, 48),
    LAYOUT_CASE(TerrainClass, 72),
    LAYOUT_CASE(UnitClass, 368),

    // Static type tables. Objects reference these by index, so their layout is
    // part of the save contract.
    LAYOUT_CASE(AircraftTypeClass, 136),
    LAYOUT_CASE(AnimTypeClass, 112),
    LAYOUT_CASE(BuildingTypeClass, 280),
    LAYOUT_CASE(BulletTypeClass, 88),
    LAYOUT_CASE(HouseTypeClass, 48),
    LAYOUT_CASE(InfantryTypeClass, 408),
    LAYOUT_CASE(OverlayTypeClass, 80),
    LAYOUT_CASE(SmudgeTypeClass, 80),
    LAYOUT_CASE(TemplateTypeClass, 88),
    LAYOUT_CASE(TerrainTypeClass, 88),
    LAYOUT_CASE(UnitTypeClass, 152),

    // Whole-object byte I/O outside the heaps.
    LAYOUT_CASE(BaseClass, 56),
    LAYOUT_CASE(CellClass, 56),
    LAYOUT_CASE(LayerClass, 40),
    LAYOUT_CASE(MouseClass, 1952),

    // Not in SAVEGAME_VERSION -- this test is the only guard.
    LAYOUT_CASE(BaseNodeClass, 8),
    LAYOUT_CASE(EventClass, 32),
    LAYOUT_CASE(ScoreClass, 56),
    LAYOUT_CASE(TriggerClass, 56),
    LAYOUT_CASE(WarheadTypeClass, 12),
    LAYOUT_CASE(WeaponTypeClass, 20),
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
