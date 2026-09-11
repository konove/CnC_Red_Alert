// Legacy layout guards retained only until step 30 removes this test.
// The save body is now field-wise and no longer depends on these sizes.

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

    // Remaining conservative layout guards.
    LAYOUT_CASE(EventClass, 32),
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
