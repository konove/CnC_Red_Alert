// Tests what losing and regaining the window's focus does to the music.

#include <SDL_audio.h>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "base/buffer.h"
#include "base/fixed.h"
#include "engine/audio/audio_mixer.h"
#include "engine/codec/aud_decoder.h"
#include "gtest/gtest.h"
#include "ra/assets/theme.h"
#include "ra/core/defines.h"
#include "ra/screens/goptions.h"
#include "ra/startup/game_state.h"
#include "ra/startup/game_test_util.h"
#include "ra/startup/winstub.h"

namespace ra {
namespace {

constexpr int kRate = 22050;

// Returns the bytes of `object` as file contents.
template <typename T>
std::string Bytes(const T& object) {
  std::string out;
  for (const std::byte byte : base::ObjectBytes(object)) {
    out.push_back(static_cast<char>(byte));
  }
  return out;
}

// A silent ADPCM score about ten seconds long, far more than a test plays.
std::string SilentScore() {
  constexpr int kBlocks = 2000;
  constexpr int kPayloadBytes = 64;
  constexpr int kDecodedBytes = kPayloadBytes * 4;
  std::string aud;
  aud += Bytes(AudHeader{.sample_rate = kRate,
                         .compressed_bytes = 0,
                         .uncompressed_bytes = kBlocks * kDecodedBytes,
                         .flags = kAudFlag16Bit,
                         .compression = static_cast<uint8_t>(SCOMP_SOS)});
  for (int block = 0; block < kBlocks; block++) {
    aud += Bytes(AudBlockHeader{.compressed_bytes = kPayloadBytes,
                                .decoded_bytes = kDecodedBytes,
                                .magic = 0x0000DEAF});
    aud.append(std::size_t{kPayloadBytes}, '\0');
  }
  return aud;
}

// A Game whose mixer plays into SDL's dummy device, with a score on disk.
class FocusTest : public GameFixture {
 public:
  FocusTest() {
    SDL_AudioInit("dummy");
    WriteGameFile("BIGF226M.AUD", SilentScore());
    TheGameState().scores_present() = true;
    TheOptions().ScoreVolume = Fixed(1, 2);
  }

  ~FocusTest() override {
    engine::audio::TheAudio().Close();
    SDL_AudioQuit();
  }

  FocusTest(const FocusTest&) = delete;
  FocusTest& operator=(const FocusTest&) = delete;
  FocusTest(FocusTest&&) = delete;
  FocusTest& operator=(FocusTest&&) = delete;
};

TEST_F(FocusTest, LosingTheFocusKeepsTheSongWhereItWas) {
  ASSERT_TRUE(engine::audio::TheAudio().Open(kRate, /*stereo=*/false));
  ASSERT_NE(TheTheme().Play_Song(THEME_BIGF), -1);

  Focus_Loss();
  // A stopped song would be started again from its first note.
  EXPECT_TRUE(TheTheme().Still_Playing());
  EXPECT_EQ(TheTheme().What_Is_Playing(), THEME_BIGF);

  Focus_Restore();
  TheTheme().AI();
  EXPECT_TRUE(TheTheme().Still_Playing());
  EXPECT_EQ(TheTheme().What_Is_Playing(), THEME_BIGF);
}

}  // namespace
}  // namespace ra
