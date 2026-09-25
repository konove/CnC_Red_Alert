// File: Movie, an open VQA movie and everything playing it takes. VqaPlayer
// is its public face; only engine_vqa and its tests include this header.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_MOVIE_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_MOVIE_H_

#include <cstdint>
#include <expected>
#include <memory>
#include <string_view>

#include "absl/base/attributes.h"
#include "engine/video/vqa/audio_output.h"
#include "engine/video/vqa/audio_ring.h"
#include "engine/video/vqa/frame_ring.h"
#include "engine/video/vqa/movie_clock.h"
#include "engine/video/vqa/movie_drawer.h"
#include "engine/video/vqa/movie_loader.h"
#include "engine/video/vqa/vqa_audio_device.h"
#include "engine/video/vqa/vqa_format.h"
#include "engine/video/vqa/vqa_player.h"
#include "engine/video/vqa/vqaio.h"

// An open movie. Open() reads the header, allocates the play buffers, sets up
// the sound and preloads the frames; destruction stops the sound and closes
// the file. Playback takes turns between the loader and the drawer on the
// caller's thread, while the sound plays on the audio device's.
//
// Example:
//   auto movie = Movie::Open(io, "INTRO.VQA", client, &audio, {});
//   if (movie) (*movie)->Run();
class Movie {
 public:
  // See VqaPlayer::Open().
  static std::expected<std::unique_ptr<Movie>, VqaError> Open(
      VqaIo& io, std::string_view name, VqaClient& client,
      VqaAudioDevice* audio, const VqaOptions& options);

  ~Movie();
  Movie(const Movie&) = delete;
  Movie& operator=(const Movie&) = delete;
  Movie(Movie&&) = delete;
  Movie& operator=(Movie&&) = delete;

  // See VqaPlayer.
  void Run();
  VqaStepResult Step();
  void Pause();
  void Resume();
  [[nodiscard]] bool paused() const { return paused_; }
  [[nodiscard]] int last_frame_shown() const {
    return drawer_->last_drawn_frame();
  }
  [[nodiscard]] int frame_count() const { return header_.frame_count; }
  [[nodiscard]] int frame_rate() const { return header_.fps; }

  // The parts, for tests.
  [[nodiscard]] const VqaHeader& header() const ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return header_;
  }
  FrameRing& ring() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *ring_; }
  MovieLoader& loader() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *loader_; }
  MovieDrawer& drawer() ABSL_ATTRIBUTE_LIFETIME_BOUND { return *drawer_; }
  MovieClock& clock() ABSL_ATTRIBUTE_LIFETIME_BOUND { return clock_; }
  // nullptr when the movie plays without sound.
  AudioRing* audio() ABSL_ATTRIBUTE_LIFETIME_BOUND { return audio_.get(); }
  AudioOutput* audio_output() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return audio_output_.get();
  }

 private:
  Movie(VqaIo& io ABSL_ATTRIBUTE_LIFETIME_BOUND,
        VqaClient& client ABSL_ATTRIBUTE_LIFETIME_BOUND);

  // Reads the chunks up to the frame table, VQHD among them, and allocates
  // the play buffers VQHD sizes.
  std::expected<void, VqaError> ReadHeader(VqaAudioDevice* audio,
                                           const VqaOptions& options);
  // Allocates the frame ring and, for a movie with sound played on audio, the
  // sound ring and its output.
  std::expected<void, VqaError> Allocate(VqaAudioDevice* audio,
                                         const VqaOptions& options);
  // Loads frames until the frame ring is full or the movie ends.
  std::expected<void, VqaError> Preload(const VqaOptions& options);

  // Starts the sound, if some is loaded, and the clock, at the first frame.
  void Start();
  // Stops the sound; the movie has ended.
  void Finish();

  VqaIo* io_;
  VqaClient* client_;
  VqaHeader header_{};
  // Declared in the order they depend on each other, so each is destroyed
  // before what it uses: the output before the ring its mixer reads, the
  // loader and drawer before everything else.
  std::unique_ptr<FrameRing> ring_;
  std::unique_ptr<AudioRing> audio_;
  std::unique_ptr<AudioOutput> audio_output_;
  MovieClock clock_;
  std::unique_ptr<MovieLoader> loader_;
  std::unique_ptr<MovieDrawer> drawer_;

  bool started_ = false;
  bool ended_ = false;
  bool paused_ = false;
  // What the clock read when the movie was paused, and whether the sound was
  // playing then; Resume() picks up from both.
  int64_t paused_ticks_ = 0;
  bool paused_sound_ = false;
  // The loader has read the whole movie, or failed.
  bool loaded_ = false;
};

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_MOVIE_H_
