#include "ra/utracker.h"

#include "gtest/gtest.h"

namespace {

TEST(UnitTrackerTest, NetworkRoundTripPreservesEveryCounter) {
  UnitTrackerClass totals(3);
  EXPECT_EQ(totals.Get_All_Totals().size(), 3);
  totals.Increment_Unit_Total(0);
  totals.Increment_Unit_Total(2);
  totals.Increment_Unit_Total(2);
  totals.To_Network_Format();
  totals.To_Network_Format();
  totals.To_PC_Format();
  totals.To_PC_Format();
  EXPECT_EQ(totals.Get_Unit_Total(0), 1);
  EXPECT_EQ(totals.Get_Unit_Total(1), 0);
  EXPECT_EQ(totals.Get_Unit_Total(2), 2);
  totals.Decrement_Unit_Total(2);
  EXPECT_EQ(totals.Get_Unit_Total(2), 1);
  totals.Clear_Unit_Total();
  EXPECT_EQ(totals.Get_Unit_Total(0), 0);
  EXPECT_EQ(totals.Get_Unit_Total(2), 0);
}

}  // namespace
