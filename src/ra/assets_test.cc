// Tests the initial state of Assets and what it reports with no data loaded.

#include "ra/assets.h"

#include <span>

#include "base/installed.h"
#include "gtest/gtest.h"
#include "magic_enum/magic_enum.hpp"
#include "ra/defines.h"

// search_paths.cc, reached through GameFile, calls the game's CD check. The
// tests never look for a disc.
// NOLINTBEGIN(misc-use-internal-linkage): satisfies search_paths.cc's extern.
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
  EXPECT_TRUE(assets.debug_strings().empty());
  EXPECT_TRUE(assets.lightning_shapes().empty());
}

TEST(AssetsTest, TutorialTextIsNullUntilItIsLoaded) {
  const Assets assets;
  EXPECT_EQ(assets.tutorial_text(0), nullptr);
  EXPECT_EQ(assets.tutorial_text(Assets::kTutorialTextCount - 1), nullptr);
}

TEST(AssetsTest, TutorialTextRejectsAnIndexOutsideTheTable) {
  const Assets assets;
  EXPECT_EQ(assets.tutorial_text(-1), nullptr);
  EXPECT_EQ(assets.tutorial_text(Assets::kTutorialTextCount), nullptr);
}

TEST(AssetsTest, SpeechSlotsAreSizedAndEmptyFromTheStart) {
  Assets assets;
  const std::span<Assets::SpeechSlot> slots = assets.speech_slots();
  ASSERT_EQ(slots.size(), Assets::kSpeechSlotCount);
  for (const Assets::SpeechSlot& slot : slots) {
    EXPECT_EQ(slot.voice, VOX_NONE);
    EXPECT_EQ(slot.buffer.size(), kSpeechBufferSize);
  }
}

TEST(AssetsTest, DiscArchivesStartUnregistered) {
  Assets assets;
  EXPECT_EQ(assets.disc_archives().main, nullptr);
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
