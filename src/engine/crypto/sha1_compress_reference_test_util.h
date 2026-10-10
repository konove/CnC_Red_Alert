// The C++ SHA-1 block compression that engine_rs's sha1 module replaced
// (src/rust/src/sha1.rs), kept so that a differential test and kernel_bench
// can hold the Rust one to it.

#ifndef CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_REFERENCE_TEST_UTIL_H_

#include <cstddef>
#include <span>

#include "engine/crypto/sha1_compress.h"

namespace sha1_reference {

// As the functions of the same names in engine/crypto/sha1_compress.h.
void Sha1Compress(engine::crypto::Sha1State& state,
                  std::span<const std::byte> blocks);
void Sha1CompressPortable(engine::crypto::Sha1State& state,
                          std::span<const std::byte> blocks);
[[nodiscard]] bool Sha1HardwareAvailable();
void Sha1CompressHardware(engine::crypto::Sha1State& state,
                          std::span<const std::byte> blocks);

}  // namespace sha1_reference

#endif  // CNC_RED_ALERT_ENGINE_CRYPTO_SHA1_COMPRESS_REFERENCE_TEST_UTIL_H_
