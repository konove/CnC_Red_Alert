// File: The C++ LCW kernels as they were before src/rust replaced them, kept
// as the reference lcw_differential_test.cc holds the Rust ones to.

#ifndef CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace lcw_reference {

// The former LCW_Uncompress(): lenient, returns the bytes written. Works in
// place: `source` may lie in `dest`'s storage at or after its start.
int32_t Uncompress(std::span<const std::byte> source,
                   std::span<std::byte> dest);

// The former LcwUncompBounded(): returns the bytes written, or -1 for a
// malformed stream.
int UncompressBounded(std::span<const std::byte> source,
                      std::span<std::byte> dest);

// The former LCW_Comp(): returns the stream size, or -1 if `out` is smaller
// than LcwWorstCaseSize(in.size()).
int Compress(std::span<const std::byte> in, std::span<std::byte> out);

}  // namespace lcw_reference

#endif  // CNC_RED_ALERT_ENGINE_CODEC_LCW_REFERENCE_TEST_UTIL_H_
