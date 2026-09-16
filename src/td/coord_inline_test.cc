// Checks the packed COORDINATE and DirType helpers in td/inline.h against the
// byte pokes they replaced, which read the words in host (little-endian) order.

#include <array>
#include <cstdint>
#include <cstring>

#include "gtest/gtest.h"
#include "td/defines.h"
#include "td/inline.h"

namespace {

// The legacy implementations read COORDINATE through byte and word pointers.
uint8_t LegacyByte(COORDINATE coord, int index) {
  std::array<uint8_t, sizeof(coord)> bytes{};
  std::memcpy(bytes.data(), &coord, sizeof(coord));
  return bytes.at(static_cast<std::size_t>(index));
}

uint16_t LegacyWord(COORDINATE coord, int index) {
  std::array<uint16_t, 2> words{};
  std::memcpy(words.data(), &coord, sizeof(coord));
  return words.at(static_cast<std::size_t>(index));
}

COORDINATE LegacySnap(COORDINATE coord) {
  return static_cast<COORDINATE>(MakeLong(
      static_cast<uint16_t>((LegacyWord(coord, 1) & 0xFF00U) | 0x80U),
      static_cast<uint16_t>((LegacyWord(coord, 0) & 0xFF00U) | 0x80U)));
}

COORDINATE LegacyMid(COORDINATE coord1, COORDINATE coord2) {
  return static_cast<COORDINATE>(MakeLong(
      static_cast<uint16_t>((LegacyWord(coord1, 1) + LegacyWord(coord2, 1)) /
                            2),
      static_cast<uint16_t>((LegacyWord(coord1, 0) + LegacyWord(coord2, 0)) /
                            2)));
}

int LegacyDirDiff(DirType dir1, DirType dir2) {
  signed char first = 0;
  signed char second = 0;
  std::memcpy(&first, &dir1, 1);
  std::memcpy(&second, &dir2, 1);
  return second - first;
}

// A fixed spread of coordinates that sets every byte to low, middle and high
// values, including the sign bits of each word.
constexpr std::array<COORDINATE, 8> kCoords = {
    0x00000000, 0xFFFFFFFF, 0x01020304, 0x80FF7F00,
    0x12345678, 0xFEDCBA98, 0x00800080, 0x7F7F8181,
};

TEST(TdCoordInline, ComponentBytesMatchLegacyOffsets) {
  for (const COORDINATE coord : kCoords) {
    EXPECT_EQ(Coord_XLepton(coord), LegacyByte(coord, 0)) << coord;
    EXPECT_EQ(Coord_XCell(coord), LegacyByte(coord, 1)) << coord;
    EXPECT_EQ(Coord_YLepton(coord), LegacyByte(coord, 2)) << coord;
    EXPECT_EQ(Coord_YCell(coord), LegacyByte(coord, 3)) << coord;
  }
}

TEST(TdCoordInline, SnapAndMidMatchLegacyWords) {
  for (const COORDINATE coord1 : kCoords) {
    EXPECT_EQ(Coord_Snap(coord1), LegacySnap(coord1)) << coord1;
    for (const COORDINATE coord2 : kCoords) {
      EXPECT_EQ(Coord_Mid(coord1, coord2), LegacyMid(coord1, coord2))
          << coord1 << " " << coord2;
    }
  }
}

TEST(TdCoordInline, DirDiffMatchesSignedBytes) {
  for (int first = 0; first < 256; first += 5) {
    for (int second = 0; second < 256; second += 3) {
      const auto dir1 = static_cast<DirType>(first);
      const auto dir2 = static_cast<DirType>(second);
      EXPECT_EQ(Dir_Diff(dir1, dir2), LegacyDirDiff(dir1, dir2))
          << first << " " << second;
    }
  }
}

}  // namespace
