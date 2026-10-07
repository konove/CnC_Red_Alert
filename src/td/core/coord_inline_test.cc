// Checks SignedDirectionDifference against the signed byte poke it replaced.
// The cell and coordinate conversions this file used to pin are Cell's members
// now (cell_test.cc).

#include <cstring>
#include <span>

#include "base/buffer.h"
#include "game/sim/direction.h"
#include "gtest/gtest.h"

namespace td {

namespace {

int LegacyDirDiff(game::DirType dir1, game::DirType dir2) {
  signed char first = 0;
  signed char second = 0;
  base::CopyBytes(base::ObjectBytes(first), base::ObjectBytes(dir1), 1);
  base::CopyBytes(base::ObjectBytes(second), base::ObjectBytes(dir2), 1);
  return second - first;
}

TEST(TdCoordInline, DirDiffMatchesSignedBytes) {
  for (int first = 0; first < 256; first += 5) {
    for (int second = 0; second < 256; second += 3) {
      const game::DirType dir1 = game::AsDirection(first);
      const game::DirType dir2 = game::AsDirection(second);
      EXPECT_EQ(game::SignedDirectionDifference(dir1, dir2),
                LegacyDirDiff(dir1, dir2))
          << first << " " << second;
    }
  }
}

}  // namespace

}  // namespace td
