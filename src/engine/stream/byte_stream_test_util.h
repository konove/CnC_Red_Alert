// File: ReadAll(), which lets a test compare what a ByteStream gives with a
// literal.

#ifndef CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_TEST_UTIL_H_
#define CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_TEST_UTIL_H_

#include <span>
#include <string>

#include "base/numeric.h"
#include "base/types.h"
#include "engine/stream/byte_stream.h"

// Reads up to `count` bytes from the position of `stream` and returns the ones
// it got, which are fewer than `count` when the stream ends first. `Bytes` is
// a std::string or a vector of a byte-sized type.
//
// Example:
//   MemoryStream stream(std::as_bytes(std::span("abc", 3)));
//   EXPECT_EQ(ReadAll(stream, 10), "abc");
//   EXPECT_EQ(ReadAll<std::vector<uint8_t>>(stream, 10),
//             std::vector<uint8_t>{});
template <typename Bytes = std::string>
Bytes ReadAll(ByteStream& stream, const base::ssize count) {
  Bytes out(base::ToSize(count), {});
  out.resize(base::ToSize(stream.Read(std::as_writable_bytes(std::span(out)))));
  return out;
}

#endif  // CNC_RED_ALERT_ENGINE_STREAM_BYTE_STREAM_TEST_UTIL_H_
