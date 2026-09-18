#include "tech/sha1_compress.h"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "absl/log/check.h"
#include "base/array.h"
#include "port/unaligned.h"

#if defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#include <immintrin.h>
#endif

namespace tech {
namespace {

// Reads a big-endian word, whatever the host byte order.
uint32_t ReadBigEndian32(std::span<const std::byte> bytes) {
  const auto value = port::ReadUnaligned<uint32_t>(bytes);
  if constexpr (std::endian::native == std::endian::little) {
    return std::byteswap(value);
  }
  return value;
}

// The round function and constant for SHA-1 round `index`, 0..79.
uint32_t RoundFunction(int index, uint32_t b, uint32_t c, uint32_t d) {
  if (index < 20) {
    return d ^ (b & (c ^ d));
  }
  if (index < 40 || index >= 60) {
    return b ^ c ^ d;
  }
  return (b & c) | (d & (b | c));
}

uint32_t RoundConstant(int index) {
  constexpr std::array<uint32_t, 4> kConstants = {0x5a827999U, 0x6ed9eba1U,
                                                  0x8f1bbcdcU, 0xca62c1d6U};
  return base::At(std::span(kConstants), index / 20);
}

void CompressBlockPortable(Sha1State& state,
                           std::span<const std::byte, kSha1BlockSize> block) {
  std::array<uint32_t, 80> schedule_words{};
  const std::span<uint32_t, 80> schedule(schedule_words);
  for (int index = 0; index < 16; ++index) {
    base::At(schedule, index) =
        ReadBigEndian32(block.subspan(static_cast<std::size_t>(index) * 4));
  }
  for (int index = 16; index < 80; ++index) {
    base::At(schedule, index) = std::rotl(
        base::At(schedule, index - 3) ^ base::At(schedule, index - 8) ^
            base::At(schedule, index - 14) ^ base::At(schedule, index - 16),
        1);
  }

  auto [a, b, c, d, e] = state;
  for (int index = 0; index < 80; ++index) {
    const uint32_t temp = std::rotl(a, 5) + RoundFunction(index, b, c, d) + e +
                          base::At(schedule, index) + RoundConstant(index);
    e = d;
    d = c;
    c = std::rotl(b, 30);
    b = a;
    a = temp;
  }
  std::get<0>(state) += a;
  std::get<1>(state) += b;
  std::get<2>(state) += c;
  std::get<3>(state) += d;
  std::get<4>(state) += e;
}

#if defined(__x86_64__) || defined(__i386__)

// This section exists to use the x86 SHA intrinsics; it is compiled only for
// x86, and Sha1Compress falls back to the portable code everywhere else.
// NOLINTBEGIN(portability-simd-intrinsics)

// Register state for the SHA extensions. ABCD holds A..D with A in the top
// lane; e alternates between the two E registers from one group of four
// rounds to the next, and msg holds the four message-schedule registers,
// which rotate through the rounds.
struct ShaNiRegisters {
  __m128i abcd;
  std::array<__m128i, 2> e;
  std::array<__m128i, 4> msg;
};

// Runs rounds 4*G to 4*G+3 of one block. G is a template parameter because
// the round-function selector of sha1rnds4 must be an immediate. Each group
// also advances the message schedule for the groups after it; the if
// constexpr conditions skip exactly the steps whose results no later group
// reads, as in Intel's reference sequence.
template <int G>
[[gnu::always_inline, gnu::target("sha,sse4.1")]] inline void ShaNiRounds(
    ShaNiRegisters& regs, std::span<const std::byte, kSha1BlockSize> block,
    __m128i byte_swap) {
  constexpr int kCur = G % 4;
  if constexpr (G < 4) {
    std::get<kCur>(regs.msg) = _mm_shuffle_epi8(
        port::ReadUnaligned<__m128i>(block.template subspan<G * 16, 16>()),
        byte_swap);
  }
  const __m128i msg = std::get<kCur>(regs.msg);
  __m128i& e = std::get<G % 2>(regs.e);
  if constexpr (G == 0) {
    e = _mm_add_epi32(e, msg);
  } else {
    e = _mm_sha1nexte_epu32(e, msg);
  }
  std::get<(G + 1) % 2>(regs.e) = regs.abcd;
  if constexpr (G >= 3 && G <= 18) {
    __m128i& next = std::get<(kCur + 1) % 4>(regs.msg);
    next = _mm_sha1msg2_epu32(next, msg);
  }
  regs.abcd = _mm_sha1rnds4_epu32(regs.abcd, e, G / 5);
  if constexpr (G >= 1 && G <= 16) {
    __m128i& prev = std::get<(kCur + 3) % 4>(regs.msg);
    prev = _mm_sha1msg1_epu32(prev, msg);
  }
  if constexpr (G >= 2 && G <= 17) {
    __m128i& after_next = std::get<(kCur + 2) % 4>(regs.msg);
    after_next = _mm_xor_si128(after_next, msg);
  }
}

template <int... G>
[[gnu::always_inline, gnu::target("sha,sse4.1")]] inline void ShaNiBlock(
    ShaNiRegisters& regs, std::span<const std::byte, kSha1BlockSize> block,
    __m128i byte_swap, std::integer_sequence<int, G...> /*groups*/) {
  (ShaNiRounds<G>(regs, block, byte_swap), ...);
}

[[gnu::target("sha,sse4.1")]] void CompressHardware(
    Sha1State& state, std::span<const std::byte> blocks) {
  // Reverses the bytes of each 32-bit word: SHA-1 reads its input big-endian.
  const __m128i byte_swap =
      _mm_set_epi64x(0x0001020304050607LL, 0x08090a0b0c0d0e0fLL);

  const auto state_bytes = std::as_writable_bytes(std::span(state));
  ShaNiRegisters regs{};
  regs.abcd = _mm_shuffle_epi32(
      port::ReadUnaligned<__m128i>(state_bytes.first(16)), 0x1B);
  std::get<0>(regs.e) =
      _mm_set_epi32(static_cast<int>(std::get<4>(state)), 0, 0, 0);

  for (; !blocks.empty(); blocks = blocks.subspan(kSha1BlockSize)) {
    const __m128i abcd_save = regs.abcd;
    const __m128i e_save = std::get<0>(regs.e);
    ShaNiBlock(regs, blocks.first<kSha1BlockSize>(), byte_swap,
               std::make_integer_sequence<int, 20>{});
    // Round 79 leaves E in e[0], which still needs the saved E added the
    // way sha1nexte does it.
    std::get<0>(regs.e) = _mm_sha1nexte_epu32(std::get<0>(regs.e), e_save);
    regs.abcd = _mm_add_epi32(regs.abcd, abcd_save);
  }

  port::WriteUnaligned(state_bytes.first(16),
                       _mm_shuffle_epi32(regs.abcd, 0x1B));
  std::get<4>(state) =
      static_cast<uint32_t>(_mm_extract_epi32(std::get<0>(regs.e), 3));
}

// NOLINTEND(portability-simd-intrinsics)

#endif

}  // namespace

void Sha1CompressPortable(Sha1State& state, std::span<const std::byte> blocks) {
  CHECK_EQ(blocks.size() % kSha1BlockSize, 0U);
  for (; !blocks.empty(); blocks = blocks.subspan(kSha1BlockSize)) {
    CompressBlockPortable(state, blocks.first<kSha1BlockSize>());
  }
}

bool Sha1HardwareAvailable() {
#if defined(__x86_64__) || defined(__i386__)
  // Queried once: CPUID is slow, and the answer cannot change.
  static const bool kAvailable = [] {
    unsigned eax = 0;
    unsigned ebx = 0;
    unsigned ecx = 0;
    unsigned edx = 0;
    if (__get_cpuid(1, &eax, &ebx, &ecx, &edx) == 0) {
      return false;
    }
    const bool has_sse41 = (ecx & bit_SSE4_1) != 0;
    if (__get_cpuid_count(7, 0, &eax, &ebx, &ecx, &edx) == 0) {
      return false;
    }
    const bool has_sha = (ebx & bit_SHA) != 0;
    return has_sse41 && has_sha;
  }();
  return kAvailable;
#else
  return false;
#endif
}

void Sha1CompressHardware(Sha1State& state, std::span<const std::byte> blocks) {
  CHECK(Sha1HardwareAvailable());
  CHECK_EQ(blocks.size() % kSha1BlockSize, 0U);
#if defined(__x86_64__) || defined(__i386__)
  CompressHardware(state, blocks);
#else
  static_cast<void>(state);
#endif
}

void Sha1Compress(Sha1State& state, std::span<const std::byte> blocks) {
  if (Sha1HardwareAvailable()) {
    Sha1CompressHardware(state, blocks);
  } else {
    Sha1CompressPortable(state, blocks);
  }
}

}  // namespace tech
