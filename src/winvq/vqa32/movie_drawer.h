// File: MovieDrawer, which takes a VQA movie's loaded frames as the clock
// says they are due, decodes them and shows them through the VqaClient.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "absl/base/attributes.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/movie_clock.h"
#include "winvq/vqa32/vq_decoder.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqa_player.h"

// What MovieDrawer::DrawNextFrame() did.
enum class DrawStatus {
  kDrawn,    // A frame was decoded and shown.
  kNotTime,  // The next frame is not due yet.
  kNoFrame,  // The loader has not loaded the next frame yet.
  kStopped,  // The client asked to stop the movie.
};

// Draws a movie's frames from a FrameRing in step with a MovieClock, into an
// image of its own the size of the movie, and hands each to the client. When
// playback runs late and skipping is allowed it drops the frames the clock
// has passed, except key frames, carrying a dropped frame's palette over to
// the next frame shown.
//
// Example:
//   MovieDrawer drawer(ring, clock, audio, header, client, false);
//   if (drawer.DrawNextFrame() == DrawStatus::kDrawn) { ... }
class MovieDrawer {
 public:
  // Reads the frames from ring, the time from clock, and whether the sound has
  // run dry from audio (nullptr without sound). With skip_late_frames clear,
  // late frames are only dropped once the sound has run dry. ring, clock,
  // audio and client must outlive the drawer.
  MovieDrawer(FrameRing& ring ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const MovieClock& clock ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const AudioRing* audio ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const VqaHeader& header,
              VqaClient& client ABSL_ATTRIBUTE_LIFETIME_BOUND,
              bool skip_late_frames);

  // Selects the frame that is due, decodes it, shows it through the client,
  // and frees its buffer for the loader.
  DrawStatus DrawNextFrame();

  // Number of the last frame shown; 0 before the first.
  [[nodiscard]] int last_drawn_frame() const { return last_drawn_frame_; }

 private:
  // Moves the ring's draw frame on to the frame to draw now, dropping late
  // ones. Returns kDrawn when there is one to draw, or why not.
  DrawStatus SelectFrame();

  FrameRing* ring_;
  const MovieClock* clock_;
  const AudioRing* audio_;
  VqaClient* client_;
  bool skip_late_frames_;
  int frame_rate_;

  // The decoded frame, width_ x height_ pixels.
  std::vector<uint8_t> image_;
  int width_;
  int height_;
  // The shape of the blocks frames are decoded from; nullopt for a block size
  // with no decoder, whose frames stay black.
  std::optional<BlockShape> block_shape_;
  // The image size in blocks, the geometry the decoder walks.
  int blocks_per_row_;
  int block_rows_;

  // The palette of the last frame dropped with one, to show with the next
  // frame drawn; empty when there is none.
  std::vector<uint8_t> carried_palette_;

  // Number of the last frame selected for drawing. The drawer draws
  // regardless of the clock once frame_rate / 5 frames have passed since, so
  // at least 5 frames a second reach the screen.
  int last_selected_frame_ = 0;
  int last_drawn_frame_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_
