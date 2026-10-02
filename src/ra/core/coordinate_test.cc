// Pins Coordinate against the golden values packed_test.cc used for the free
// coordinate helpers it replaced, including the 16-bit wrap of each component.

#include "ra/core/coordinate.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"

namespace ra {
namespace {

constexpr Coordinate kSample = Coordinate::FromPacked(0x12345678);

// The members are usable in constant expressions.
static_assert(kSample.x() == 0x5678);
static_assert(Coordinate::FromLeptons(0x5678, 0x1234) == kSample);

// Leptons are plain ints at the interface.
static_assert(std::is_same_v<decltype(kSample.x()), int>);
static_assert(std::is_same_v<decltype(kSample.y()), int>);
static_assert(std::is_same_v<decltype(kSample.cell_x()), int>);
static_assert(std::is_same_v<decltype(kSample.sub_x()), int>);

// A coordinate converts to bool only explicitly, and never to an integer.
static_assert(!std::is_convertible_v<Coordinate, bool>);
static_assert(!std::is_convertible_v<Coordinate, uint32_t>);
static_assert(!std::is_convertible_v<uint32_t, Coordinate>);

TEST(CoordinateTest, DefaultIsZero) {
  EXPECT_EQ(Coordinate{}.packed(), 0U);
  EXPECT_EQ(Coordinate{}, Coordinate::FromLeptons(0, 0));
}

TEST(CoordinateTest, CellCenterOffsetIsHalfACellOnEachAxis) {
  EXPECT_EQ(Coordinate::CellCenterOffset().packed(), 0x00800080U);
  EXPECT_EQ(Coordinate::CellCenterOffset(), Coordinate{}.Snapped());
}

TEST(CoordinateTest, NoPositionHasEveryBitSet) {
  EXPECT_EQ(Coordinate::NoPosition().packed(), 0xFFFFFFFFU);
  EXPECT_NE(Coordinate::NoPosition(), Coordinate{});
  EXPECT_TRUE(static_cast<bool>(Coordinate::NoPosition()));
}

TEST(CoordinateTest, Components) {
  EXPECT_EQ(kSample.packed(), 0x12345678U);
  EXPECT_EQ(kSample.x(), 0x5678);
  EXPECT_EQ(kSample.y(), 0x1234);
  EXPECT_EQ(kSample.cell_x(), 0x56);
  EXPECT_EQ(kSample.cell_y(), 0x12);
  EXPECT_EQ(kSample.sub_x(), 0x78);
  EXPECT_EQ(kSample.sub_y(), 0x34);
}

TEST(CoordinateTest, Rounding) {
  EXPECT_EQ(kSample.Snapped().packed(), 0x12805680U);
  EXPECT_EQ(kSample.Fraction().packed(), 0x00340078U);
  EXPECT_EQ(kSample.Whole().packed(), 0x12005600U);
}

// Leptons are 16-bit words wherever they are packed: out-of-range components
// wrap on the way in, and every component read back is in 0..65535.
TEST(CoordinateTest, ComponentsWrapAtSixteenBits) {
  EXPECT_EQ(Coordinate::FromLeptons(-1, 0x10000),
            Coordinate::FromLeptons(0xFFFF, 0));
  EXPECT_EQ(Coordinate::FromLeptons(0x12345, -0x10000),
            Coordinate::FromLeptons(0x2345, 0));
  constexpr Coordinate kFull = Coordinate::FromPacked(0xFFFFFFFFU);
  EXPECT_EQ(kFull.x(), 0xFFFF);
  EXPECT_EQ(kFull.y(), 0xFFFF);
}

TEST(CoordinateTest, ArithmeticWrapsPerComponent) {
  EXPECT_EQ(
      Coordinate::FromLeptons(0xFFFF, 1) + Coordinate::FromLeptons(2, 0xFFFF),
      Coordinate::FromLeptons(1, 0));
  EXPECT_EQ(Coordinate::FromLeptons(1, 0) - Coordinate::FromLeptons(2, 0),
            Coordinate::FromLeptons(0xFFFF, 0));
  // A carry out of the X word does not reach Y.
  EXPECT_EQ((Coordinate::FromLeptons(0xFFFF, 5) + Coordinate::FromLeptons(1, 0))
                .packed(),
            0x00050000U);
}

TEST(CoordinateTest, MidpointAveragesUnsignedComponents) {
  EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromLeptons(0xFFFF, 0),
                                 Coordinate::FromLeptons(1, 0)),
            Coordinate::FromLeptons(0x8000, 0));
  EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromLeptons(0, 0x100),
                                 Coordinate::FromLeptons(0, 0x301)),
            Coordinate::FromLeptons(0, 0x200));
}

// The order is the packed word's: Y first, then X.
TEST(CoordinateTest, OrdersByPackedWord) {
  EXPECT_LT(Coordinate::FromLeptons(0xFFFF, 1), Coordinate::FromLeptons(0, 2));
  EXPECT_LT(Coordinate::FromLeptons(1, 2), Coordinate::FromLeptons(2, 2));
  EXPECT_GT(Coordinate::FromLeptons(0, 0x8000), Coordinate::FromLeptons(0, 1));
  EXPECT_LE(kSample, kSample);
}

TEST(CoordinateTest, ExplicitBoolIsNonZero) {
  EXPECT_FALSE(static_cast<bool>(Coordinate{}));
  EXPECT_TRUE(static_cast<bool>(Coordinate::FromLeptons(1, 0)));
  EXPECT_TRUE(static_cast<bool>(Coordinate::FromLeptons(0, 1)));
}

TEST(CoordinateTest, SerializesThePackedWordLittleEndian) {
  std::vector<std::byte> bytes;
  VectorSink sink(bytes);
  // The call operator is non-const; misc-const-correctness misses the call
  // because its argument is template-dependent.
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveWriter writer(sink);
  Coordinate written = kSample;
  writer(written);
  ASSERT_TRUE(writer.ok());
  EXPECT_EQ(bytes, (std::vector<std::byte>{std::byte{0x78}, std::byte{0x56},
                                           std::byte{0x34}, std::byte{0x12}}));

  SpanSource source{std::span<const std::byte>(bytes)};
  // NOLINTNEXTLINE(misc-const-correctness)
  ArchiveReader reader(source);
  Coordinate read{};
  reader(read);
  EXPECT_EQ(std::string(reader.error()), "");
  EXPECT_EQ(read, kSample);
}

}  // namespace
}  // namespace ra
