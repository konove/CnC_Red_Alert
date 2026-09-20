// Tests the initial state of Assets and what it reports with no data loaded.

#include "td/assets.h"

#include <cstddef>
#include <vector>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "td/defines.h"

// LoadFonts() and LoadStrings() are not run here; these stand in for the game
// code they call, which would pull in most of the engine.
class File;
// NOLINTBEGIN(misc-use-internal-linkage): these satisfy other units' externs.
std::vector<std::byte> LoadAllocData(File& file);
std::vector<std::byte> LoadAllocData(File& /*file*/) { return {}; }
const char* Language_Name(const char* basename);
const char* Language_Name(const char* /*basename*/) { return "CONQUER.ENG"; }
int Get_CD_Index(int cd_drive, int timeout);
int Get_CD_Index(int /*cd_drive*/, int /*timeout*/) { return -1; }
// NOLINTEND(misc-use-internal-linkage)

namespace {

TEST(AssetsTest, NothingIsLoadedBeforeTheLoadersRun) {
  const Assets assets;
  for (const FontType type : magic_enum::enum_values<FontType>()) {
    EXPECT_TRUE(assets.font(type).empty()) << magic_enum::enum_name(type);
  }
  EXPECT_TRUE(assets.system_strings().empty());
}

TEST(AssetsTest, TheSpeechBufferIsSizedFromTheStart) {
  Assets assets;
  EXPECT_EQ(assets.speech_buffer().size(), SPEECH_BUFFER_SIZE);
}

TEST(AssetsTest, DiscArchivesStartUnregistered) {
  Assets assets;
  EXPECT_EQ(assets.disc_archives().general, nullptr);
  EXPECT_EQ(assets.disc_archives().movies, nullptr);
  EXPECT_EQ(assets.disc_archives().score, nullptr);
}

TEST(AssetsTest, TheAssetsReturnsTheInstalledAssets) {
  Assets assets;
  const base::Installed<Assets>::Scope scope(assets);
  EXPECT_EQ(&TheAssets(), &assets);
}

}  // namespace
