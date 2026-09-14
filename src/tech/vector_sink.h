// File: VectorSink, a byte sink that appends to a std::vector.

#ifndef CNC_RED_ALERT_TECH_VECTOR_SINK_H_
#define CNC_RED_ALERT_TECH_VECTOR_SINK_H_

#include <cstddef>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "tech/byte_sink.h"

// A chain terminator that appends every byte to a caller-owned vector. It
// never fails.
//
// Example:
//   std::vector<std::byte> packed;
//   VectorSink sink(packed);
//   LzoSink compressor(CodecMode::kCompress, sink);
class VectorSink : public ByteSink {
 public:
  // bytes must outlive the sink.
  explicit VectorSink(
      std::vector<std::byte>& bytes ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : bytes_(bytes) {}

  bool Write(std::span<const std::byte> data) override {
    bytes_.insert(bytes_.end(), data.begin(), data.end());
    return true;
  }

 private:
  std::vector<std::byte>& bytes_;
};

#endif  // CNC_RED_ALERT_TECH_VECTOR_SINK_H_
