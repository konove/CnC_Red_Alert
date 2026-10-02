// Pins the packed CELL, lepton, COORDINATE and TARGET encodings, including the
// truncation the original bit-fields performed on out-of-range components.

#include "ra/core/packed.h"

#include <cstdint>
#include <type_traits>

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

constexpr COORDINATE kSample = 0x12345678;

// The packers are usable in constant expressions.
static_assert(XY_Cell(5, 3) == 389);
static_assert(Coord_Cell(kSample) == XY_Cell(0x56, 0x12));
static_assert(Build_Target(RTTI_NONE, 0) == kTargetNone);

// The type of a noexcept function returning R from Args.
template <class R, class... Args>
using NoexceptFn = R(Args...) noexcept;

// Leptons are plain ints at the interface; the 16-bit word exists only inside
// the packers, which wrap what they are given.
static_assert(
    std::is_same_v<decltype(XY_Coord), NoexceptFn<COORDINATE, int, int>>);
static_assert(std::is_same_v<decltype(Coord_X), NoexceptFn<int, COORDINATE>>);
static_assert(std::is_same_v<decltype(Coord_Y), NoexceptFn<int, COORDINATE>>);
static_assert(std::is_same_v<decltype(Lepton_Cell), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_Fraction), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Make_Lepton), NoexceptFn<int, int, int>>);
static_assert(std::is_same_v<decltype(Cell_To_Lepton), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_To_Cell), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_To_Pixel), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Pixel_To_Lepton), NoexceptFn<int, int>>);

TEST(PackedTest, CellPacksSevenBitsPerAxis) {
  EXPECT_EQ(XY_Cell(5, 3), (3 * 128) + 5);
  EXPECT_EQ(Cell_X(XY_Cell(5, 3)), 5);
  EXPECT_EQ(Cell_Y(XY_Cell(5, 3)), 3);
}

TEST(PackedTest, CellTruncatesOutOfRangeComponents) {
  EXPECT_EQ(XY_Cell(-1, 0), 127);
  EXPECT_EQ(XY_Cell(130, 1), 128 + 2);
}

TEST(PackedTest, SentinelCellsReadAsFullComponents) {
  EXPECT_EQ(Cell_X(-1), 127);
  EXPECT_EQ(Cell_Y(-1), 127);
  EXPECT_EQ(Cell_X(kRefreshEol), 127);
  EXPECT_EQ(Cell_Y(kRefreshEol), 127);
}

TEST(PackedTest, CoordinateComponents) {
  EXPECT_EQ(Coord_X(kSample), 0x5678);
  EXPECT_EQ(Coord_Y(kSample), 0x1234);
  EXPECT_EQ(Coord_XCell(kSample), 0x56);
  EXPECT_EQ(Coord_YCell(kSample), 0x12);
  EXPECT_EQ(Coord_XLepton(kSample), 0x78);
  EXPECT_EQ(Coord_YLepton(kSample), 0x34);
  EXPECT_EQ(XY_Coord(0x5678, 0x1234), kSample);
}

TEST(PackedTest, CoordinateRounding) {
  EXPECT_EQ(Coord_Snap(kSample), 0x12805680U);
  EXPECT_EQ(Coord_Fraction(kSample), 0x00340078U);
  EXPECT_EQ(Coord_Whole(kSample), 0x12005600U);
  EXPECT_EQ(CellCenterFraction(), 0x00800080U);
}

TEST(PackedTest, CellCoordinateRoundTrip) {
  EXPECT_EQ(Cell_Coord(XY_Cell(5, 3)), 0x03800580U);
  EXPECT_EQ(Coord_Cell(kSample), XY_Cell(0x56, 0x12));
  // An X cell byte of 128 is off the map and wraps to column 0.
  EXPECT_EQ(Coord_Cell(0x00008000U), XY_Cell(0, 0));
}

TEST(PackedTest, CoordinateArithmeticWrapsPerComponent) {
  EXPECT_EQ(Coord_Add(XY_Coord(0xFFFF, 1), XY_Coord(2, 0xFFFF)),
            XY_Coord(1, 0));
  EXPECT_EQ(Coord_Sub(XY_Coord(1, 0), XY_Coord(2, 0)), XY_Coord(0xFFFF, 0));
  EXPECT_EQ(Coord_Mid(XY_Coord(0xFFFF, 0), XY_Coord(1, 0)),
            XY_Coord(0x8000, 0));
}

TEST(PackedTest, LeptonCellConversions) {
  EXPECT_EQ(Cell_To_Lepton(3), 0x0300);
  EXPECT_EQ(Cell_To_Lepton(300), 0x2C00);
  EXPECT_EQ(Lepton_To_Cell(0x017F), 1);
  EXPECT_EQ(Lepton_To_Cell(0x0180), 2);
  EXPECT_EQ(Lepton_To_Cell(0xFF80), 256);
}

// Leptons are 16-bit words wherever they are packed: out-of-range components
// wrap on the way in, and every component read back is in 0..65535.
TEST(PackedTest, CoordinateComponentsWrapAtSixteenBits) {
  EXPECT_EQ(XY_Coord(-1, 0x10000), XY_Coord(0xFFFF, 0));
  EXPECT_EQ(XY_Coord(0x12345, -0x10000), XY_Coord(0x2345, 0));
  EXPECT_EQ(Coord_X(0xFFFFFFFFU), 0xFFFF);
  EXPECT_EQ(Coord_Y(0xFFFFFFFFU), 0xFFFF);
  EXPECT_GE(Coord_X(0xFFFFFFFFU), 0);
  EXPECT_GE(Coord_Y(0xFFFFFFFFU), 0);
}

TEST(PackedTest, LeptonBytesWrapAtSixteenBits) {
  EXPECT_EQ(Lepton_Cell(-1), 0xFF);
  EXPECT_EQ(Lepton_Fraction(-1), 0xFF);
  EXPECT_EQ(Lepton_Cell(0x10200), 2);
  EXPECT_EQ(Make_Lepton(-1, 300), 0xFF2C);
  EXPECT_EQ(Cell_To_Lepton(-1), 0xFF00);
  // -1 lepton is the word 0xFFFF: 255 cells and a fraction that rounds up.
  EXPECT_EQ(Lepton_To_Cell(-1), 256);
  EXPECT_EQ(Lepton_To_Cell(0x10180), 2);
}

TEST(PackedTest, LeptonPixelConversions) {
  EXPECT_EQ(Lepton_To_Pixel(0x0100), 24);
  EXPECT_EQ(Lepton_To_Pixel(0x10100), 24);
  // The word is read as signed: 0xFFFF is -1 lepton, 0xFE00 is -512.
  EXPECT_EQ(Lepton_To_Pixel(0xFFFF), 0);
  EXPECT_EQ(Lepton_To_Pixel(-1), 0);
  EXPECT_EQ(Lepton_To_Pixel(0xFE00), -47);
  EXPECT_EQ(Lepton_To_Pixel(-512), -47);
  EXPECT_EQ(Lepton_To_Pixel(0x8000), -3071);
  EXPECT_EQ(Pixel_To_Lepton(24), 0x0100);
  // Negative and oversized pixel counts wrap to a 16-bit word.
  EXPECT_EQ(Pixel_To_Lepton(-1), 0xFFF6);
  EXPECT_EQ(Pixel_To_Lepton(-24), 0xFF01);
  EXPECT_EQ(Pixel_To_Lepton(10000), 41131);
}

TEST(PackedTest, TargetPacksKindOverValue) {
  const TARGET target = Build_Target(RTTI_CELL, 389);
  EXPECT_EQ(static_cast<uint32_t>(target),
            (static_cast<uint32_t>(RTTI_CELL) << 24) | 389U);
  EXPECT_EQ(Target_Kind(target), RTTI_CELL);
  EXPECT_EQ(Target_Value(target), 389);
}

TEST(PackedTest, TargetTruncatesNegativeValues) {
  const TARGET target = Build_Target(RTTI_UNIT, -1);
  EXPECT_EQ(Target_Kind(target), RTTI_UNIT);
  EXPECT_EQ(Target_Value(target), 0xFFFFFF);
  EXPECT_EQ(Build_Target(RTTI_NONE, 0), kTargetNone);
}

}  // namespace
}  // namespace ra
