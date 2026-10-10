// Holds the Rust VQ frame decoder in src/rust to the C++ one it replaced: the
// same image bytes, including the ones left alone, for frames that fit, sizes
// that do not, and entries past the end of the codebook.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/lcg_test_util.h"
#include "base/numeric.h"
#include "engine/video/vqa/vq_decoder.h"
#include "engine/video/vqa/vq_decoder_reference_test_util.h"
#include "gtest/gtest.h"

namespace {

std::vector<uint8_t> RandomBuffer(base::Lcg& lcg, const int size) {
  std::vector<uint8_t> buffer(base::ToSize(size));
  std::ranges::generate(buffer, [&lcg] { return lcg.Byte(); });
  return buffer;
}

// Decodes one random frame with both decoders. `slack` is how far the sizes
// of the codebook, pointers and image may fall short of or exceed what the
// frame needs: 0 for a frame that fits exactly. Returns whether the frame was
// drawn.
bool ExpectMatch(base::Lcg& lcg, const int slack) {
  const BlockShape shape =
      lcg.Below(2) == 0 ? BlockShape::k4x2 : BlockShape::k4x4;
  const int height = shape == BlockShape::k4x2 ? 2 : 4;
  const auto vary = [&](const int size) {
    return slack == 0 ? size
                      : std::max(0, size + lcg.Below((2 * slack) + 1) - slack);
  };
  const int columns = 1 + lcg.Below(12);
  const int rows = 1 + lcg.Below(8);
  const int stride = vary((columns * 4) + lcg.Below(6));
  const int codebook_blocks = 1 + lcg.Below(40);
  const auto codebook = RandomBuffer(lcg, vary(codebook_blocks * 4 * height));
  // Mostly entries inside the codebook or solid; with slack, any high byte.
  auto pointers = RandomBuffer(lcg, vary(columns * rows * 2));
  const auto blocks = base::ToSize(columns * rows);
  for (std::size_t block = 0; block < pointers.size(); ++block) {
    if (block >= blocks && (slack == 0 || lcg.Below(20) != 0)) {
      const bool solid = lcg.Below(5) == 0;
      const int marker = shape == BlockShape::k4x2 ? 0x0f : 0xff;
      pointers.at(block) =
          static_cast<uint8_t>(solid ? marker : codebook_blocks / 256);
    } else if (block < blocks && codebook_blocks < 256) {
      pointers.at(block) = static_cast<uint8_t>(
          lcg.Below(slack == 0 ? codebook_blocks : codebook_blocks + 2));
    }
  }
  const auto image = RandomBuffer(
      lcg, vary((std::max(stride, 1) * ((rows * height) - 1)) + (columns * 4)));

  auto want = image;
  auto got = image;
  const int given_columns = slack == 0 ? columns : columns + lcg.Below(3) - 1;
  const int given_rows = slack == 0 ? rows : rows + lcg.Below(3) - 1;
  vq_decoder_reference::DecodeVqFrame(shape, codebook, pointers, want,
                                      given_columns, given_rows, stride);
  DecodeVqFrame(shape, codebook, pointers, got, given_columns, given_rows,
                stride);
  EXPECT_TRUE(got == want)
      << given_columns << "x" << given_rows << " blocks of 4x" << height
      << ", stride " << stride << ", image " << image.size() << ", pointers "
      << pointers.size() << ", codebook " << codebook.size();
  return want != image;
}

TEST(VqDecoderDifferentialTest, FramesThatFitMatchReference) {
  base::Lcg lcg(1);
  int drawn = 0;
  for (int round = 0; round < 4000; ++round) {
    drawn += ExpectMatch(lcg, 0) ? 1 : 0;
  }
  // Nearly all of them: a frame of blocks that happen to match the image
  // already there is not told apart from one that was refused.
  EXPECT_GT(drawn, 3900);
}

TEST(VqDecoderDifferentialTest, SizesOffByALittleMatchReference) {
  base::Lcg lcg(2);
  for (int round = 0; round < 8000; ++round) {
    ExpectMatch(lcg, 1 + lcg.Below(3));
  }
}

TEST(VqDecoderDifferentialTest, SizesFarOffMatchReference) {
  base::Lcg lcg(3);
  for (int round = 0; round < 8000; ++round) {
    ExpectMatch(lcg, 40);
  }
}

TEST(VqDecoderDeathTest, RejectsAnImageThatHoldsItsPointers) {
  std::vector<uint8_t> buffer(64);
  const std::vector<uint8_t> codebook(16);
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(
      DecodeVqFrame(BlockShape::k4x2, codebook, std::span(buffer).first(8),
                    buffer, 2, 2, 8),
      "its own codebook or pointers");
}

}  // namespace
