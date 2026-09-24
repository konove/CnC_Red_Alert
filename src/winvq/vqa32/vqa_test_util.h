// File: helpers for the VQA player tests: an in-memory VqaIo and builders
// for small synthetic movies. Test-only; nothing in vqa32 includes it.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_VQA_TEST_UTIL_H_
#define CNC_RED_ALERT_WINVQ_VQA32_VQA_TEST_UTIL_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/seek_origin.h"
#include "base/types.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"

// Scripted in-memory file source. Records how the player drives it so
// tests can assert on the interaction.
class FakeVqaIo final : public VqaIo {
 public:
  bool Open(std::string_view /*name*/) override {
    opens++;
    if (fail_open) {
      return false;
    }
    pos = 0;
    return true;
  }

  bool Read(std::span<std::byte> buffer) override {
    const int64_t bytes = std::ssize(buffer);
    if (fail_read || pos + bytes > static_cast<int64_t>(data.size())) {
      return false;
    }
    base::CopyBytes(buffer,
                    std::as_bytes(std::span(data).subspan(base::ToSize(pos))),
                    buffer.size());
    pos += bytes;
    return true;
  }

  // Like a real file, refuses to move outside the data.
  bool Seek(base::ssize offset, SeekOrigin origin) override {
    int64_t target = offset;
    switch (origin) {
      case SeekOrigin::kCurrent:
        target += pos;
        break;
      case SeekOrigin::kEnd:
        target += static_cast<int64_t>(data.size());
        break;
      case SeekOrigin::kBegin:
      default:
        break;
    }
    if (target < 0 || std::cmp_greater(target, data.size())) {
      return false;
    }
    pos = target;
    return true;
  }

  void Close() override { closes++; }

  std::vector<uint8_t> data;
  int64_t pos = 0;
  bool fail_open = false;
  bool fail_read = false;
  int opens = 0;
  int closes = 0;
};

inline void AppendBytes(std::vector<uint8_t>& out, std::string_view text) {
  out.insert(out.end(), text.begin(), text.end());
}

// LLVM 23 mistakes element invalidation for invalidating the vector reference;
// no element reference or iterator is retained across these appends.
// NOLINTNEXTLINE(clang-diagnostic-lifetime-safety-invalidation)
inline void AppendBigEndian32(std::vector<uint8_t>& out, uint32_t value) {
  out.push_back(static_cast<uint8_t>(value >> 24));
  out.push_back(static_cast<uint8_t>(value >> 16));
  out.push_back(static_cast<uint8_t>(value >> 8));
  out.push_back(static_cast<uint8_t>(value));
}

// "FORM" <size> "WVQA" — the file preamble OpenVqa() validates first.
inline std::vector<uint8_t> ValidPreamble() {
  std::vector<uint8_t> data;
  AppendBytes(data, "FORM");
  AppendBigEndian32(data, 0x1234);
  AppendBytes(data, "WVQA");
  return data;
}

// Appends an IFF chunk: id, big-endian declared size, payload and the pad
// byte for odd payloads. declared_size may disagree with the payload to
// model malformed files.
inline void AppendChunk(std::vector<uint8_t>& out, std::string_view id,
                        uint32_t declared_size,
                        const std::vector<uint8_t>& payload) {
  // Built locally and appended once, so out is modified in a single step.
  std::vector<uint8_t> chunk;
  AppendBytes(chunk, id);
  AppendBigEndian32(chunk, declared_size);
  chunk.insert(chunk.end(), payload.begin(), payload.end());
  if (payload.size() % 2 != 0) {
    chunk.push_back(0);
  }
  out.insert(out.end(), chunk.begin(), chunk.end());
}

inline void AppendChunk(std::vector<uint8_t>& out, std::string_view id,
                        const std::vector<uint8_t>& payload) {
  AppendChunk(out, id, static_cast<uint32_t>(payload.size()), payload);
}

// A 3-frame 8x8 movie with 4x2 blocks and a 16-entry codebook. The loader
// derives these buffer sizes from it:
//   codebook_capacity  = (16 * 4 * 2 + 250) & 0xFFFC = 376
//   pointers_capacity = (2 * 4 * 2 + 1024) & 0xFFFC = 1040
//   palette_capacity = (768 + 1024) & 0xFFFC       = 1792
inline VqaHeader SmallHeader() {
  VqaHeader header{};
  header.version = kVqaVersion2;
  header.frame_count = 3;
  header.image_width = 8;
  header.image_height = 8;
  header.block_width = 4;
  header.block_height = 2;
  header.fps = 15;
  header.frames_per_group = 1;
  header.codebook_entries = 16;
  return header;
}

// SmallHeader() with a sound track of 22050 Hz, 8-bit mono.
inline VqaHeader SmallSoundHeader() {
  VqaHeader header = SmallHeader();
  header.flags = kVqaHasAudio;
  header.sample_rate = 22050;
  header.channels = 1;
  header.bits_per_sample = 8;
  return header;
}

inline std::vector<uint8_t> HeaderPayload(const VqaHeader& header) {
  std::vector<uint8_t> payload(sizeof(header));
  base::CopyBytes(std::as_writable_bytes(std::span(payload)),
                  base::ObjectBytes(header), sizeof(header));
  return payload;
}

// FINF entries are 4 bytes each, stored in native (little-endian) order.
inline std::vector<uint8_t> FinfPayload(const std::vector<uint32_t>& entries) {
  std::vector<uint8_t> payload(entries.size() * sizeof(uint32_t));
  base::CopyBytes(std::as_writable_bytes(std::span(payload)),
                  std::as_bytes(std::span(entries)), payload.size());
  return payload;
}

// Preamble, VQHD and FINF: everything OpenVqa() reads before the frames.
inline std::vector<uint8_t> MovieStart(const VqaHeader& header,
                                       const std::vector<uint32_t>& entries) {
  std::vector<uint8_t> data = ValidPreamble();
  AppendChunk(data, "VQHD", HeaderPayload(header));
  AppendChunk(data, "FINF", FinfPayload(entries));
  return data;
}

// Appends the chunk that completes a frame: uncompressed vector pointers.
inline void AppendFrameEnd(std::vector<uint8_t>& data) {
  AppendChunk(data, "VPT0", std::vector<uint8_t>(2));
}

// A movie of frame_count frames, each only the chunk that completes it.
inline std::vector<uint8_t> EmptyFrames(const VqaHeader& header) {
  std::vector<uint8_t> data =
      MovieStart(header, std::vector<uint32_t>(header.frame_count));
  for (int i = 0; i < int{header.frame_count}; ++i) {
    AppendFrameEnd(data);
  }
  return data;
}

#endif  // CNC_RED_ALERT_WINVQ_VQA32_VQA_TEST_UTIL_H_
