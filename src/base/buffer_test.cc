// Tests byte spans without relying on raw memory operations for expectations.

#include "base/buffer.h"

#include <array>
#include <cstddef>
#include <span>

#include "gtest/gtest.h"

namespace {

template <class T>
concept HasObjectBytes = requires(T& object) { base::ObjectBytes(object); };
static_assert(!HasObjectBytes<unsigned char*>);
static_assert(HasObjectBytes<unsigned char[4]>);

TEST(BufferTest, PreservesObjectAndArrayExtent) {
  unsigned char values[4] = {1, 2, 3, 4};
  const auto bytes = base::ObjectBytes(values);
  EXPECT_EQ(bytes.size(), sizeof(values));
  bytes[2] = std::byte{9};
  EXPECT_EQ(values[2], 9);
  const unsigned char immutable[2] = {5, 6};
  EXPECT_EQ(base::ObjectBytes(immutable)[1], std::byte{6});
}

TEST(BufferTest, CopiesOnlyTheRequestedPrefix) {
  const unsigned char source[4] = {1, 2, 3, 4};
  unsigned char dest[4] = {8, 8, 8, 8};
  base::CopyBytes(base::ObjectBytes(dest), base::ObjectBytes(source), 2);
  EXPECT_EQ(dest[0], 1);
  EXPECT_EQ(dest[1], 2);
  EXPECT_EQ(dest[2], 8);
  EXPECT_EQ(dest[3], 8);
}

TEST(BufferTest, MovesInBothOverlappingDirections) {
  std::array<std::byte, 4> bytes = {std::byte{1}, std::byte{2}, std::byte{3},
                                    std::byte{4}};
  base::MoveBytes(std::span(bytes).subspan(1), bytes, 3);
  EXPECT_EQ(bytes, (std::array<std::byte, 4>{std::byte{1}, std::byte{1},
                                             std::byte{2}, std::byte{3}}));
  base::MoveBytes(bytes, std::span(bytes).subspan(1), 3);
  EXPECT_EQ(bytes, (std::array<std::byte, 4>{std::byte{1}, std::byte{2},
                                             std::byte{3}, std::byte{3}}));
  base::MoveBytes(bytes, bytes, 4);
  EXPECT_EQ(bytes[0], std::byte{1});
}

TEST(BufferTest, FillUsesTheLowByteAndPreservesTheSuffix) {
  unsigned char bytes[3] = {1, 2, 3};
  base::FillBytes(base::ObjectBytes(bytes), -1, 2);
  EXPECT_EQ(bytes[0], 255);
  EXPECT_EQ(bytes[1], 255);
  EXPECT_EQ(bytes[2], 3);
}

TEST(BufferTest, CompareUsesUnsignedBytes) {
  const unsigned char left[3] = {0, 255, 0};
  const unsigned char right[3] = {0, 127, 1};
  EXPECT_EQ(
      base::CompareBytes(base::ObjectBytes(left), base::ObjectBytes(right), 1),
      0);
  EXPECT_GT(
      base::CompareBytes(base::ObjectBytes(left), base::ObjectBytes(right), 3),
      0);
  EXPECT_LT(
      base::CompareBytes(base::ObjectBytes(right), base::ObjectBytes(left), 3),
      0);
}

TEST(BufferDeathTest, RejectsInvalidCountsBeforeAccess) {
  unsigned char bytes[3] = {};
  const unsigned char source[2] = {};
  // The switches and formatted diagnostics are inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(
      base::CopyBytes(base::ObjectBytes(bytes), base::ObjectBytes(source), 3),
      "Check failed");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(base::FillBytes(base::ObjectBytes(bytes), 0, 4), "Check failed");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(
      base::MoveBytes(base::ObjectBytes(bytes), base::ObjectBytes(source), -1),
      "Check failed");
}

}  // namespace
