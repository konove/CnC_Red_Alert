#include "base/unaligned.h"

#include <array>
#include <cstdint>
#include <span>

#include "base/aligned_buffer.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "gtest/gtest.h"

namespace {
TEST(UnalignedTest, ReadsAndWritesNativeValuesAtEveryByteOffset) {
  constexpr uint64_t kValue = 0x0123456789abcdef;
  alignas(uint64_t) std::array<unsigned char, 24> bytes{};
  for (int offset = 1; offset <= 8; ++offset) {
    bytes.fill(0xa5);
    base::WriteUnaligned(base::ObjectBytes(bytes).subspan(base::ToSize(offset)),
                         kValue);
    EXPECT_EQ(base::ReadUnaligned<uint64_t>(
                  base::ObjectBytes(bytes).subspan(base::ToSize(offset))),
              kValue);
    EXPECT_EQ(base::CompareBytes(base::ObjectBytes(bytes).subspan(base::ToSize(offset)), base::ObjectBytes(kValue), sizeof(kValue)), 0);
    EXPECT_EQ(bytes.at(base::ToSize(offset - 1)), 0xa5);
    EXPECT_EQ(bytes.at(base::ToSize(offset) + sizeof(kValue)), 0xa5);
  }
}

TEST(UnalignedTest, ReadLittleEndianIgnoresHostOrderAndBounds) {
  constexpr std::array<unsigned char, 5> kBytes = {0x01, 0x02, 0x03, 0x04,
                                                   0x05};
  const auto bytes = std::as_bytes(std::span(kBytes));
  EXPECT_EQ(base::ReadLittleEndian<uint8_t>(bytes, 4), 0x05);
  EXPECT_EQ(base::ReadLittleEndian<uint16_t>(bytes, 1), 0x0302);
  EXPECT_EQ(base::ReadLittleEndian<uint32_t>(bytes, 1), 0x05040302U);
  EXPECT_EQ(base::ReadLittleEndian<int16_t>(bytes, 0), 0x0201);
  EXPECT_EQ(base::ReadLittleEndian<uint16_t>(bytes, 4), 0);
  EXPECT_EQ(base::ReadLittleEndian<uint32_t>(bytes, 2), 0U);
  EXPECT_EQ(base::ReadLittleEndian<uint8_t>(bytes, 5), 0);
  EXPECT_EQ(base::ReadLittleEndian<uint8_t>(bytes, -1), 0);
}
}  // namespace

TEST(AlignedBufferTest, RecoversOriginalObjectAndRejectsInteriorByte) {
  alignas(uint64_t) std::array<unsigned char, 16> bytes{};
  uint64_t value = 42;
  EXPECT_EQ(base::AlignedObject<uint64_t>(&value), &value);
  EXPECT_EQ(base::RestoreMutableObject<uint64_t>(&value), &value);
  EXPECT_EQ(base::RestoreMutableObject<uint64_t>(nullptr), nullptr);
  // the switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(
      (void)base::AlignedObject<uint64_t>(std::span(bytes).subspan(1).data()),
      "Check failed");
}
