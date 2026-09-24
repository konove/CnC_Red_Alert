// Tests for the VQA player: configuration defaults, player lifecycle, the
// OpenVqa() validation/error paths, chunk loading into the play buffers and
// drawer placement. Movies are small synthetic files served by a scripted
// in-memory VqaIo file source. No real movie assets are required.

#include "winvq/vqa32/vqa_player.h"

#include <SDL_audio.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "base/array.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/lcw_buffer.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqa32/vqa_test_util.h"

// Link-time stub for the game's palette hook, which records what it was
// handed.

// A copy of the palette last passed to QueueVqaPalette(), for the drawer
// tests. The stub is a free function, so it cannot reach a fixture member.
static std::vector<uint8_t>& QueuedPalette() {
  static std::vector<uint8_t> palette;
  return palette;
}

void QueueVqaPalette(std::span<uint8_t> palette, int32_t numbytes,
                     uint32_t /*slowpal*/) {
  QueuedPalette().assign(
      palette.begin(),
      palette.begin() + std::min<base::ssize>(numbytes, std::ssize(palette)));
}

namespace {

class VqaPlayTest : public testing::Test {
 protected:
  void SetUp() override {
    player_.SetIo(&fake_);

    // Audio and drawing stay off: the tests run headless and only exercise
    // the file validation logic.
    SetVqaConfigDefaults(&config_);
    config_.option_flags = 0;
    config_.draw_flags = kVqaDrawNothing;
  }

  FakeVqaIo fake_;
  VqaPlayer player_;
  VqaConfig config_{};
};

TEST(VqaConfigTest, DefaultConfigHasDocumentedDefaults) {
  VqaConfig config;
  SetVqaConfigDefaults(&config);

  EXPECT_EQ(config.image_width, 320);
  EXPECT_EQ(config.image_height, 200);
  EXPECT_EQ(config.margin_x, -1);
  EXPECT_EQ(config.margin_y, -1);
  EXPECT_EQ(config.frame_rate, -1);  // -1 means use the movie's frame rate.
  EXPECT_EQ(config.draw_flags, 0);
  EXPECT_EQ(config.option_flags, kVqaOptionAudio);
  EXPECT_EQ(config.frame_buffer_count, 6);
  EXPECT_EQ(config.codebook_buffer_count, 3);
}

TEST_F(VqaPlayTest, OpenReportsOpenErrorWhenHandlerCannotOpen) {
  fake_.fail_open = true;

  EXPECT_EQ(player_.Open("missing.vqa", &config_), kVqaErrorOpen);
  EXPECT_EQ(fake_.opens, 1);
  // The file never opened, so the player must not try to close it.
  EXPECT_EQ(fake_.closes, 0);
}

TEST_F(VqaPlayTest, OpenReportsReadErrorAndClosesOnEmptyFile) {
  // No data at all: the first 8-byte header read fails.
  EXPECT_EQ(player_.Open("empty.vqa", &config_), kVqaErrorRead);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsNonIffFile) {
  AppendBytes(fake_.data, "XXXX");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(player_.Open("notiff.vqa", &config_), kVqaErrorNotVqa);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsFormWithZeroSize) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(player_.Open("zerosize.vqa", &config_), kVqaErrorNotVqa);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsFormWithoutWvqaId) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(player_.Open("notvqa.vqa", &config_), kVqaErrorNotVqa);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenReportsReadErrorWhenTruncatedAfterPreamble) {
  fake_.data = ValidPreamble();

  EXPECT_EQ(player_.Open("truncated.vqa", &config_), kVqaErrorRead);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsHeaderChunkWithWrongSize) {
  fake_.data = ValidPreamble();
  AppendBytes(fake_.data, "VQHD");
  AppendBigEndian32(fake_.data, 4);  // Real VQA headers are much larger.
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(player_.Open("badheader.vqa", &config_), kVqaErrorNotVqa);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, IoHandlerSurvivesFailedOpen) {
  // A failed open runs CloseVqa(), which resets the player state. The
  // installed io object must survive the reset so the player can be reused.
  ASSERT_EQ(player_.Open("empty.vqa", &config_), kVqaErrorRead);

  fake_.data = ValidPreamble();
  fake_.pos = 0;
  EXPECT_EQ(player_.Open("second.vqa", &config_), kVqaErrorRead);
  EXPECT_EQ(fake_.opens, 2);
  EXPECT_EQ(fake_.closes, 2);
}

constexpr int kCodebookCapacity = 376;
// Drives the private loader directly so tests can inspect the play buffers.
// One frame buffer means OpenVqa() primes exactly one frame.
class VqaLoaderTest : public testing::Test {
 protected:
  void SetUp() override {
    state_.io = &fake_;
    SetVqaConfigDefaults(&config_);
    config_.option_flags = 0;
    config_.draw_flags = kVqaDrawNothing;
    config_.frame_buffer_count = 1;
    config_.codebook_buffer_count = 1;
  }

  void TearDown() override {
    if (state_.movie != nullptr) {
      CloseVqa(&state_);
    }
  }

  int32_t Open() { return OpenVqa(&state_, "test.vqa", &config_); }

  // Turns the sound on, played through device_.
  void EnableAudio() {
    config_.option_flags |= kVqaOptionAudio;
    config_.audio_device = &device_;
  }

  FakeVqaIo fake_;
  FakeVqaAudioDevice device_;
  VqaPlayerState state_;
  VqaConfig config_{};
};

TEST_F(VqaLoaderTest, OpensMinimalMovie) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), 0);
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(VqaLoaderTest, SkipsTheFrameTable) {
  // Eight entries for a three-frame movie.
  fake_.data =
      MovieStart(SmallHeader(), {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  // The table was skipped whole, so the frame after it loaded.
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(VqaLoaderTest, RejectsFinfBeforeHeader) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "FINF", FinfPayload({0, 0, 0}));

  EXPECT_EQ(Open(), kVqaErrorNotVqa);
}

TEST_F(VqaLoaderTest, RejectsHeaderWithZeroGroupsize) {
  VqaHeader header = SmallHeader();
  header.frames_per_group = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorNotVqa);
}

TEST_F(VqaLoaderTest, RejectsHeaderWithZeroBlockSize) {
  VqaHeader header = SmallHeader();
  header.block_width = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorNotVqa);
}

TEST_F(VqaLoaderTest, RejectsPreFrameChunkOf2GiB) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "VQHD", HeaderPayload(SmallHeader()));
  // 2^31 reads back as a negative int32_t size.
  AppendChunk(fake_.data, "XXXX", 0x80000000U, {});

  EXPECT_EQ(Open(), kVqaErrorNotVqa);
}

TEST_F(VqaLoaderTest, RejectsFrameChunkOf2GiB) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "VQFR", 0x80000000U, {});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

TEST_F(VqaLoaderTest, RejectsChunkOf2GiBInsideFrame) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  std::vector<uint8_t> frame;
  AppendChunk(frame, "CBF0", 0xFFFFFFF8U, {});
  AppendChunk(fake_.data, "VQFR", frame);
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

TEST_F(VqaLoaderTest, KeyFrameContainerMarksAKeyFrame) {
  // The container's own ID says key frame; the chunks inside it do not.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  std::vector<uint8_t> frame;
  AppendFrameEnd(frame);
  AppendChunk(fake_.data, "VQFK", frame);

  ASSERT_EQ(Open(), 0);
  EXPECT_TRUE(state_.movie->ring.draw_frame().key);
}

TEST_F(VqaLoaderTest, PartialCompressedCodebookLoadsAtEstimatedOffset) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20, 0xAB));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  // Groupsize 1: offset = codebook_capacity - (20 * 1 + 100).
  const LcwBuffer& codebook =
      state_.movie->ring.codebook(state_.movie->loader->full_codebook()).data;
  EXPECT_TRUE(codebook.compressed());
  EXPECT_EQ(base::At(codebook.data(), kCodebookCapacity - 121), 0);
  EXPECT_EQ(base::At(codebook.data(), kCodebookCapacity - 120), 0xAB);
}

TEST_F(VqaLoaderTest, FullCodebookDiscardsCollectedPieces) {
  // Groupsize 2: a piece of the next codebook, then a full codebook, in frame
  // 0. The two pieces in frames 1 and 2 must then make the next codebook from
  // its start, not after the discarded piece.
  config_.frame_buffer_count = 3;
  config_.codebook_buffer_count = 2;
  VqaHeader header = SmallHeader();
  header.frames_per_group = 2;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xAA));
  AppendChunk(fake_.data, "CBF0", std::vector<uint8_t>(128));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xBB));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CBP0", std::vector<uint8_t>(20, 0xCC));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  const LcwBuffer& codebook =
      state_.movie->ring.codebook(state_.movie->loader->full_codebook()).data;
  ASSERT_EQ(codebook.size(), 40);
  EXPECT_EQ(base::At(codebook.contents(), 0), 0xBB);
  EXPECT_EQ(base::At(codebook.contents(), 20), 0xCC);
}

TEST_F(VqaLoaderTest, RejectsPartialCodebookWithNegativeOffset) {
  // 300 bytes fit the 376-byte codebook, but the estimated start
  // 376 - (300 * 1 + 100) = -24 lies before the buffer.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(300));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

TEST_F(VqaLoaderTest, RejectsPartialCodebooksOverflowingTheEnd) {
  // Groupsize 2: the first 20-byte part sets the offset to
  // 376 - (20 * 2 + 100) = 236; a 200-byte second part would end at 456.
  VqaHeader header = SmallHeader();
  header.frames_per_group = 2;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20));
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(200));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

TEST_F(VqaLoaderTest, AcceptsFullPalette) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 7));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  const Frame& frame = state_.movie->ring.draw_frame();
  EXPECT_TRUE(frame.has_palette);
  EXPECT_EQ(frame.palette.size(), 768);
  EXPECT_EQ(base::At(frame.palette.contents(), 767), 7);
}

TEST_F(VqaLoaderTest, SkippedFramePaletteIsSetWithTheNextFrameDrawn) {
  // Frames 0 and 1 carry palettes of 7s and 9s, frame 2 none. With the clock
  // at frame 2 the drawer skips 0 and 1 and draws 2, which must set the
  // palette of frame 1, the last one skipped.
  config_.frame_buffer_count = 3;
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 7));
  AppendFrameEnd(fake_.data);
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 9));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  // A buffer to draw into; without kVqaDrawToBuffer nothing is decoded.
  std::vector<unsigned char> image(size_t{320} * 200);
  config_.image_buffer = image;
  ASSERT_EQ(Open(), 0);
  QueuedPalette().clear();

  ConfigureDrawer(&state_);
  // 15 fps: tick 9 of 60 per second is frame 2, with 50 ms to spare before
  // frame 3 comes due.
  state_.movie->clock.Set(9, nullptr);
  ASSERT_EQ(DrawNextFrame(&state_), 0);

  EXPECT_EQ(state_.movie->drawer.last_drawn_frame, 2);
  ASSERT_EQ(QueuedPalette().size(), 768U);
  EXPECT_EQ(QueuedPalette().front(), 9);
}

TEST_F(VqaLoaderTest, AnAudioUnderrunLetsTheDrawerSkip) {
  config_.frame_buffer_count = 3;
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  config_.draw_flags = kVqaDrawNoSkip;
  ASSERT_EQ(Open(), 0);
  ConfigureDrawer(&state_);
  // 15 fps: tick 9 of 60 per second is frame 2.
  state_.movie->clock.Set(9, nullptr);

  // Frame 0 is late, but skipping is off.
  ASSERT_EQ(DrawNextFrame(&state_), 0);
  EXPECT_EQ(state_.movie->drawer.last_drawn_frame, 0);

  // The sound runs dry: nothing follows the block playing.
  state_.movie->audio->Advance();
  ASSERT_TRUE(state_.movie->audio->underran());

  // Now frame 1 is skipped for frame 2, the one due.
  ASSERT_EQ(DrawNextFrame(&state_), 0);
  EXPECT_EQ(state_.movie->drawer.last_drawn_frame, 2);
}

TEST_F(VqaLoaderTest, RejectsUncompressedPaletteOver256Colors) {
  // 800 bytes fit the frame's 1792-byte palette buffer, but not the 768-byte
  // drawer palette the first palette is copied into.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(800));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

// Each chunk type is 2000 bytes, larger than its destination buffer.
class VqaOversizedChunkTest : public VqaLoaderTest,
                              public testing::WithParamInterface<const char*> {
};

TEST_P(VqaOversizedChunkTest, RejectsChunkLargerThanItsBuffer) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, GetParam(), std::vector<uint8_t>(2000));
  // Without the bounds check the frame would load, so Open() would succeed.
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

INSTANTIATE_TEST_SUITE_P(AllBufferedChunks, VqaOversizedChunkTest,
                         testing::Values("CBF0", "CBFZ", "CBP0", "CBPZ",
                                         "CPL0", "CPLZ", "VPT0", "VPTZ"));

TEST_F(VqaLoaderTest, OpensMovieShorterThanFrameBuffers) {
  config_.frame_buffer_count = 3;
  VqaHeader header = SmallHeader();
  header.frame_count = 1;
  fake_.data = MovieStart(header, {0});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  EXPECT_EQ(state_.movie->loader->next_frame_number(), 1);
}

TEST_F(VqaLoaderTest, TruncatedMovieStillFailsToOpen) {
  // The header promises three frames but the file holds one.
  config_.frame_buffer_count = 3;
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), kVqaErrorRead);
}

TEST_F(VqaLoaderTest, RejectsAudioWithZeroBlockBytes) {
  VqaHeader header = SmallHeader();
  header.flags = kVqaHasAudio;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);
  config_.option_flags = kVqaOptionAudio;
  config_.audio_block_bytes = 0;

  EXPECT_EQ(Open(), kVqaErrorAudio);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaLoaderTest, PlaysSilentlyWithoutAnAudioRing) {
  VqaHeader header = SmallSoundHeader();
  header.frame_count = 1;
  fake_.data = EmptyFrames(header);
  EnableAudio();
  config_.audio_buffer_bytes = 0;
  ASSERT_EQ(Open(), 0);
  // No sound path runs: there is nothing to play the sound with.
  EXPECT_EQ(state_.config.option_flags & kVqaOptionAudio, 0U);
  EXPECT_EQ(state_.movie->audio_output, nullptr);

  // A pause and resume must not start the sound either.
  EXPECT_EQ(PlayVqa(&state_, kVqaModePause), kVqaPaused);
  EXPECT_EQ(PlayVqa(&state_, kVqaModeRun), kVqaEndOfMovie);
  EXPECT_FALSE(device_.attached());
}

TEST_F(VqaLoaderTest, WalkKeepsTheSoundPlayingUntilTheEnd) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // More than one frame's staging buffer, so it loads straight into the
  // audio ring as its first two blocks and the sound starts with the movie.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  ASSERT_EQ(Open(), 0);

  // Each walk moves one frame on; the sound, and the clock that follows it,
  // must keep running between them.
  PlayVqa(&state_, kVqaModeWalk);
  EXPECT_TRUE(state_.movie->audio_output->playing());
  PlayVqa(&state_, kVqaModeWalk);
  EXPECT_TRUE(state_.movie->audio_output->playing());
  EXPECT_TRUE(device_.attached());

  // The walk that reaches the end stops it.
  int32_t result = 0;
  for (int i = 0; i < 10 && result != kVqaEndOfMovie; ++i) {
    result = PlayVqa(&state_, kVqaModeWalk);
  }
  EXPECT_EQ(result, kVqaEndOfMovie);
  EXPECT_FALSE(state_.movie->audio_output->playing());
  EXPECT_FALSE(device_.attached());
}

TEST_F(VqaLoaderTest, FailsToOpenWhenTheSoundCannotBeConverted) {
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  // Not an SDL sample format, so SDL cannot convert to it.
  device_.audio_spec.format = 0;

  EXPECT_EQ(Open(), kVqaErrorAudio);
  EXPECT_FALSE(device_.attached());
}

TEST_F(VqaLoaderTest, FailsToOpenWithSoundButNoDevice) {
  fake_.data = EmptyFrames(SmallSoundHeader());
  config_.option_flags |= kVqaOptionAudio;

  EXPECT_EQ(Open(), kVqaErrorAudio);
}

TEST_F(VqaLoaderTest, RingOfPartBlocksWrapsAfterTheLastWholeBlock) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // Loads straight into the ring as its two whole blocks.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  // Two 2048-byte blocks and 904 bytes that are not one.
  config_.audio_buffer_bytes = 5000;
  ASSERT_EQ(Open(), 0);
  const AudioRing& audio = *state_.movie->audio;
  EXPECT_EQ(audio.block_count(), 2);
  EXPECT_EQ(audio.capacity(), 2 * 2048);
  ASSERT_TRUE(state_.movie->audio_output->Start());

  // The device pulls the preloaded blocks through the mixer.
  std::array<std::byte, 256> buffer{};
  for (int i = 0; i < 1000 && audio.play_block() != 1; ++i) {
    device_.Pump(buffer);
  }
  EXPECT_EQ(audio.play_block(), 1);
}

TEST_F(VqaLoaderTest, FirstSoundChunkFillingTheRingWrapsTheWriteOffset) {
  fake_.data = MovieStart(SmallSoundHeader(), {0, 0, 0});
  // Larger than staging, so it preloads the ring, and exactly as large.
  AppendChunk(fake_.data, "SND0", std::vector<uint8_t>(4096, 0x80));
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  AppendFrameEnd(fake_.data);
  EnableAudio();
  config_.audio_buffer_bytes = 2 * 2048;
  ASSERT_EQ(Open(), 0);
  const AudioRing& audio = *state_.movie->audio;
  EXPECT_EQ(audio.write_offset(), 0);
  EXPECT_TRUE(audio.block_loaded(0));
  EXPECT_TRUE(audio.block_loaded(1));
}

TEST_F(VqaLoaderTest, PlaysToTheEndWithoutAnImageBuffer) {
  // The defaults center the image in a 320x200 buffer, but with
  // kVqaDrawToBuffer clear there is no buffer at all.
  fake_.data = EmptyFrames(SmallHeader());
  config_.draw_flags = 0;
  config_.option_flags = kVqaOptionStep;
  ASSERT_EQ(Open(), 0);
  ASSERT_TRUE(state_.movie->drawer.image_buffer.empty());

  int32_t result = 0;
  for (int i = 0; i < 20 && result != kVqaEndOfMovie; ++i) {
    result = PlayVqa(&state_, kVqaModeWalk);
  }
  EXPECT_EQ(result, kVqaEndOfMovie);
}

TEST_F(VqaLoaderTest, RejectsNegativeBufferCounts) {
  fake_.data = EmptyFrames(SmallHeader());
  config_.frame_buffer_count = -1;
  EXPECT_EQ(Open(), kVqaErrorNoMemory);

  config_.frame_buffer_count = 1;
  config_.codebook_buffer_count = -1;
  EXPECT_EQ(Open(), kVqaErrorNoMemory);
}

TEST_F(VqaLoaderTest, StopEndsPlaybackWithoutLoadingTheRest) {
  fake_.data = EmptyFrames(SmallHeader());
  ASSERT_EQ(Open(), 0);

  EXPECT_EQ(PlayVqa(&state_, kVqaModeStop), kVqaEndOfMovie);
  EXPECT_EQ(state_.movie->loader->next_frame_number(), 1);
}

// Places the 8x8 SmallHeader() image in a 320x200 buffer, gap_x pixels
// horizontally and gap_y vertically from the corner named by origin.
VqaDrawer PlaceImage(uint32_t origin, int gap_x = 10, int gap_y = 20) {
  VqaPlayerState state;
  state.movie = std::make_unique<VqaMovie>(FrameRing(1, 1, 1, 1, 1));
  state.movie->drawer.image_width = 320;
  state.movie->drawer.image_height = 200;
  state.movie->drawer.y2 = 12345;  // Stale value the placement must not read.
  state.header = SmallHeader();
  state.config.margin_x = gap_x;
  state.config.margin_y = gap_y;
  state.config.draw_flags = origin;

  ConfigureDrawer(&state);

  return state.movie->drawer;
}

TEST(VqaDrawerTest, TopLeftOrigin) {
  const VqaDrawer drawer = PlaceImage(kVqaDrawTopLeft);
  EXPECT_EQ(drawer.x1, 10);
  EXPECT_EQ(drawer.x2, 17);
  EXPECT_EQ(drawer.y1, 20);
  EXPECT_EQ(drawer.y2, 27);
  EXPECT_EQ(drawer.image_offset, (320 * 20) + 10);
}

TEST(VqaDrawerTest, TopRightOrigin) {
  // Columns 302..309 leave a 10-column gap (310..319) on the right.
  const VqaDrawer drawer = PlaceImage(kVqaDrawTopRight);
  EXPECT_EQ(drawer.x1, 309);
  EXPECT_EQ(drawer.x2, 302);
  EXPECT_EQ(drawer.y1, 20);
  EXPECT_EQ(drawer.y2, 27);
  EXPECT_EQ(drawer.image_offset, (320 * 20) + 302);
}

TEST(VqaDrawerTest, BottomLeftOrigin) {
  // Rows 172..179 leave a 20-row gap (180..199) at the bottom.
  const VqaDrawer drawer = PlaceImage(kVqaDrawBottomLeft);
  EXPECT_EQ(drawer.x1, 10);
  EXPECT_EQ(drawer.x2, 17);
  EXPECT_EQ(drawer.y1, 179);
  EXPECT_EQ(drawer.y2, 172);
  EXPECT_EQ(drawer.image_offset, (320 * 172) + 10);
}

TEST(VqaDrawerTest, BottomRightOrigin) {
  const VqaDrawer drawer = PlaceImage(kVqaDrawBottomRight);
  EXPECT_EQ(drawer.x1, 309);
  EXPECT_EQ(drawer.x2, 302);
  EXPECT_EQ(drawer.y1, 179);
  EXPECT_EQ(drawer.y2, 172);
  EXPECT_EQ(drawer.image_offset, (320 * 172) + 302);
}

#ifndef NDEBUG
TEST(VqaDrawerDeathTest, ImageOutsideBufferFailsCheck) {
  // A gap wider than the buffer would start drawing outside it.
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(PlaceImage(kVqaDrawTopLeft, 400, 20), "Check failed");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(PlaceImage(kVqaDrawBottomRight, 10, 250), "Check failed");
}
#endif

}  // namespace
