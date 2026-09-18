// SHA-1 block compression: the part of the hash that runs once per 64 bytes
// of input. SHAEngine (tech/sha.h) handles padding and partial blocks and
// calls this for the whole blocks.
#ifndef CNC_RED_ALERT_TECH_SHA1_COMPRESS_H_
#define CNC_RED_ALERT_TECH_SHA1_COMPRESS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace tech {

// The five 32-bit words SHA-1 accumulates, A first.
using Sha1State = std::array<uint32_t, 5>;

inline constexpr int kSha1BlockSize = 64;

// Compresses `blocks`, whose size must be a multiple of kSha1BlockSize, into
// `state`. Uses the x86 SHA extensions when the CPU has them, which are about
// ten times faster than the portable code; the result is identical.
void Sha1Compress(Sha1State& state, std::span<const std::byte> blocks);

// The two implementations Sha1Compress chooses between, exposed so tests can
// compare them. Sha1CompressHardware requires Sha1HardwareAvailable().
void Sha1CompressPortable(Sha1State& state, std::span<const std::byte> blocks);
[[nodiscard]] bool Sha1HardwareAvailable();
void Sha1CompressHardware(Sha1State& state, std::span<const std::byte> blocks);

}  // namespace tech

#endif  // CNC_RED_ALERT_TECH_SHA1_COMPRESS_H_
