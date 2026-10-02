// Pins the packed lepton encoding and its pixel conversions, including the
// truncation the original bit-fields performed on out-of-range components.
// Cell's layout is pinned by cell_test.cc, Coordinate's by
// coordinate_test.cc, Target's by ra/objects/target_test.cc.

#include "ra/core/packed.h"

#include <type_traits>

#include "gtest/gtest.h"
#include "ra/core/defines.h"

namespace ra {
namespace {

// The packers are usable in constant expressions.
static_assert(Make_Lepton(3, 0x80) == 0x0380);

// The type of a noexcept function returning R from Args.
template <class R, class... Args>
using NoexceptFn = R(Args...) noexcept;

// Leptons are plain ints at the interface; the 16-bit word exists only inside
// the packers, which wrap what they are given.
static_assert(std::is_same_v<decltype(Lepton_Cell), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_Fraction), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Make_Lepton), NoexceptFn<int, int, int>>);
static_assert(std::is_same_v<decltype(Cell_To_Lepton), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_To_Cell), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Lepton_To_Pixel), NoexceptFn<int, int>>);
static_assert(std::is_same_v<decltype(Pixel_To_Lepton), NoexceptFn<int, int>>);

TEST(PackedTest, LeptonCellConversions) {
  EXPECT_EQ(Cell_To_Lepton(3), 0x0300);
  EXPECT_EQ(Cell_To_Lepton(300), 0x2C00);
  EXPECT_EQ(Lepton_To_Cell(0x017F), 1);
  EXPECT_EQ(Lepton_To_Cell(0x0180), 2);
  EXPECT_EQ(Lepton_To_Cell(0xFF80), 256);
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

}  // namespace
}  // namespace ra
