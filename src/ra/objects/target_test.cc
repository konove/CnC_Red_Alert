// Pins how TargetClass packs its kind and value, which network events carry
// as raw bytes.

#include "ra/objects/target.h"

#include <cstdint>

#include "gtest/gtest.h"
#include "ra/core/defines.h"
#include "ra/core/packed.h"
#include "ra/objects/abstract.h"

namespace ra {
namespace {

TEST(TargetClassTest, KindAndValuePackLikeBuildTarget) {
  const TargetClass target(RTTI_UNIT, 5);
  EXPECT_EQ(target.As_Target(), Build_Target(RTTI_UNIT, 5));
  EXPECT_EQ(static_cast<RTTIType>(target), RTTI_UNIT);
  EXPECT_EQ(target.Value(), 5U);
  EXPECT_TRUE(target.Is_Valid());
}

TEST(TargetClassTest, CellTargetKeepsTwentyFourBits) {
  const TargetClass target(static_cast<CELL>(-1));
  EXPECT_EQ(static_cast<RTTIType>(target), RTTI_CELL);
  EXPECT_EQ(target.Value(), 0xFFFFFFU);
}

TEST(TargetClassTest, DefaultIsInvalidWithAllValueBitsSet) {
  const TargetClass target;
  EXPECT_FALSE(target.Is_Valid());
  EXPECT_EQ(static_cast<uint32_t>(target.As_Target()), 0x00FFFFFFU);
}

TEST(TargetClassTest, NullPointerIsTargetNone) {
  const TargetClass target(static_cast<const AbstractClass*>(nullptr));
  EXPECT_EQ(target.As_Target(), kTargetNone);
  EXPECT_FALSE(target.Is_Valid());
}

TEST(TargetClassTest, RoundTripsAPackedTarget) {
  const TARGET packed = Build_Target(RTTI_BUILDING, 0x123456);
  const TargetClass target(packed);
  EXPECT_EQ(target.As_TARGET(), packed);
  EXPECT_TRUE(target == TargetClass(RTTI_BUILDING, 0x123456));
  EXPECT_FALSE(target == TargetClass(RTTI_BUILDING, 0x123457));
}

}  // namespace
}  // namespace ra
