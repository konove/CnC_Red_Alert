// Tests for base::Payload: union layout, copy-out reads, and writes that
// change only the written arm's bytes.

#include "base/payload.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "gtest/gtest.h"

namespace base {
namespace {

struct Pair {
  int32_t first;
  int8_t second;  // three padding bytes follow
};
struct Narrow {
  int16_t value;
};
struct Pointer {
  void* address;
};

static_assert(sizeof(Payload<Pair, Narrow>) == 8);
static_assert(alignof(Payload<Pair, Narrow>) == 4);
static_assert(sizeof(Payload<Narrow, Pointer>) == sizeof(void*));
static_assert(alignof(Payload<Narrow, Pointer>) == alignof(void*));
static_assert(Payload<Pair, Narrow>::kHolds<Narrow>);
static_assert(!Payload<Pair, Narrow>::kHolds<Pointer>);
static_assert(std::is_trivially_copyable_v<Payload<Pair, Narrow>>);

TEST(PayloadTest, StartsZeroed) {
  const Payload<Pair, Narrow> payload;
  for (const std::byte b : payload.bytes()) {
    EXPECT_EQ(b, std::byte{0});
  }
}

TEST(PayloadTest, UpdateThenGetReturnsTheArm) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = 0x01020304;
    p.second = 5;
  });
  const Pair p = payload.Get<Pair>();
  EXPECT_EQ(p.first, 0x01020304);
  EXPECT_EQ(p.second, 5);
}

TEST(PayloadTest, UpdateKeepsTheBytesPastTheArm) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = -1;
    p.second = 7;
  });
  payload.Update<Narrow>([](Narrow& n) { n.value = 0; });
  // A union store of the narrow member would leave bytes 2..7 alone too.
  const auto bytes = payload.bytes();
  EXPECT_EQ(bytes.front(), std::byte{0});
  EXPECT_EQ(bytes.subspan(1).front(), std::byte{0});
  EXPECT_EQ(bytes.subspan(2).front(), std::byte{0xFF});
  EXPECT_EQ(bytes.subspan(3).front(), std::byte{0xFF});
  EXPECT_EQ(payload.Get<Pair>().second, 7);
}

TEST(PayloadTest, UpdateKeepsZeroPadding) {
  Payload<Pair, Narrow> payload;
  payload.Update<Pair>([](Pair& p) {
    p.first = 1;
    p.second = 1;
  });
  for (const std::byte b : payload.bytes().subspan(5)) {
    EXPECT_EQ(b, std::byte{0});
  }
}

TEST(PayloadTest, SetCopiesAnArmFromAnotherPayload) {
  Payload<Pair, Narrow> from;
  from.Update<Pair>([](Pair& p) { p.first = 42; });
  Payload<Pair, Narrow> to;
  to.Set(from.Get<Pair>());
  EXPECT_EQ(to.Get<Pair>().first, 42);
}

}  // namespace
}  // namespace base
