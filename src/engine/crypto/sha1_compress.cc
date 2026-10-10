#include "engine/crypto/sha1_compress.h"

#include <cstddef>
#include <span>

#include "absl/log/check.h"
#include "engine/stream/rust_bytes.h"
#include "engine_rs_cxx/lib.h"

// The compression is engine_rs's sha1 module (src/rust/src/sha1.rs).

namespace engine::crypto {

void Sha1Compress(Sha1State& state, const std::span<const std::byte> blocks) {
  CHECK_EQ(blocks.size() % kSha1BlockSize, 0U);
  engine_rs::sha1_compress(state, AsRustSlice(blocks));
}

void Sha1CompressPortable(Sha1State& state,
                          const std::span<const std::byte> blocks) {
  CHECK_EQ(blocks.size() % kSha1BlockSize, 0U);
  engine_rs::sha1_compress_portable(state, AsRustSlice(blocks));
}

bool Sha1HardwareAvailable() { return engine_rs::sha1_hardware_available(); }

void Sha1CompressHardware(Sha1State& state,
                          const std::span<const std::byte> blocks) {
  CHECK(Sha1HardwareAvailable());
  CHECK_EQ(blocks.size() % kSha1BlockSize, 0U);
  engine_rs::sha1_compress_hardware(state, AsRustSlice(blocks));
}

}  // namespace engine::crypto
