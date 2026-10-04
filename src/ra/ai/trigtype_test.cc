// Tests for the trigger type in a real Game with no game data: what it drops
// when a type it refers to is destroyed, and how it reads and writes its
// scenario INI entry.

#include "ra/ai/trigtype.h"

#include "base/pooled.h"
#include "gtest/gtest.h"
#include "ra/ai/teamtype.h"
#include "ra/core/object_heaps.h"
#include "ra/startup/game_test_util.h"

namespace ra {

namespace {

// A Game with one team type and one trigger type.
class TriggerTypeTest : public GameFixture {
 protected:
  void SetUp() override {
    TheObjectHeaps().team_type().Reset(4);
    TheObjectHeaps().trigger_type().Reset(4);
    team_type_ = base::NewPooled<TeamTypeClass>();
    trigger_type_ = base::NewPooled<TriggerType>();
  }

  TeamTypeClass* team_type_ = nullptr;
  TriggerType* trigger_type_ = nullptr;
};

TEST_F(TriggerTypeTest, DetachClearsTheTeamTypeOfItsEventsAndActions) {
  trigger_type_->Event1.Team = team_type_;
  trigger_type_->Event2.Team = team_type_;
  trigger_type_->Action1.team_type = team_type_;
  trigger_type_->Action2.team_type = team_type_;

  trigger_type_->Detach(team_type_->As_Target());

  EXPECT_EQ(trigger_type_->Event1.Team, nullptr);
  EXPECT_EQ(trigger_type_->Event2.Team, nullptr);
  EXPECT_EQ(trigger_type_->Action1.team_type, nullptr);
  EXPECT_EQ(trigger_type_->Action2.team_type, nullptr);
}

TEST_F(TriggerTypeTest, DetachLeavesAnotherTeamTypeAlone) {
  trigger_type_->Event1.Team = team_type_;
  trigger_type_->Event2.Team = team_type_;

  trigger_type_->Detach(base::NewPooled<TeamTypeClass>()->As_Target());

  EXPECT_EQ(trigger_type_->Event1.Team, team_type_);
  EXPECT_EQ(trigger_type_->Event2.Team, team_type_);
}

}  // namespace

}  // namespace ra
