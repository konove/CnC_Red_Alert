// Copies a pipe's input to an optional diagnostic sink.

#ifndef CNC_RED_ALERT_TECH_TEEPIPE_H_
#define CNC_RED_ALERT_TECH_TEEPIPE_H_

#include "tech/pipe.h"

// Forwards bytes to the main sink and copies them to a diagnostic sink.
// Both sinks must outlive this pipe. Diagnostic failures never affect the
// main stream; copy_ok() reports incomplete copies. Inherited Flush/End only
// flush the main sink; the caller must finalize the diagnostic sink separately.
class TeePipe : public Pipe {
 public:
  // A null copy sink disables copying.
  TeePipe(Pipe& main_sink, Pipe* copy_sink) : copy_sink_(copy_sink) {
    SetSink(main_sink);
  }

  int Put(const void* source, int length) override {
    if (copy_sink_ != nullptr && copy_ok_ && source != nullptr && length > 0) {
      copy_ok_ = copy_sink_->Put(source, length) == length;
    }
    return Pipe::Put(source, length);
  }

  [[nodiscard]] bool copy_ok() const { return copy_ok_; }

 private:
  Pipe* copy_sink_;  // nullptr when copying is disabled.
  bool copy_ok_ = true;
};

#endif  // CNC_RED_ALERT_TECH_TEEPIPE_H_
