// Times the LCW kernels the engine uses (Rust, through src/rust) against the
// C++ reference they replaced, on 8 MiB of game-like data.
//
// Usage: lcw_bench

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <tuple>
#include <vector>

#include "absl/strings/str_format.h"
#include "engine/codec/lcw.h"
#include "engine/codec/lcw_reference_test_util.h"
#include "engine/codec/lcw_uncompress.h"

namespace {

constexpr int kBlockSize = 64 * 1024;
constexpr int kBlocks = 128;
constexpr auto kBlockBytes = static_cast<std::size_t>(kBlockSize);
constexpr auto kStreamBytes =
    static_cast<std::size_t>(LcwWorstCaseSize(kBlockSize));

// Runs, repeats and noise, as in lcw_differential_test.cc's Corpus().
std::vector<std::byte> GameLikeBlock(uint32_t seed) {
  std::vector<std::byte> bytes;
  const auto next = [&seed] {
    seed = (seed * 1664525U) + 1013904223U;
    return seed >> 8;
  };
  while (std::ssize(bytes) < kBlockSize) {
    const auto piece = static_cast<std::size_t>(1 + (next() % 300));
    switch (next() % 3) {
      case 0:
        bytes.insert(bytes.end(), piece, static_cast<std::byte>(next() & 0xff));
        break;
      case 1:
        if (bytes.size() > piece) {
          const auto from = next() % (bytes.size() - piece);
          for (std::size_t i = 0; i < piece; ++i) {
            bytes.push_back(bytes.at(from + i));
          }
          break;
        }
        [[fallthrough]];
      default:
        for (std::size_t i = 0; i < piece; ++i) {
          bytes.push_back(static_cast<std::byte>(next() & 0xff));
        }
        break;
    }
  }
  bytes.resize(kBlockBytes);
  return bytes;
}

template <typename Function>
double MegabytesPerSecond(const Function& function) {
  const auto start = std::chrono::steady_clock::now();
  function();
  const std::chrono::duration<double> seconds =
      std::chrono::steady_clock::now() - start;
  const double megabytes = static_cast<double>(kBlockSize) *
                           static_cast<double>(kBlocks) / (1024.0 * 1024.0);
  return megabytes / seconds.count();
}

}  // namespace

int main() {
  std::vector<std::vector<std::byte>> plain;
  std::vector<std::vector<std::byte>> streams;
  for (int block = 0; block < kBlocks; ++block) {
    plain.push_back(GameLikeBlock(static_cast<uint32_t>(block + 1)));
    streams.push_back(LCW_Comp(plain.back()));
  }
  std::vector<std::byte> output(kBlockBytes);
  std::vector<std::byte> scratch(kStreamBytes);

  const auto report = [](const char* what, double rust, double cpp) {
    absl::PrintF("%-18s Rust %8.1f MB/s   C++ %8.1f MB/s   %+.0f%%\n", what,
                 rust, cpp, ((rust / cpp) - 1.0) * 100.0);
  };
  report("lenient decode", MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             std::ignore = LcwUncompress(stream, kBlockSize);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             lcw_reference::Uncompress(stream, output);
           }
         }));
  report("bounded decode", MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             std::ignore = LcwUncompBounded(stream, kBlockSize);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& stream : streams) {
             lcw_reference::UncompressBounded(stream, output);
           }
         }));
  report("compress", MegabytesPerSecond([&] {
           for (const auto& block : plain) {
             std::ignore = LCW_Comp(block);
           }
         }),
         MegabytesPerSecond([&] {
           for (const auto& block : plain) {
             lcw_reference::Compress(block, scratch);
           }
         }));
  return 0;
}
