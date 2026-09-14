// Tests for the VQA player: configuration defaults, handle lifecycle, the
// VQA_Open() validation/error paths, chunk loading into the play buffers and
// drawer placement. Movies are small synthetic files served by a scripted
// in-memory VqaIo file source. No real movie assets are required.

#include "winvq/vqa32/vqaplay.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/seek_origin.h"
#include "base/types.h"
#include "gtest/gtest.h"
#include "winvq/vqa32/vqafile.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplayp.h"
#include "winvq/vqm32/compress.h"
#include "winvq/vqm32/palette.h"

// Link-time stubs for symbols normally provided by the game or sdllib. The
// tests never draw frames or decode palettes, so these are never called.
extern void* MainWindow;  // Declared by the Windows viewer as an HWND.
void* MainWindow = nullptr;

int32_t LCW_Uncompress(const void* /*source*/, void* /*dest*/,
                       int32_t /*length*/) {
  return 0;
}

void SetPalette(unsigned char* /*palette*/, int32_t /*numbytes*/,
                uint32_t /*slowpal*/) {}

void Flag_To_Set_Palette(unsigned char* /*palette*/, int32_t /*numbytes*/,
                         uint32_t /*slowpal*/) {}

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
    memcpy(buffer.data(), data.data() + pos, buffer.size());
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

void AppendBytes(std::vector<uint8_t>& out, const char* text) {
  out.insert(out.end(), text, text + strlen(text));
}

void AppendBigEndian32(std::vector<uint8_t>& out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value >> 24));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}

// "FORM" <size> "WVQA" — the file preamble VQA_Open() validates first.
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
    VQA_DefaultConfig(&config_);
    config_.OptionFlags = 0;
    config_.DrawFlags = VQACFGF_NODRAW;
  }

  FakeVqaIo fake_;
  VqaPlayer player_;
  VQAConfig config_{};
};

TEST(VqaConfigTest, DefaultConfigHasDocumentedDefaults) {
  VQAConfig config;
  VQA_DefaultConfig(&config);

  EXPECT_EQ(config.ImageWidth, 320);
  EXPECT_EQ(config.ImageHeight, 200);
  EXPECT_EQ(config.X1, -1);
  EXPECT_EQ(config.Y1, -1);
  EXPECT_EQ(config.FrameRate, -1);  // -1 means use the movie's frame rate.
  EXPECT_EQ(config.DrawRate, -1);
  EXPECT_EQ(config.DrawFlags, 0);
  EXPECT_EQ(config.OptionFlags, VQAOPTF_AUDIO);
  EXPECT_EQ(config.NumFrameBufs, 6);
  EXPECT_EQ(config.NumCBBufs, 3);
  EXPECT_EQ(config.Volume, 0x00FF);
}

TEST_F(VqaPlayTest, OpenReportsOpenErrorWhenHandlerCannotOpen) {
  fake_.fail_open = true;

  EXPECT_EQ(player_.Open("missing.vqa", &config_), VQAERR_OPEN);
  EXPECT_EQ(fake_.opens, 1);
  // The file never opened, so the player must not try to close it.
  EXPECT_EQ(fake_.closes, 0);
}

TEST_F(VqaPlayTest, OpenReportsReadErrorAndClosesOnEmptyFile) {
  // No data at all: the first 8-byte header read fails.
  EXPECT_EQ(player_.Open("empty.vqa", &config_), VQAERR_READ);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsNonIffFile) {
  AppendBytes(fake_.data, "XXXX");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(player_.Open("notiff.vqa", &config_), VQAERR_NOTVQA);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsFormWithZeroSize) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0);
  AppendBytes(fake_.data, "WVQA");

  EXPECT_EQ(player_.Open("zerosize.vqa", &config_), VQAERR_NOTVQA);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsFormWithoutWvqaId) {
  AppendBytes(fake_.data, "FORM");
  AppendBigEndian32(fake_.data, 0x1234);
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(player_.Open("notvqa.vqa", &config_), VQAERR_NOTVQA);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenReportsReadErrorWhenTruncatedAfterPreamble) {
  fake_.data = ValidPreamble();

  EXPECT_EQ(player_.Open("truncated.vqa", &config_), VQAERR_READ);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, OpenRejectsHeaderChunkWithWrongSize) {
  fake_.data = ValidPreamble();
  AppendBytes(fake_.data, "VQHD");
  AppendBigEndian32(fake_.data, 4);  // Real VQA headers are much larger.
  AppendBytes(fake_.data, "XXXX");

  EXPECT_EQ(player_.Open("badheader.vqa", &config_), VQAERR_NOTVQA);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaPlayTest, IoHandlerSurvivesFailedOpen) {
  // A failed open runs VQA_Close(), which resets the handle. The installed
  // io object must survive the reset so the handle can be reused.
  ASSERT_EQ(player_.Open("empty.vqa", &config_), VQAERR_READ);

  fake_.data = ValidPreamble();
  fake_.pos = 0;
  EXPECT_EQ(player_.Open("second.vqa", &config_), VQAERR_READ);
  EXPECT_EQ(fake_.opens, 2);
  EXPECT_EQ(fake_.closes, 2);
}

// Appends an IFF chunk: id, big-endian declared size, payload and the pad
// byte for odd payloads. declared_size may disagree with the payload to
// model malformed files.
void AppendChunk(std::vector<uint8_t>& out, const char* id,
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

void AppendChunk(std::vector<uint8_t>& out, const char* id,
                 const std::vector<uint8_t>& payload) {
  AppendChunk(out, id, static_cast<uint32_t>(payload.size()), payload);
}

// A 3-frame 8x8 movie with 4x2 blocks and a 16-entry codebook. The loader
// derives these buffer sizes from it:
//   Max_CB_Size  = (16 * 4 * 2 + 250) & 0xFFFC = 376
//   Max_Ptr_Size = (2 * 4 * 2 + 1024) & 0xFFFC = 1040
//   Max_Pal_Size = (768 + 1024) & 0xFFFC       = 1792
VQAHeader SmallHeader() {
  VQAHeader header{};
  header.Version = VQAHD_VER2;
  header.Frames = 3;
  header.ImageWidth = 8;
  header.ImageHeight = 8;
  header.BlockWidth = 4;
  header.BlockHeight = 2;
  header.FPS = 15;
  header.Groupsize = 1;
  header.CBentries = 16;
  return header;
}

constexpr int kMaxCbSize = 376;

std::vector<uint8_t> HeaderPayload(const VQAHeader& header) {
  std::vector<uint8_t> payload(sizeof(header));
  memcpy(payload.data(), &header, sizeof(header));
  return payload;
}

// FINF entries are 4 bytes each, stored in native (little-endian) order.
std::vector<uint8_t> FinfPayload(const std::vector<uint32_t>& entries) {
  std::vector<uint8_t> payload(entries.size() * sizeof(uint32_t));
  memcpy(payload.data(), entries.data(), payload.size());
  return payload;
}

// Preamble, VQHD and FINF: everything VQA_Open() reads before the frames.
std::vector<uint8_t> MovieStart(const VQAHeader& header,
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
// One frame buffer means VQA_Open() primes exactly one frame.
class VqaLoaderTest : public testing::Test {
 protected:
  void SetUp() override {
    handle_.io = &fake_;
    VQA_DefaultConfig(&config_);
    config_.OptionFlags = 0;
    config_.DrawFlags = VQACFGF_NODRAW;
    config_.NumFrameBufs = 1;
    config_.NumCBBufs = 1;
  }

  void TearDown() override {
    if (handle_.data != nullptr) {
      VQA_Close(&handle_);
    }
  }

  int32_t Open() { return VQA_Open(&handle_, "test.vqa", &config_); }

  FakeVqaIo fake_;
  VQAHandle handle_;
  VQAConfig config_{};
};

TEST_F(VqaLoaderTest, OpensMinimalMovie) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), 0);
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(VqaLoaderTest, FinfEntriesAreFourBytesEach) {
  const std::vector<uint32_t> entries = {0x40000010, 0x00000020, 0x80000030};
  fake_.data = MovieStart(SmallHeader(), entries);
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  ASSERT_EQ(handle_.data->FoffStorage.size(), entries.size());
  EXPECT_EQ(handle_.data->Foff[0], entries[0]);
  EXPECT_EQ(handle_.data->Foff[1], entries[1]);
  EXPECT_EQ(handle_.data->Foff[2], entries[2]);
  // The flags occupy the top bits; the offset is stored halved.
  EXPECT_NE(handle_.data->Foff[0] & VQAFINF_PAL, 0);
  EXPECT_EQ(VQAFRAME_OFFSET(handle_.data->Foff[1]), 0x40);
}

TEST_F(VqaLoaderTest, OversizedFinfChunkIsSkippedPastTheTable) {
  // Eight entries for a three-frame movie.
  fake_.data =
      MovieStart(SmallHeader(), {0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  ASSERT_EQ(handle_.data->FoffStorage.size(), 3U);
  EXPECT_EQ(handle_.data->Foff[2], 0x30U);
  // The excess entries were skipped, so the frame after them still loaded.
  EXPECT_EQ(fake_.pos, static_cast<int64_t>(fake_.data.size()));
}

TEST_F(VqaLoaderTest, RejectsFinfBeforeHeader) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "FINF", FinfPayload({0, 0, 0}));

  EXPECT_EQ(Open(), VQAERR_NOTVQA);
}

TEST_F(VqaLoaderTest, RejectsHeaderWithZeroGroupsize) {
  VQAHeader header = SmallHeader();
  header.Groupsize = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_NOTVQA);
}

TEST_F(VqaLoaderTest, RejectsHeaderWithZeroBlockSize) {
  VQAHeader header = SmallHeader();
  header.BlockWidth = 0;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_NOTVQA);
}

TEST_F(VqaLoaderTest, RejectsPreFrameChunkOf2GiB) {
  fake_.data = ValidPreamble();
  AppendChunk(fake_.data, "VQHD", HeaderPayload(SmallHeader()));
  // 2^31 reads back as a negative int32_t size.
  AppendChunk(fake_.data, "XXXX", 0x80000000U, {});

  EXPECT_EQ(Open(), VQAERR_NOTVQA);
}

TEST_F(VqaLoaderTest, RejectsFrameChunkOf2GiB) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "VQFR", 0x80000000U, {});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
}

TEST_F(VqaLoaderTest, RejectsChunkOf2GiBInsideFrame) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  std::vector<uint8_t> frame;
  AppendChunk(frame, "CBF0", 0xFFFFFFF8U, {});
  AppendChunk(fake_.data, "VQFR", frame);
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
}

TEST_F(VqaLoaderTest, PartialCompressedCodebookLoadsAtEstimatedOffset) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20, 0xAB));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  // Groupsize 1: offset = Max_CB_Size - (20 * 1 + 100).
  const VQACBNode* codebook = handle_.data->Loader.FullCB;
  EXPECT_EQ(codebook->CBOffset, kMaxCbSize - 120);
  EXPECT_EQ(codebook->Buffer[codebook->CBOffset], 0xAB);
}

TEST_F(VqaLoaderTest, RejectsPartialCodebookWithNegativeOffset) {
  // 300 bytes fit the 376-byte codebook, but the estimated start
  // 376 - (300 * 1 + 100) = -24 lies before the buffer.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(300));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
}

TEST_F(VqaLoaderTest, RejectsPartialCodebooksOverflowingTheEnd) {
  // Groupsize 2: the first 20-byte part sets the offset to
  // 376 - (20 * 2 + 100) = 236; a 200-byte second part would end at 456.
  VQAHeader header = SmallHeader();
  header.Groupsize = 2;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(20));
  AppendChunk(fake_.data, "CBPZ", std::vector<uint8_t>(200));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
}

TEST_F(VqaLoaderTest, AcceptsFullPalette) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(768, 7));
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  EXPECT_EQ(handle_.data->Drawer.CurPalSize, 768);
  EXPECT_EQ(handle_.data->Drawer.Palette_24[767], 7);
}

TEST_F(VqaLoaderTest, RejectsUncompressedPaletteOver256Colors) {
  // 800 bytes fit the frame's 1792-byte palette buffer, but not the 768-byte
  // drawer palette the first palette is copied into.
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendChunk(fake_.data, "CPL0", std::vector<uint8_t>(800));
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
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

  EXPECT_EQ(Open(), VQAERR_READ);
}

INSTANTIATE_TEST_SUITE_P(AllBufferedChunks, VqaOversizedChunkTest,
                         testing::Values("CBF0", "CBFZ", "CBP0", "CBPZ",
                                         "CPL0", "CPLZ", "VPT0", "VPTZ"));

TEST_F(VqaLoaderTest, OpensMovieShorterThanFrameBuffers) {
  config_.NumFrameBufs = 3;
  VQAHeader header = SmallHeader();
  header.Frames = 1;
  fake_.data = MovieStart(header, {0});
  AppendFrameEnd(fake_.data);

  ASSERT_EQ(Open(), 0);
  EXPECT_EQ(handle_.data->LoadedFrames, 1);
}

TEST_F(VqaLoaderTest, TruncatedMovieStillFailsToOpen) {
  // The header promises three frames but the file holds one.
  config_.NumFrameBufs = 3;
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);

  EXPECT_EQ(Open(), VQAERR_READ);
}

TEST_F(VqaLoaderTest, RejectsAudioWithZeroHmiBufferSize) {
  VQAHeader header = SmallHeader();
  header.Flags = VQAHDF_AUDIO;
  fake_.data = MovieStart(header, {0, 0, 0});
  AppendFrameEnd(fake_.data);
  config_.OptionFlags = VQAOPTF_AUDIO;
  config_.HMIBufSize = 0;

  EXPECT_EQ(Open(), VQAERR_AUDIO);
  EXPECT_EQ(fake_.closes, 1);
}

TEST_F(VqaLoaderTest, SeekFrameLoadsFromTheFrameTable) {
  constexpr int64_t kFrameBytes = 10;  // "VPT0", size and 2 payload bytes.
  const auto start =
      static_cast<int64_t>(MovieStart(SmallHeader(), {0, 0, 0}).size());
  std::vector<uint32_t> entries(3);
  for (int i = 0; i < 3; ++i) {
    // Entries store half the file offset.
    entries[static_cast<size_t>(i)] =
        static_cast<uint32_t>((start + (i * kFrameBytes)) / 2);
  }
  fake_.data = MovieStart(SmallHeader(), entries);
  for (int i = 0; i < 3; ++i) {
    const auto tag = static_cast<uint8_t>(i);
    AppendChunk(fake_.data, "VPT0", {tag, tag});
  }
  config_.OptionFlags = VQAOPTF_PALOFF;
  ASSERT_EQ(Open(), 0);

  // Groupsize 1: frame 1 is replayed for its codebook, then frame 2 primed.
  EXPECT_EQ(VQA_SeekFrame(&handle_, 2, SEEK_SET), 2);
  EXPECT_EQ(handle_.data->Loader.CurFrameNum, 3);
  EXPECT_EQ(handle_.data->Loader.CurFrame->Pointers[0], 2);
}

TEST_F(VqaLoaderTest, SeekFrameRejectsFramesOutsideTheMovie) {
  fake_.data = MovieStart(SmallHeader(), {0, 0, 0});
  AppendFrameEnd(fake_.data);
  ASSERT_EQ(Open(), 0);

  EXPECT_EQ(VQA_SeekFrame(&handle_, 3, SEEK_SET), VQAERR_EOF);
  EXPECT_EQ(VQA_SeekFrame(&handle_, -1, SEEK_SET), VQAERR_SEEK);
}

// Places the 8x8 SmallHeader() image in a 320x200 buffer, gap_x pixels
// horizontally and gap_y vertically from the corner named by origin.
VQADrawer PlaceImage(uint32_t origin, int gap_x = 10, int gap_y = 20) {
  VQAData data;
  data.Drawer.ImageWidth = 320;
  data.Drawer.ImageHeight = 200;
  data.Drawer.Y2 = 12345;  // Stale value the placement must not read.

  VQAHandle handle;
  handle.data = &data;
  handle.header = SmallHeader();
  handle.config.X1 = gap_x;
  handle.config.Y1 = gap_y;
  handle.config.DrawFlags = origin;

  VQA_Configure_Drawer(&handle);

  handle.data = nullptr;
  return data.Drawer;
}

TEST(VqaDrawerTest, TopLeftOrigin) {
  const VQADrawer drawer = PlaceImage(VQACFGF_TOPLEFT);
  EXPECT_EQ(drawer.X1, 10);
  EXPECT_EQ(drawer.X2, 17);
  EXPECT_EQ(drawer.Y1, 20);
  EXPECT_EQ(drawer.Y2, 27);
  EXPECT_EQ(drawer.ScreenOffset, (320 * 20) + 10);
}

TEST(VqaDrawerTest, TopRightOrigin) {
  // Columns 302..309 leave a 10-column gap (310..319) on the right.
  const VQADrawer drawer = PlaceImage(VQACFGF_TOPRIGHT);
  EXPECT_EQ(drawer.X1, 309);
  EXPECT_EQ(drawer.X2, 302);
  EXPECT_EQ(drawer.Y1, 20);
  EXPECT_EQ(drawer.Y2, 27);
  EXPECT_EQ(drawer.ScreenOffset, (320 * 20) + 302);
}

TEST(VqaDrawerTest, BottomLeftOrigin) {
  // Rows 172..179 leave a 20-row gap (180..199) at the bottom.
  const VQADrawer drawer = PlaceImage(VQACFGF_BOTLEFT);
  EXPECT_EQ(drawer.X1, 10);
  EXPECT_EQ(drawer.X2, 17);
  EXPECT_EQ(drawer.Y1, 179);
  EXPECT_EQ(drawer.Y2, 172);
  EXPECT_EQ(drawer.ScreenOffset, (320 * 172) + 10);
}

TEST(VqaDrawerTest, BottomRightOrigin) {
  const VQADrawer drawer = PlaceImage(VQACFGF_BOTRIGHT);
  EXPECT_EQ(drawer.X1, 309);
  EXPECT_EQ(drawer.X2, 302);
  EXPECT_EQ(drawer.Y1, 179);
  EXPECT_EQ(drawer.Y2, 172);
  EXPECT_EQ(drawer.ScreenOffset, (320 * 172) + 302);
}

#ifndef NDEBUG
TEST(VqaDrawerDeathTest, ImageOutsideBufferFailsCheck) {
  // A gap wider than the buffer would start drawing outside it.
  // The switch is inside GoogleTest's macro.
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(PlaceImage(VQACFGF_TOPLEFT, 400, 20), "Check failed");
  // NOLINTNEXTLINE(clang-diagnostic-switch-default)
  EXPECT_DEATH(PlaceImage(VQACFGF_BOTRIGHT, 10, 250), "Check failed");
}
#endif

}  // namespace
