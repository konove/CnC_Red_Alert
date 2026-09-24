// File: MovieClock, the clock a VQA movie's frames are paced by.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_MOVIE_CLOCK_H_
#define CNC_RED_ALERT_WINVQ_VQA32_MOVIE_CLOCK_H_

#include <cstdint>

class AudioOutput;

// Resolution of a movie's clock: frame times are in ticks of 1/60 second.
inline constexpr int kVqaTicksPerSecond = 60;

// A clock in kVqaTicksPerSecond that runs either from the movie's sound, so
// the frames keep in step with what is heard, or from the system clock.
//
// Example:
//   clock.Set(first_frame_ticks, output.playing() ? &output : nullptr);
//   ...
//   if (clock.Now() >= due_ticks) Draw();
class MovieClock {
 public:
  // Makes the clock read now_ticks, and from here on run from the sound
  // audio plays, or from the system clock when audio is nullptr. audio must
  // stay alive and playing until the next Set().
  void Set(int64_t now_ticks, const AudioOutput* audio);

  [[nodiscard]] int64_t Now() const { return RawTicks() + offset_ticks_; }

 private:
  // The chosen source's reading, without the offset.
  [[nodiscard]] int64_t RawTicks() const;

  const AudioOutput* audio_ = nullptr;
  // Added to the source's reading so the clock reads what Set() was given.
  int64_t offset_ticks_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_MOVIE_CLOCK_H_
