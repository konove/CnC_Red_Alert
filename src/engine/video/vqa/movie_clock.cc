#include "engine/video/vqa/movie_clock.h"

#include <cstdint>

#include "engine/platform/timer.h"
#include "engine/video/vqa/audio_output.h"

void MovieClock::Set(const int64_t now_ticks, const AudioOutput* const audio) {
  audio_ = audio;
  offset_ticks_ = now_ticks - RawTicks();
}

int64_t MovieClock::RawTicks() const {
  if (audio_ != nullptr) {
    return audio_->PlayedTicks();
  }
  // The game's monotonic clock, not the wall clock: an adjustment to that
  // mid-movie would drop a run of frames, or freeze the movie while it caught
  // up. Going through the game's clock also lets a scripted run, which moves
  // that clock by hand, play a silent movie at its own pace.
  const int64_t ms = engine::platform::SystemMilliseconds();
  return ms * kVqaTicksPerSecond / 1000;
}
