#include "port/unaligned.h"

#include <array>
#include <cstdint>
#include <cstring>

#include "base/numeric.h"
#include "gtest/gtest.h"
#include "port/aligned_buffer.h"

namespace {
TEST(UnalignedTest, ReadsAndWritesNativeValuesAtEveryByteOffset) {
  constexpr uint64_t kValue = 0x0123456789abcdef;
  alignas(uint64_t) std::array<unsigned char, 24> bytes{};
  for (int offset = 1; offset <= 8; ++offset) {
    bytes.fill(0xa5);
    port::WriteUnaligned(bytes.data() + offset, kValue);
    EXPECT_EQ(port::ReadUnaligned<uint64_t>(bytes.data() + offset), kValue);
    EXPECT_EQ(std::memcmp(bytes.data() + offset, &kValue, sizeof(kValue)), 0);
    EXPECT_EQ(bytes[base::ToSize(offset - 1)], 0xa5);
    EXPECT_EQ(bytes[base::ToSize(offset) + sizeof(kValue)], 0xa5);
  }
}
}  // namespace

TEST(AlignedBufferTest, RecoversOriginalObjectAndRejectsInteriorByte) {
  alignas(uint64_t) std::array<unsigned char, 16> bytes{};
  uint64_t value = 42;
  EXPECT_EQ(port::AlignedObject<uint64_t>(&value), &value);
  EXPECT_EQ(port::RestoreMutableObject<uint64_t>(&value), &value);
  EXPECT_EQ(port::RestoreMutableObject<uint64_t>(nullptr), nullptr);
  // the switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH((void)port::AlignedObject<uint64_t>(bytes.data() + 1),
               "Check failed");
}
