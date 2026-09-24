#include "winvq/vqa32/movie_clock.h"

#include <chrono>
#include <cstdint>

#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/vqa_player.h"

void MovieClock::Set(const int64_t now_ticks, const AudioOutput* const audio) {
  audio_ = audio;
  offset_ticks_ = now_ticks - RawTicks();
}

int64_t MovieClock::RawTicks() const {
  if (audio_ != nullptr) {
    return audio_->PlayedTicks();
  }
  // steady_clock, not the wall clock: an adjustment to that mid-movie would
  // drop a run of frames, or freeze the movie while it caught up.
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count();
  return ms * kVqaTicksPerSecond / 1000;
}
