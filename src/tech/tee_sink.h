// Copies a sink's input to an optional diagnostic sink.

#ifndef CNC_RED_ALERT_TECH_TEE_SINK_H_
#define CNC_RED_ALERT_TECH_TEE_SINK_H_

#include <cstddef>
#include <span>

#include "absl/base/attributes.h"
#include "tech/byte_sink.h"

// Forwards bytes to the main sink and copies them to a diagnostic sink.
// Both sinks must outlive this sink. Diagnostic failures never affect the
// main stream; copy_ok() reports incomplete copies. Inherited Flush/Finish only
// flush the main sink; the caller must finalize the diagnostic sink separately.
class TeeSink : public ChainedSink {
 public:
  // A null copy sink disables copying.
  TeeSink(ByteSink& main_sink,
          ByteSink* copy_sink ABSL_ATTRIBUTE_LIFETIME_BOUND)
      : ChainedSink(main_sink), copy_sink_(copy_sink) {}

  bool Write(std::span<const std::byte> bytes) override {
    if (copy_sink_ != nullptr && copy_ok_ && !bytes.empty()) {
      copy_ok_ = copy_sink_->Write(bytes);
    }
    return ChainedSink::Write(bytes);
  }

  [[nodiscard]] bool copy_ok() const { return copy_ok_; }

 private:
  ByteSink* copy_sink_;  // nullptr when copying is disabled.
  bool copy_ok_ = true;
};

#endif  // CNC_RED_ALERT_TECH_TEE_SINK_H_
