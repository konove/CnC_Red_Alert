// Detects incomplete writes of the uncompressed TD save body.

#ifndef CNC_RED_ALERT_TD_SAVEPIPE_H_
#define CNC_RED_ALERT_TD_SAVEPIPE_H_

#include "tech/pipe.h"

// ArchiveWriter does not interpret Put's result because compression pipes
// can buffer data. TD saves directly to an unbuffered file sink, where a
// short write is an error. The sink must outlive this pipe.
class SaveGamePipe : public Pipe {
 public:
  explicit SaveGamePipe(Pipe& sink) { SetSink(sink); }

  int Put(const void* source, int length) override {
    if (!ok_) {
      return 0;
    }
    const int written = Pipe::Put(source, length);
    ok_ = written == length;
    return written;
  }

  bool ok() const { return ok_; }

 private:
  bool ok_ = true;
};

#endif  // CNC_RED_ALERT_TD_SAVEPIPE_H_
