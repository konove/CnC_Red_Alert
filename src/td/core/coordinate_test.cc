// Pins Coordinate against the byte and word pokes TD's free coordinate helpers
// replaced, plus golden values for the signed component reading and the
// 16-bit wrap of each component.

#include "td/core/coordinate.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "base/buffer.h"
#include "engine/stream/archive.h"
#include "engine/stream/span_source.h"
#include "engine/stream/vector_sink.h"
#include "gtest/gtest.h"

namespace td {
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

// The legacy implementations read a coordinate through byte and word pointers,
// in host (little-endian) order.
uint8_t LegacyByte(uint32_t packed, int index) {
  std::array<uint8_t, sizeof(packed)> bytes{};
  base::CopyBytes(std::as_writable_bytes(std::span(bytes)),
                  base::ObjectBytes(packed), sizeof(packed));
  return bytes.at(static_cast<std::size_t>(index));
}

uint16_t LegacyWord(uint32_t packed, int index) {
  std::array<uint16_t, 2> words{};
  base::CopyBytes(std::as_writable_bytes(std::span(words)),
                  base::ObjectBytes(packed), sizeof(packed));
  return words.at(static_cast<std::size_t>(index));
}

uint32_t LegacySnap(uint32_t packed) {
  return (uint32_t{
              static_cast<uint16_t>((LegacyWord(packed, 1) & 0xFF00U) | 0x80U)}
          << 16U) |
         static_cast<uint16_t>((LegacyWord(packed, 0) & 0xFF00U) | 0x80U);
}

uint32_t LegacyMid(uint32_t packed1, uint32_t packed2) {
  return (uint32_t{static_cast<uint16_t>(
              (LegacyWord(packed1, 1) + LegacyWord(packed2, 1)) / 2)}
          << 16U) |
         static_cast<uint16_t>(
             (LegacyWord(packed1, 0) + LegacyWord(packed2, 0)) / 2);
}

// A fixed spread of packed words that sets every byte to low, middle and high
// values, including the sign bits of each word.
constexpr std::array<uint32_t, 8> kWords = {
    0x00000000, 0xFFFFFFFF, 0x01020304, 0x80FF7F00,
    0x12345678, 0xFEDCBA98, 0x00800080, 0x7F7F8181,
};

TEST(TdCoordinateTest, DefaultIsZero) {
  EXPECT_EQ(Coordinate{}.packed(), 0U);
  EXPECT_EQ(Coordinate{}, Coordinate::FromLeptons(0, 0));
}

TEST(TdCoordinateTest, Components) {
  EXPECT_EQ(kSample.packed(), 0x12345678U);
  EXPECT_EQ(kSample.x(), 0x5678);
  EXPECT_EQ(kSample.y(), 0x1234);
  EXPECT_EQ(kSample.cell_x(), 0x56);
  EXPECT_EQ(kSample.cell_y(), 0x12);
  EXPECT_EQ(kSample.sub_x(), 0x78);
  EXPECT_EQ(kSample.sub_y(), 0x34);
}

TEST(TdCoordinateTest, ComponentBytesMatchLegacyOffsets) {
  for (const uint32_t packed : kWords) {
    const Coordinate coord = Coordinate::FromPacked(packed);
    EXPECT_EQ(coord.sub_x(), LegacyByte(packed, 0)) << packed;
    EXPECT_EQ(coord.cell_x(), LegacyByte(packed, 1)) << packed;
    EXPECT_EQ(coord.sub_y(), LegacyByte(packed, 2)) << packed;
    EXPECT_EQ(coord.cell_y(), LegacyByte(packed, 3)) << packed;
    EXPECT_EQ(coord.x(), static_cast<int16_t>(LegacyWord(packed, 0))) << packed;
    EXPECT_EQ(coord.y(), static_cast<int16_t>(LegacyWord(packed, 1))) << packed;
  }
}

// TD reads each lepton word as a signed 16-bit value, as its original code
// read it through a short.
TEST(TdCoordinateTest, ComponentsReadSigned) {
  constexpr Coordinate kSigned = Coordinate::FromPacked(0x8000FFFFU);
  EXPECT_EQ(kSigned.x(), -1);
  EXPECT_EQ(kSigned.y(), -32768);
  EXPECT_EQ(kSigned.cell_x(), 0xFF);
  EXPECT_EQ(kSigned.cell_y(), 0x80);
  EXPECT_EQ(kSigned.sub_x(), 0xFF);
  EXPECT_EQ(kSigned.sub_y(), 0x00);
  constexpr Coordinate kFull = Coordinate::FromPacked(0xFFFFFFFFU);
  EXPECT_EQ(kFull.x(), -1);
  EXPECT_EQ(kFull.y(), -1);
  EXPECT_EQ(Coordinate::FromPacked(0x7FFF7FFFU).x(), 32767);
}

TEST(TdCoordinateTest, Rounding) {
  EXPECT_EQ(kSample.Snapped().packed(), 0x12805680U);
  EXPECT_EQ(kSample.Fraction().packed(), 0x00340078U);
  EXPECT_EQ(kSample.Whole().packed(), 0x12005600U);
  for (const uint32_t packed : kWords) {
    const Coordinate coord = Coordinate::FromPacked(packed);
    EXPECT_EQ(coord.Snapped().packed(), LegacySnap(packed)) << packed;
    EXPECT_EQ(coord.Whole().packed(), packed & 0xFF00FF00U) << packed;
    EXPECT_EQ(coord.Fraction().packed(), packed & 0x00FF00FFU) << packed;
  }
}

// Leptons are 16-bit words wherever they are packed: out-of-range components
// wrap on the way in.
TEST(TdCoordinateTest, ComponentsWrapAtSixteenBits) {
  EXPECT_EQ(Coordinate::FromLeptons(-1, 0x10000),
            Coordinate::FromLeptons(0xFFFF, 0));
  EXPECT_EQ(Coordinate::FromLeptons(0x12345, -0x10000),
            Coordinate::FromLeptons(0x2345, 0));
  EXPECT_EQ(Coordinate::FromLeptons(-1, -32768).packed(), 0x8000FFFFU);
}

TEST(TdCoordinateTest, ArithmeticWrapsPerComponent) {
  EXPECT_EQ(
      Coordinate::FromLeptons(0xFFFF, 1) + Coordinate::FromLeptons(2, 0xFFFF),
      Coordinate::FromLeptons(1, 0));
  EXPECT_EQ(Coordinate::FromLeptons(1, 0) - Coordinate::FromLeptons(2, 0),
            Coordinate::FromLeptons(0xFFFF, 0));
  // A carry out of the X word does not reach Y.
  EXPECT_EQ((Coordinate::FromLeptons(0xFFFF, 5) + Coordinate::FromLeptons(1, 0))
                .packed(),
            0x00050000U);
  // A borrow out of the X word does not reach Y either.
  EXPECT_EQ(
      (Coordinate::FromLeptons(0, 5) - Coordinate::FromLeptons(1, 0)).packed(),
      0x0005FFFFU);
  // Signed overflow of a component wraps like the unsigned word.
  EXPECT_EQ((Coordinate::FromLeptons(0x7FFF, 0x8000) +
             Coordinate::FromLeptons(1, 0xFFFF))
                .packed(),
            0x7FFF8000U);
}

TEST(TdCoordinateTest, MidpointAveragesUnsignedWords) {
  EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromLeptons(0xFFFF, 0),
                                 Coordinate::FromLeptons(1, 0)),
            Coordinate::FromLeptons(0x8000, 0));
  EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromLeptons(0, 0x100),
                                 Coordinate::FromLeptons(0, 0x301)),
            Coordinate::FromLeptons(0, 0x200));
  // Words above 0x8000 average as unsigned words, rounding down: a signed
  // average would give 0xC000 and 0xFFFF here.
  EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromLeptons(0x8000, 0xFFFF),
                                 Coordinate::FromLeptons(0, 0xFFFE))
                .packed(),
            0xFFFE4000U);
  for (const uint32_t packed1 : kWords) {
    for (const uint32_t packed2 : kWords) {
      EXPECT_EQ(Coordinate::Midpoint(Coordinate::FromPacked(packed1),
                                     Coordinate::FromPacked(packed2))
                    .packed(),
                LegacyMid(packed1, packed2))
          << packed1 << " " << packed2;
    }
  }
}

TEST(TdCoordinateTest, ExplicitBoolIsNonZero) {
  EXPECT_FALSE(static_cast<bool>(Coordinate{}));
  EXPECT_TRUE(static_cast<bool>(Coordinate::FromLeptons(1, 0)));
  EXPECT_TRUE(static_cast<bool>(Coordinate::FromLeptons(0, 1)));
}

TEST(TdCoordinateTest, SerializesThePackedWordLittleEndian) {
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
}  // namespace td
