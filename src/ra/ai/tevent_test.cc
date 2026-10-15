// Tests for the trigger events in a real Game with no game data: how an event
// reads its scenario INI fields.

#include "ra/ai/tevent.h"

#include <string_view>

#include "base/pooled.h"
#include "base/strings/tokenizer.h"
#include "gtest/gtest.h"
#include "ra/ai/team_type.h"
#include "ra/core/object_heaps.h"
#include "ra/map/world.h"
#include "ra/startup/game_test_util.h"

namespace ra {

namespace {

// A Game with one team type, in slot 0 of a four-slot heap.
class TEventTest : public GameFixture {
 protected:
  void SetUp() override {
    TheObjectHeaps().team_type().Reset(4);
    team_type_ = base::NewPooled<TeamType>();
    team_type_->Set_Name("tanks");
  }

  // Reads an event from `entry` in INI format `format`.
  static TEventClass Read(const int format, const std::string_view entry) {
    TheWorld().new_ini_format() = format;
    base::Tokenizer tokens(entry);
    TEventClass event;
    event.Read_INI(tokens);
    return event;
  }

  TeamType* team_type_ = nullptr;
};

// A number past the last event, in any format, reads as no event rather than
// as one that indexes past the name table and always happens.
TEST_F(TEventTest, ANumberThatIsNoEventReadsAsNone) {
  EXPECT_EQ(Read(2, "33,-1,5").Event, TEVENT_NONE);
  EXPECT_EQ(Read(2, "-2,-1,5").Event, TEVENT_NONE);
  EXPECT_EQ(Read(1, "99,5").Event, TEVENT_NONE);
  EXPECT_EQ(Read(0, "40,0,0,None,5,0").Event, TEVENT_NONE);
}

// Format 1 gives a team event's team as its heap slot.
TEST_F(TEventTest, FormatOneReadsTheTeamSlot) {
  const TEventClass event = Read(1, "23,0");
  EXPECT_EQ(event.Event, TEVENT_LEAVES_MAP);
  EXPECT_EQ(event.Team, team_type_);
}

// A slot outside the heap reads as no team instead of failing a check.
TEST_F(TEventTest, FormatOneReadsASlotOutsideTheHeapAsNoTeam) {
  EXPECT_EQ(Read(1, "23,9").Team, nullptr);
  EXPECT_EQ(Read(1, "23,-1").Team, nullptr);
}

}  // namespace

}  // namespace ra
