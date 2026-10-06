// Tests for the team types: reading and writing their scenario INI entries.

#include "ra/ai/teamtype.h"

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/core/defines.h"
#include "ra/core/object_heaps.h"
#include "ra/core/type_heaps.h"
#include "ra/map/world.h"
#include "ra/objects/type.h"
#include "ra/startup/ui_test_util.h"

namespace ra {
namespace {

// A Game with every type a team can have as a member, reading the current
// scenario format, and one team type to work on.
class TeamTypeTest : public UiFixture {
 protected:
  void SetUp() override {
    TheTypeHeaps().aircraft().Reset(magic_enum::enum_count<AircraftType>());
    TheTypeHeaps().infantry().Reset(magic_enum::enum_count<InfantryType>());
    TheTypeHeaps().vessel().Reset(magic_enum::enum_count<VesselType>());
    AircraftTypeClass::Init_Heap();
    InfantryTypeClass::Init_Heap();
    VesselTypeClass::Init_Heap();
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    TheWorld().new_ini_format() = 3;
    team_type_ = base::NewPooled<TeamType>();
  }

  TeamType* team_type_ = nullptr;
};

TEST_F(TeamTypeTest, ReadIniEntrySkipsAnUnknownFirstMember) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,-1,-1,2,BOGUS:1,E1:3,1,3:5");

  ASSERT_EQ(team_type_->ClassCount, 1);
  EXPECT_EQ(team_type_->Members[0].type,
            &InfantryTypeClass::As_Reference(INFANTRY_E1));
  EXPECT_EQ(team_type_->Members[0].quantity, 3);
  ASSERT_EQ(team_type_->MissionCount, 1);
  EXPECT_EQ(team_type_->MissionList[0].Mission, TeamMissionType::kMove);
  EXPECT_EQ(team_type_->MissionList[0].Data.Value, 5);
}

TEST_F(TeamTypeTest, ReadIniEntryTakesAnOriginThatIsNoWaypointAsNone) {
  team_type_->ReadIniEntry("tanks", "2,0,7,0,1,500,-1,0,0");

  EXPECT_EQ(team_type_->Origin, -1);
}

}  // namespace
}  // namespace ra
