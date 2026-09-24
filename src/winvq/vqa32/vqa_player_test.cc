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
#include <utility>
#include <vector>

#include "base/array.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player_state.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqm32/compress.h"

// Link-time stubs for symbols normally provided by the game or sdllib. The
// tests never decode compressed data, so LCW_Uncompress() is never called;
// QueueVqaPalette() records what it was handed.

// A copy of the palette last passed to QueueVqaPalette(), for the drawer
// tests. The stub is a free function, so it cannot reach a fixture member.
static std::vector<uint8_t>& QueuedPalette() {
  static std::vector<uint8_t> palette;
  return palette;
}

int32_t LCW_Uncompress(std::span<const unsigned char> /*source*/,
                       std::span<unsigned char> /*dest*/) {
  return 0;
}

void QueueVqaPalette(std::span<uint8_t> palette, int32_t numbytes,
                     uint32_t /*slowpal*/) {
  QueuedPalette().assign(
      palette.begin(),
      palette.begin() + std::min<base::ssize>(numbytes, std::ssize(palette)));
}

namespace {

// Scripted in-memory file source. Records how the player drives it so
// tests can assert on the interaction.
class FakeVqaIo final : public VqaIo {
 public:
  bool Open(std::string_view /*name*/) override {
    opens++;
    if (fail_open) {
      return false;
    }
    pos = 0;
    return true;
  }

  bool Read(std::span<std::byte> buffer) override {
    const int64_t bytes = std::ssize(buffer);
    if (fail_read || pos + bytes > static_cast<int64_t>(data.size())) {
      return false;
    }
    base::CopyBytes(buffer,
                    std::as_bytes(std::span(data).subspan(base::ToSize(pos))),
                    buffer.size());
    pos += bytes;
    return true;
  }

  // Like a real file, refuses to move outside the data.
  bool Seek(base::ssize offset, SeekOrigin origin) override {
    int64_t target = offset;
    switch (origin) {
      case SeekOrigin::kCurrent:
        target += pos;
        break;
      case SeekOrigin::kEnd:
        target += static_cast<int64_t>(data.size());
        break;
      case SeekOrigin::kBegin:
      default:
        break;
    }
    if (target < 0 || std::cmp_greater(target, data.size())) {
      return false;
    }
    pos = target;
    return true;
  }

  void Close() override { closes++; }

  std::vector<uint8_t> data;
  int64_t pos = 0;
  bool fail_open = false;
  bool fail_read = false;
  int opens = 0;
  int closes = 0;
};

void AppendBytes(std::vector<uint8_t>& out, std::string_view text) {
  out.insert(out.end(), text.begin(), text.end());
}

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
void AppendBigEndian32(std::vector<uint8_t>& out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value >> 24));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}

// "FORM" <size> "WVQA" — the file preamble OpenVqa() validates first.
std::vector<uint8_t> ValidPreamble() {
  std::vector<uint8_t> data;
  AppendBytes(data, "FORM");
  AppendBigEndian32(data, 0x1234);
  AppendBytes(data, "WVQA");
  return data;
}

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

// Appends an IFF chunk: id, big-endian declared size, payload and the pad
// byte for odd payloads. declared_size may disagree with the payload to
// model malformed files.
void AppendChunk(std::vector<uint8_t>& out, std::string_view id,
                 uint32_t declared_size, const std::vector<uint8_t>& payload) {
  // Built locally and appended once, so out is modified in a single step.
  std::vector<uint8_t> chunk;
  AppendBytes(chunk, id);
  AppendBigEndian32(chunk, declared_size);
  chunk.insert(chunk.end(), payload.begin(), payload.end());
  if (payload.size() % 2 != 0) {
    chunk.push_back(0);
  }
  out.insert(out.end(), chunk.begin(), chunk.end());
}

void AppendChunk(std::vector<uint8_t>& out, std::string_view id,
                 const std::vector<uint8_t>& payload) {
  AppendChunk(out, id, static_cast<uint32_t>(payload.size()), payload);
}

// A 3-frame 8x8 movie with 4x2 blocks and a 16-entry codebook. The loader
// derives these buffer sizes from it:
//   codebook_capacity  = (16 * 4 * 2 + 250) & 0xFFFC = 376
//   pointers_capacity = (2 * 4 * 2 + 1024) & 0xFFFC = 1040
//   palette_capacity = (768 + 1024) & 0xFFFC       = 1792
VqaHeader SmallHeader() {
  VqaHeader header{};
  header.version = kVqaVersion2;
  header.frame_count = 3;
  header.image_width = 8;
  header.image_height = 8;
  header.block_width = 4;
  header.block_height = 2;
  header.fps = 15;
  header.frames_per_group = 1;
  header.codebook_entries = 16;
  return header;
}

constexpr int kCodebookCapacity = 376;

// SmallHeader() with a sound track of 22050 Hz, 8-bit mono.
VqaHeader SmallSoundHeader() {
  VqaHeader header = SmallHeader();
  header.flags = kVqaHasAudio;
  header.sample_rate = 22050;
  header.channels = 1;
  header.bits_per_sample = 8;
  return header;
}

std::vector<uint8_t> HeaderPayload(const VqaHeader& header) {
  std::vector<uint8_t> payload(sizeof(header));
  base::CopyBytes(std::as_writable_bytes(std::span(payload)),
                  base::ObjectBytes(header), sizeof(header));
  return payload;
}

// FINF entries are 4 bytes each, stored in native (little-endian) order.
std::vector<uint8_t> FinfPayload(const std::vector<uint32_t>& entries) {
  std::vector<uint8_t> payload(entries.size() * sizeof(uint32_t));
  base::CopyBytes(std::as_writable_bytes(std::span(payload)),
                  std::as_bytes(std::span(entries)), payload.size());
  return payload;
}

// Preamble, VQHD and FINF: everything OpenVqa() reads before the frames.
std::vector<uint8_t> MovieStart(const VqaHeader& header,
                                const std::vector<uint32_t>& entries) {
  std::vector<uint8_t> data = ValidPreamble();
  AppendChunk(data, "VQHD", HeaderPayload(header));
  AppendChunk(data, "FINF", FinfPayload(entries));
  return data;
}

// Appends the chunk that completes a frame: uncompressed vector pointers.
void AppendFrameEnd(std::vector<uint8_t>& data) {
  AppendChunk(data, "VPT0", std::vector<uint8_t>(2));
}

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

  // Turns the sound on. The player converts to audio_spec_ and installs its
  // mixer in audio_callback_, which must outlive the movie.
  void EnableAudio() {
    audio_spec_.freq = 22050;
    audio_spec_.format = AUDIO_S16;
    audio_spec_.channels = 2;
    config_.option_flags |= kVqaOptionAudio;
    config_.audio_spec = &audio_spec_;
    config_.audio_callback = &audio_callback_;
  }

  FakeVqaIo fake_;
  SDL_AudioSpec audio_spec_{};
  void (*audio_callback_)(std::span<std::byte>) = nullptr;
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

TEST_F(VqaLoaderTest, PartialCompressedCodebookLoadsAtEstimatedOffset) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20, 0xAB));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  // Groupsize 1: offset = codebook_capacity - (20 * 1 + 100).
  const VqaCodebook* codebook = state_.movie->loader.full_codebook;
  EXPECT_EQ(codebook->compressed_offset, kCodebookCapacity - 120);
  EXPECT_EQ(codebook->buffer.at(base::ToSize(codebook->compressed_offset)),
            0xAB);
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
  EXPECT_EQ(state_.movie->drawer.saved_palette_bytes, 768);
  EXPECT_EQ(state_.movie->drawer.saved_palette.at(767), 7);
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
  SetMovieClock(&state_, 9, kVqaClockSystem);
  ASSERT_EQ(DrawNextFrame(&state_), 0);

  EXPECT_EQ(state_.movie->drawer.last_drawn_frame, 2);
  ASSERT_EQ(QueuedPalette().size(), 768U);
  EXPECT_EQ(QueuedPalette().front(), 9);
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
  EXPECT_EQ(state_.movie->loader.next_frame_number, 1);
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

// A movie of frame_count frames, each only the chunk that completes it.
std::vector<uint8_t> EmptyFrames(const VqaHeader& header) {
  std::vector<uint8_t> data =
      MovieStart(header, std::vector<uint32_t>(header.frame_count));
  for (int i = 0; i < int{header.frame_count}; ++i) {
    AppendFrameEnd(data);
  }
  return data;
}

TEST_F(VqaLoaderTest, PlaysSilentlyWithoutAnAudioRing) {
  VqaHeader header = SmallSoundHeader();
  header.frame_count = 1;
  fake_.data = EmptyFrames(header);
  EnableAudio();
  config_.audio_buffer_bytes = 0;
  ASSERT_EQ(Open(), 0);
  // No sound path runs: the mixer was never installed.
  EXPECT_EQ(state_.config.option_flags & kVqaOptionAudio, 0U);
  EXPECT_EQ(audio_callback_, nullptr);

  // A pause and resume must not start the sound either.
  EXPECT_EQ(PlayVqa(&state_, kVqaModePause), kVqaPaused);
  EXPECT_EQ(PlayVqa(&state_, kVqaModeRun), kVqaEndOfMovie);
  EXPECT_EQ(state_.movie->audio.flags & kAudioPlaying, 0U);
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
  EXPECT_NE(state_.movie->audio.flags & kAudioPlaying, 0U);
  PlayVqa(&state_, kVqaModeWalk);
  EXPECT_NE(state_.movie->audio.flags & kAudioPlaying, 0U);

  // The walk that reaches the end stops it.
  int32_t result = 0;
  for (int i = 0; i < 10 && result != kVqaEndOfMovie; ++i) {
    result = PlayVqa(&state_, kVqaModeWalk);
  }
  EXPECT_EQ(result, kVqaEndOfMovie);
  EXPECT_EQ(state_.movie->audio.flags & kAudioPlaying, 0U);
}

TEST_F(VqaLoaderTest, FailsToOpenWhenTheSoundCannotBeConverted) {
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  // Not an SDL sample format, so SDL cannot convert to it.
  audio_spec_.format = 0;

  EXPECT_EQ(Open(), kVqaErrorAudio);
  EXPECT_EQ(audio_callback_, nullptr);
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
  VqaAudio& audio = state_.movie->audio;
  ASSERT_EQ(audio.block_count, 2);
  ASSERT_EQ(StartMovieAudio(&state_), 0);

  std::array<std::byte, 256> device{};
  for (int i = 0; i < 1000 && audio.play_block != 1; ++i) {
    audio_callback_(device);
  }
  ASSERT_EQ(audio.play_block, 1);

  // Once the loader has refilled block 0, playing moves on to it.
  audio.block_loaded.at(0) = 1;
  for (int i = 0; i < 1000 && audio.play_block == 1; ++i) {
    audio_callback_(device);
  }
  EXPECT_EQ(audio.play_block, 0);
}

TEST_F(VqaLoaderTest, CopyStagedAudioWrapsAtTheEndOfTheRing) {
  fake_.data = EmptyFrames(SmallSoundHeader());
  EnableAudio();
  config_.audio_buffer_bytes = 4 * 2048;
  ASSERT_EQ(Open(), 0);
  VqaAudio& audio = state_.movie->audio;
  ASSERT_EQ(audio.block_count, 4);

  // 1000 bytes into block 0: nothing completed yet.
  std::ranges::fill(audio.staging, uint8_t{1});
  audio.staged_bytes = 1000;
  EXPECT_EQ(CopyStagedAudio(&state_), 0);
  EXPECT_EQ(audio.write_offset, 1000);
  EXPECT_EQ(audio.staged_bytes, 0);
  EXPECT_EQ(audio.block_loaded, (std::vector<int16_t>{0, 0, 0, 0}));

  // From the middle of block 3 round to the middle of block 0, completing
  // block 3 only.
  audio.write_offset = (3 * 2048) + 1024;
  std::ranges::fill(audio.staging, uint8_t{2});
  audio.staged_bytes = 1024 + 500;
  EXPECT_EQ(CopyStagedAudio(&state_), 0);
  EXPECT_EQ(audio.write_offset, 500);
  EXPECT_EQ(audio.block_loaded, (std::vector<int16_t>{0, 0, 0, 1}));
  EXPECT_EQ(base::At(audio.ring, (4 * 2048) - 1), 2);
  EXPECT_EQ(base::At(audio.ring, 499), 2);
  EXPECT_EQ(base::At(audio.ring, 500), 1);

  // Block 3 has not played, so a write that would reach it again waits.
  audio.write_offset = 2 * 2048;
  audio.staged_bytes = 2048;
  EXPECT_EQ(CopyStagedAudio(&state_), kVqaSleeping);
  EXPECT_EQ(audio.staged_bytes, 2048);
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
  VqaAudio& audio = state_.movie->audio;
  ASSERT_EQ(audio.write_offset, 0);

  // Once both blocks have played, the next chunk goes in at the start.
  std::ranges::fill(audio.block_loaded, int16_t{0});
  audio.staged_bytes = 100;
  EXPECT_EQ(CopyStagedAudio(&state_), 0);
  EXPECT_EQ(audio.write_offset, 100);
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
  EXPECT_EQ(state_.movie->loader.next_frame_number, 1);
}

// Places the 8x8 SmallHeader() image in a 320x200 buffer, gap_x pixels
// horizontally and gap_y vertically from the corner named by origin.
VqaDrawer PlaceImage(uint32_t origin, int gap_x = 10, int gap_y = 20) {
  VqaPlayerState state;
  state.movie = std::make_unique<VqaMovie>();
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
