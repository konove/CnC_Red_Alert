// Times the engine kernels that moved to Rust (src/rust) against the C++
// references they replaced. Run a RelWithDebInfo build; see CLAUDE.md on Debug.
//
// Usage: kernel_bench

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "absl/strings/str_format.h"
#include "base/lcg_test_util.h"
#include "engine/codec/aud_decoder.h"
#include "engine/codec/aud_decoder_reference_test_util.h"
#include "engine/codec/xor_delta.h"
#include "engine/codec/xor_delta_reference_test_util.h"
#include "engine/crypto/blowfish.h"
#include "engine/crypto/blowfish_reference_test_util.h"
#include "engine/crypto/sha1_compress.h"
#include "engine/crypto/sha1_compress_reference_test_util.h"
#include "engine/video/vqa/adpcm_decoders.h"
#include "engine/video/vqa/adpcm_decoders_reference_test_util.h"
#include "engine/video/vqa/vq_decoder.h"
#include "engine/video/vqa/vq_decoder_reference_test_util.h"

namespace {

// Returns how many megabytes a second `function` gets through if one call
// handles `bytes`: the best of five timings of enough calls to fill 50 ms.
template <typename Function>
double MegabytesPerSecond(const std::size_t bytes, const Function& function) {
  using Clock = std::chrono::steady_clock;
  double best = 0;
  for (int attempt = 0; attempt < 5; ++attempt) {
    int calls = 0;
    const auto start = Clock::now();
    std::chrono::duration<double> elapsed{};
    do {
      function();
      ++calls;
      elapsed = Clock::now() - start;
    } while (elapsed.count() < 0.05);
    const double megabytes =
        static_cast<double>(bytes) * calls / (1024.0 * 1024.0);
    best = std::max(best, megabytes / elapsed.count());
  }
  return best;
}

void Report(const char* const what, const double rust, const double cpp) {
  absl::PrintF("%-26s Rust %8.1f MB/s   C++ %8.1f MB/s   %+.0f%%\n", what,
               rust, cpp, ((rust / cpp) - 1.0) * 100.0);
}

// A delta over `pixels` pixels shaped like an animation frame's: short and
// long runs, literals and skips in turn.
std::vector<std::byte> FrameDelta(const int pixels) {
  base::Lcg lcg(1);
  std::vector<std::byte> delta;
  const auto append = [&delta](const uint32_t value) {
    delta.push_back(static_cast<std::byte>(value));
  };
  for (int left = pixels; left > 0;) {
    const int kind = lcg.Below(5);
    const int pixels_covered =
        std::min(left, 1 + lcg.Below(kind == 4 ? 400 : 127));
    left -= pixels_covered;
    const auto count = static_cast<uint32_t>(pixels_covered);
    switch (kind) {
      case 0:
        append(0);
        append(count);
        append(lcg.Byte());
        break;
      case 1:
      case 2:
        append(count);
        for (uint32_t i = 0; i < count; ++i) {
          append(lcg.Byte());
        }
        break;
      case 3:
        append(0x80U | count);
        break;
      default:
        append(0x80);
        append(count & 0xffU);
        append(0x80U | (count >> 8U));
        for (uint32_t i = 0; i < count; ++i) {
          append(lcg.Byte());
        }
        break;
    }
  }
  delta.insert(delta.end(), {std::byte{0x80}, std::byte{0}, std::byte{0}});
  return delta;
}

void BenchXorDelta() {
  constexpr int kWidth = 320;
  constexpr int kHeight = 200;
  constexpr int kStride = 640;
  const auto delta = FrameDelta(kWidth * kHeight);
  std::vector<uint8_t> flat(std::size_t{kWidth} * kHeight);
  std::vector<uint8_t> view(std::size_t{kStride} * kHeight);
  Report(
      "xor delta, contiguous",
      MegabytesPerSecond(flat.size(), [&] { ApplyXorDelta(flat, delta); }),
      MegabytesPerSecond(flat.size(), [&] {
        xor_delta_reference::ApplyXorDelta(flat, delta);
      }));
  Report("xor delta, rows", MegabytesPerSecond(flat.size(), [&] {
           ApplyXorDeltaToView(view, delta, kWidth, kStride, /*copy=*/false);
         }),
         MegabytesPerSecond(flat.size(), [&] {
           xor_delta_reference::ApplyXorDeltaToView(view, delta, kWidth,
                                                    kStride, /*copy=*/false);
         }));
}

// Megabytes are of decoded sound.
void BenchAudio() {
  constexpr int kBlockBytes = 64 * 1024;
  base::Lcg lcg(2);
  const auto adpcm = lcg.Bytes(kBlockBytes);
  Report("ima adpcm, .AUD block",
         MegabytesPerSecond(adpcm.size() * 4,
                            [&] {
                              AdpcmState state;
                              DecodeAdpcmBlock(state, adpcm);
                            }),
         MegabytesPerSecond(adpcm.size() * 4, [&] {
           AdpcmState state;
           aud_decoder_reference::DecodeAdpcmBlock(state, adpcm);
         }));

  // The movie loader's layout: the stream at the end of its destination.
  std::vector<uint8_t> sound(adpcm.size() * 4);
  const auto stream = std::span(sound).subspan(adpcm.size() * 3);
  const auto refill = [&] {
    std::ranges::transform(adpcm, stream.begin(), [](const std::byte value) {
      return std::to_integer<uint8_t>(value);
    });
  };
  Report("ima adpcm, movie chunk",
         MegabytesPerSecond(sound.size(),
                            [&] {
                              refill();
                              ImaAdpcmDecoder().Decode(stream, sound);
                            }),
         MegabytesPerSecond(sound.size(), [&] {
           refill();
           adpcm_decoders_reference::ImaAdpcmDecoder().Decode(stream, sound);
         }));

  // Random commands, each with its whole payload.
  std::vector<std::byte> westwood;
  while (std::ssize(westwood) < kBlockBytes) {
    const uint8_t command = lcg.Byte();
    westwood.push_back(std::byte{command});
    if (command < 0x80 || (command < 0xc0 && (command & 0x20U) == 0)) {
      const auto payload = lcg.Bytes((command & 0x3fU) + 1);
      westwood.insert(westwood.end(), payload.begin(), payload.end());
    }
  }
  const std::size_t decoded =
      DecodeWestwoodBlock(westwood).value_or(std::vector<uint8_t>{}).size();
  Report("westwood 8-bit block",
         MegabytesPerSecond(decoded, [&] { DecodeWestwoodBlock(westwood); }),
         MegabytesPerSecond(decoded, [&] {
           aud_decoder_reference::DecodeWestwoodBlock(westwood);
         }));
}

// A 320x200 movie frame of 4x2 blocks, one in eight of them solid.
void BenchVq() {
  constexpr int kColumns = 80;
  constexpr int kRows = 100;
  constexpr int kStride = 320;
  constexpr int kCodebookBlocks = 2000;
  base::Lcg lcg(3);
  std::vector<uint8_t> codebook(std::size_t{kCodebookBlocks} * 8);
  std::ranges::generate(codebook, [&lcg] { return lcg.Byte(); });
  constexpr std::size_t kBlocks = std::size_t{kColumns} * kRows;
  std::vector<uint8_t> pointers(kBlocks * 2);
  for (std::size_t block = 0; block < kBlocks; ++block) {
    const bool solid = lcg.Below(8) == 0;
    const auto code = static_cast<uint32_t>(lcg.Below(kCodebookBlocks));
    pointers.at(block) = static_cast<uint8_t>(code & 0xffU);
    pointers.at(kBlocks + block) =
        solid ? uint8_t{0x0f} : static_cast<uint8_t>(code >> 8U);
  }
  std::vector<uint8_t> image(std::size_t{kStride} * kRows * 2);
  Report("vq frame, 4x2 blocks",
         MegabytesPerSecond(image.size(),
                            [&] {
                              DecodeVqFrame(BlockShape::k4x2, codebook,
                                            pointers, image, kColumns, kRows,
                                            kStride);
                            }),
         MegabytesPerSecond(image.size(), [&] {
           vq_decoder_reference::DecodeVqFrame(BlockShape::k4x2, codebook,
                                               pointers, image, kColumns, kRows,
                                               kStride);
         }));
}

// Each side with the fastest code it has for this CPU.
void BenchSha1() {
  base::Lcg lcg(4);
  const auto blocks = lcg.Bytes(1024 * engine::crypto::kSha1BlockSize);
  engine::crypto::Sha1State state{};
  Report(
      "sha-1",
      MegabytesPerSecond(blocks.size(),
                         [&] { engine::crypto::Sha1Compress(state, blocks); }),
      MegabytesPerSecond(blocks.size(),
                         [&] { sha1_reference::Sha1Compress(state, blocks); }));
}

void BenchBlowfish() {
  base::Lcg lcg(5);
  const auto key = lcg.Bytes(BlowfishEngine::kMaxKeyLength);
  auto data = lcg.Bytes(64 * 1024);
  BlowfishEngine engine;
  blowfish_reference::BlowfishEngine reference;
  engine.Submit_Key(key);
  reference.Submit_Key(key);
  Report(
      "blowfish, encrypt",
      MegabytesPerSecond(data.size(), [&] { engine.Encrypt(data, data); }),
      MegabytesPerSecond(data.size(), [&] { reference.Encrypt(data, data); }));
  Report(
      "blowfish, decrypt",
      MegabytesPerSecond(data.size(), [&] { engine.Decrypt(data, data); }),
      MegabytesPerSecond(data.size(), [&] { reference.Decrypt(data, data); }));
  // A key set-up costs what encrypting 4,168 bytes of tables does.
  constexpr std::size_t kTableBytes = 4168;
  Report("blowfish, key set-up",
         MegabytesPerSecond(kTableBytes, [&] { engine.Submit_Key(key); }),
         MegabytesPerSecond(kTableBytes, [&] { reference.Submit_Key(key); }));
}

}  // namespace

int main() {
  BenchXorDelta();
  BenchAudio();
  BenchVq();
  BenchSha1();
  BenchBlowfish();
  return 0;
}
