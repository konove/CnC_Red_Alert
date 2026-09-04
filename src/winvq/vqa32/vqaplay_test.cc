// Tests for the public VQA player API: configuration defaults, handle
// lifecycle, and the VQA_Open() validation/error paths, driven by a scripted
// in-memory VqaIo file source. No real movie assets are required.

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "gtest/gtest.h"
#include "winvq/vqa32/vqaio.h"
#include "winvq/vqa32/vqaplay.h"

// Link-time stubs for symbols normally provided by the game or sdllib. The
// tests never draw frames or decode palettes, so these are never called.
void* MainWindow = nullptr;

extern "C" unsigned long LCW_Uncompress(char const* /*source*/, char* /*dest*/,
                                        unsigned long /*length*/) {
  return 0;
}

extern "C" void SetPalette(unsigned char* /*palette*/, long /*numbytes*/,
                           unsigned long /*slowpal*/) {}

void Flag_To_Set_Palette(unsigned char* /*palette*/, long /*numbytes*/,
                         unsigned long /*slowpal*/) {}

namespace {

// Scripted in-memory file source. Records how the player drives it so
// tests can assert on the interaction.
class FakeVqaIo final : public VqaIo {
 public:
  int Open(const char* /*filename*/) override {
    opens++;
    if (fail_open) {
      return 1;
    }
    pos = 0;
    return 0;
  }

  int Read(void* buffer, int64_t bytes) override {
    if (fail_read || pos + bytes > static_cast<int64_t>(data.size())) {
      return 1;
    }
    memcpy(buffer, data.data() + pos, static_cast<size_t>(bytes));
    pos += bytes;
    return 0;
  }

  int Seek(int64_t offset, int origin) override {
    if (origin == SEEK_SET) {
      pos = offset;
    } else {
      pos += offset;
    }
    return 0;
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

}  // namespace
