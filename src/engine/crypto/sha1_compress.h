// SHA-1 block compression: the part of the hash that runs once per 64 bytes
// of input. SHAEngine (engine/crypto/sha.h) handles padding and partial
// blocks and calls this for the whole blocks.
#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace engine::crypto {

// The five 32-bit words SHA-1 accumulates, A first.
using Sha1State = std::array<uint32_t, 5>;

inline constexpr int kSha1BlockSize = 64;

// Compresses `blocks`, whose size must be a multiple of kSha1BlockSize, into
// `state`. Uses the SHA extensions of x86 and ARM CPUs that have them, which
// are several times faster than the portable code; the result is identical.
void Sha1Compress(Sha1State& state, std::span<const std::byte> blocks);

}  // namespace engine::crypto

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_H_
