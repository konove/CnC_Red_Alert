// Pins the low-byte rule the trigger events and actions read their one-byte
// arguments with.

#include "ra/core/low_byte.h"

#include <utility>

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

TEST(LowByteTest, ReadsTheLowByteAsSigned) {
  EXPECT_EQ(LowByte(5), 5);
  EXPECT_EQ(LowByte(0x1234567F), 0x7F);
  EXPECT_EQ(LowByte(0xFF), -1);
  EXPECT_EQ(LowByte(-255), 1);
  EXPECT_EQ(LowByte(-170), 86);
}

TEST(LowByteTest, ReadsAnEnumFromTheLowByte) {
  EXPECT_EQ(LowByteAs<HousesType>(0x12345600 + std::to_underlying(HOUSE_USSR)),
            HOUSE_USSR);
  EXPECT_EQ(LowByteAs<HousesType>(-1), HOUSE_NONE);
}

}  // namespace
}  // namespace ra
