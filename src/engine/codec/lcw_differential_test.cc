// Holds the Rust LCW kernels in src/rust to the C++ ones they replace, byte
// for byte: the same output for valid, damaged and random streams, and the
// same compressed stream.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "base/numeric.h"
#include "base/types.h"
#include "engine/codec/lcw.h"
#include "engine/codec/lcw_reference_test_util.h"
#include "engine/codec/lcw_uncompress.h"
#include "engine/codec/rust_bytes.h"
#include "engine_rs_cxx/lib.h"
#include "gtest/gtest.h"

namespace {

constexpr std::byte kFill{0xcc};  // What untouched destination bytes hold.

// A deterministic generator, so every failure is a reproducible case.
class Lcg {
 public:
  explicit Lcg(const uint32_t seed) : state_(seed) {}

  uint32_t Next() {
    state_ = (state_ * 1664525U) + 1013904223U;
    return state_ >> 8;
  }
  // Returns a value in [0, bound); bound must be positive.
  int Below(const int bound) {
    return static_cast<int>(Next() % static_cast<uint32_t>(bound));
  }

 private:
  uint32_t state_;
};

std::vector<std::byte> RandomBytes(Lcg& lcg, const int size) {
  std::vector<std::byte> bytes(base::ToSize(size));
  std::ranges::generate(
      bytes, [&] { return static_cast<std::byte>(lcg.Next() & 0xff); });
  return bytes;
}

// Game-like data: runs, repeated patterns and noise, so every command kind
// appears in its compressed form.
std::vector<std::byte> Corpus(Lcg& lcg, const int size) {
  std::vector<std::byte> bytes;
  while (std::ssize(bytes) < size) {
    const int piece = 1 + lcg.Below(300);
    switch (lcg.Below(3)) {
      case 0:
        bytes.insert(bytes.end(), base::ToSize(piece),
                     static_cast<std::byte>(lcg.Next() & 0xff));
        break;
      case 1:
        if (!bytes.empty()) {
          const int from = lcg.Below(static_cast<int>(bytes.size()));
          for (int i = 0; i < piece; ++i) {
            bytes.push_back(bytes.at(base::ToSize(from + i)));
          }
          break;
        }
        [[fallthrough]];
      default: {
        const auto noise = RandomBytes(lcg, piece);
        bytes.insert(bytes.end(), noise.begin(), noise.end());
        break;
      }
    }
  }
  bytes.resize(base::ToSize(size));
  return bytes;
}

std::vector<std::byte> ReferenceCompress(const std::vector<std::byte>& plain) {
  std::vector<std::byte> stream(
      base::ToSize(LcwWorstCaseSize(static_cast<int>(plain.size()))));
  const int size = lcw_reference::Compress(plain, stream);
  stream.resize(base::ToSize(size));
  return stream;
}

// A valid stream with up to three bytes changed and, one time in four, its
// tail cut off.
std::vector<std::byte> Damage(Lcg& lcg, std::vector<std::byte> stream) {
  for (int flips = lcg.Below(4); flips > 0 && !stream.empty(); --flips) {
    const auto at = base::ToSize(lcg.Below(static_cast<int>(stream.size())));
    stream.at(at) ^= static_cast<std::byte>(1 + lcg.Below(255));
  }
  if (lcg.Below(4) == 0 && !stream.empty()) {
    stream.resize(base::ToSize(lcg.Below(static_cast<int>(stream.size()))));
  }
  return stream;
}

// Runs both lenient decoders on `stream` with room for `max_size` bytes.
void ExpectLenientMatch(const std::vector<std::byte>& stream,
                        const int max_size) {
  std::vector<std::byte> want(base::ToSize(max_size), kFill);
  want.resize(base::ToSize(lcw_reference::Uncompress(stream, want)));
  const auto got = LcwUncompress(stream, max_size);
  EXPECT_TRUE(std::ranges::equal(std::as_bytes(std::span(got)), want));
}

// Runs both strict decoders on `stream` with room for `max_size` bytes.
void ExpectBoundedMatch(const std::vector<std::byte>& stream,
                        const int max_size) {
  std::vector<std::byte> want(base::ToSize(max_size), kFill);
  const int want_size = lcw_reference::UncompressBounded(stream, want);
  const auto got = LcwUncompBounded(stream, max_size);
  ASSERT_EQ(got.has_value(), want_size >= 0);
  if (got) {
    want.resize(base::ToSize(want_size));
    EXPECT_TRUE(*got == want);
  }
}

TEST(LcwDifferentialTest, CompressMatchesReference) {
  Lcg lcg(1);
  for (const int size : {0, 1, 2, 3, 100, 4096, 65535, 65536, 70000, 200000}) {
    SCOPED_TRACE(size);
    const auto plain = Corpus(lcg, size);
    EXPECT_TRUE(LCW_Comp(plain) == ReferenceCompress(plain));
  }
}

TEST(LcwDifferentialTest, DamagedStreamsMatchLenientReference) {
  Lcg lcg(3);
  for (int iteration = 0; iteration < 2000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, lcg.Below(5000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    // Exact, short and roomy destinations.
    const int size = static_cast<int>(plain.size());
    ExpectLenientMatch(stream, size);
    ExpectLenientMatch(stream, lcg.Below(size + 1));
    ExpectLenientMatch(stream, size + 64);
  }
}

TEST(LcwDifferentialTest, DamagedStreamsMatchBoundedReference) {
  Lcg lcg(4);
  for (int iteration = 0; iteration < 2000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, lcg.Below(5000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    const int size = static_cast<int>(plain.size());
    ExpectBoundedMatch(stream, size);
    ExpectBoundedMatch(stream, lcg.Below(size + 1));
    ExpectBoundedMatch(stream, size + 64);
  }
}

TEST(LcwDifferentialTest, RandomBytesMatchReference) {
  Lcg lcg(5);
  for (int iteration = 0; iteration < 5000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto stream = RandomBytes(lcg, lcg.Below(400));
    ExpectLenientMatch(stream, lcg.Below(2000));
    ExpectBoundedMatch(stream, lcg.Below(2000));
  }
}

TEST(LcwDifferentialTest, InPlaceMatchesReference) {
  Lcg lcg(6);
  for (int iteration = 0; iteration < 1000; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, 1 + lcg.Below(4000));
    const auto stream = Damage(lcg, ReferenceCompress(plain));
    // The WSA layout: output from the front, the stream at the very end, the
    // buffer as large as the larger of the two plus some slack.
    const auto dest_size = plain.size();
    const auto buffer_size =
        std::max(dest_size, stream.size()) + base::ToSize(lcg.Below(64));
    std::vector<std::byte> want(buffer_size, kFill);
    const auto source_start = buffer_size - stream.size();
    std::ranges::copy(stream,
                      want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);

    const int32_t want_size =
        lcw_reference::Uncompress(std::span(want).subspan(source_start),
                                  std::span(want).first(dest_size));
    const auto got_size = engine_rs::lcw_uncompress_in_place(
        AsRustSlice(std::span(got)), source_start, stream.size(), dest_size);
    EXPECT_EQ(static_cast<int64_t>(got_size), int64_t{want_size});
    EXPECT_TRUE(got == want);
  }
}

// The callers' in-place layouts, through the public entry point: the shim
// must route them to the in-place decoder and decode exactly as before.
TEST(LcwDifferentialTest, PublicApiDecodesInPlace) {
  Lcg lcg(7);
  for (int iteration = 0; iteration < 500; ++iteration) {
    SCOPED_TRACE(iteration);
    const auto plain = Corpus(lcg, 1 + lcg.Below(4000));
    const auto stream = ReferenceCompress(plain);
    const auto buffer_size = std::max(plain.size(), stream.size()) + 16;
    // At the tail (WSA, VQA), or from the destination's first byte.
    const auto source_start =
        lcg.Below(2) == 0 ? buffer_size - stream.size() : 0;
    std::vector<std::byte> want(buffer_size, kFill);
    std::ranges::copy(stream,
                      want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);

    const auto source = [&](std::vector<std::byte>& buffer) {
      return std::span<const std::byte>(buffer).subspan(source_start,
                                                        stream.size());
    };
    const int32_t want_size = lcw_reference::Uncompress(
        source(want), std::span(want).first(plain.size()));
    const int32_t got_size =
        LcwUncompressInPlace(source(got), std::span(got).first(plain.size()));
    EXPECT_EQ(got_size, want_size);
    EXPECT_TRUE(got == want);
  }
}

// Load_Uncompress()'s layout: the packet at the tail of the buffer and the
// destination a prefix that may stop short of it, touch it, or run into it.
TEST(LcwDifferentialTest, PublicApiDecodesTailLayoutDisjoint) {
  Lcg lcg(8);
  const auto plain = Corpus(lcg, 3000);
  const auto stream = ReferenceCompress(plain);
  for (const int gap : {-100, -1, 0, 1, 100}) {
    SCOPED_TRACE(gap);
    const auto buffer_size = static_cast<std::size_t>(
        std::max<base::ssize>(std::ssize(plain) + gap, 0) + std::ssize(stream));
    std::vector<std::byte> want(buffer_size, kFill);
    const auto source_start = buffer_size - stream.size();
    std::ranges::copy(stream,
                      want.begin() + static_cast<base::ssize>(source_start));
    std::vector<std::byte> got(want);
    const int32_t want_size = lcw_reference::Uncompress(
        std::span<const std::byte>(want).subspan(source_start),
        std::span(want).first(plain.size()));
    const int32_t got_size = LcwUncompressInPlace(
        std::span<const std::byte>(got).subspan(source_start),
        std::span(got).first(plain.size()));
    EXPECT_EQ(got_size, want_size);
    EXPECT_TRUE(got == want);
  }
}

TEST(LcwDifferentialDeathTest, PublicApiRejectsSourceOverlappingTheFront) {
  std::vector<std::byte> buffer(16, std::byte{0x80});
  // NOLINTNEXTLINE(clang-diagnostic-switch-default,clang-diagnostic-unsafe-buffer-usage-in-libc-call)
  EXPECT_DEATH(LcwUncompressInPlace(std::span<const std::byte>(buffer).first(8),
                                    std::span(buffer).subspan(4, 8)),
               "overlaps the front");
}

}  // namespace
