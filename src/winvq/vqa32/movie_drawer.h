// File: MovieDrawer, which takes a VQA movie's loaded frames as the clock
// says they are due and decodes them into the image buffer.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_

#include <array>
#include <cstdint>
#include <optional>
#include <span>
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
  kDrawn,    // A frame was drawn and shown.
  kNotTime,  // The next frame is not due yet.
  kNoFrame,  // The loader has not loaded the next frame yet.
  kStopped,  // The frame callback asked to stop the movie.
};

// Where the image sits in the image buffer, in buffer pixels.
struct ImagePlacement {
  // Inclusive image corners. x1,y1 is the corner anchored by the
  // kVqaDrawOriginMask flags, x2,y2 the opposite one.
  int x1 = 0;
  int y1 = 0;
  int x2 = 0;
  int y2 = 0;
  // Index in the buffer of the image's top-left pixel.
  int offset = 0;
};

// Draws a movie's frames from a FrameRing in step with a MovieClock. When
// playback runs late it skips the frames the clock has passed, except key
// frames, carrying a skipped frame's palette over to the next frame drawn. It
// decodes into the caller's image buffer or, when there is none and the
// configuration decodes, a buffer of its own the size of the movie.
//
// Example:
//   MovieDrawer drawer(ring, clock, audio, header, config);
//   if (drawer.DrawNextFrame() == DrawStatus::kDrawn) { ... }
class MovieDrawer {
 public:
  // Reads the frames from ring, the time from clock, and whether the sound has
  // run dry from audio (nullptr without sound). config is the movie's copy,
  // with its defaults resolved. All of them must outlive the drawer.
  MovieDrawer(FrameRing& ring ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const MovieClock& clock ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const AudioRing* audio ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const VqaHeader& header,
              const VqaConfig& config ABSL_ATTRIBUTE_LIFETIME_BOUND);

  // Selects the frame that is due, decodes it into the image buffer, hands it
  // to the frame callback, and frees its buffer for the loader.
  DrawStatus DrawNextFrame();

  // Number of the last frame drawn; 0 before the first.
  [[nodiscard]] int last_drawn_frame() const { return last_drawn_frame_; }
  [[nodiscard]] const ImagePlacement& placement() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return placement_;
  }
  // The buffer frames are decoded into; empty when there is none.
  [[nodiscard]] std::span<unsigned char> image_buffer() const {
    return image_buffer_;
  }

 private:
  // Moves the ring's draw frame on to the frame to draw now, skipping late
  // ones. Returns kDrawn when there is one to draw, or why not.
  DrawStatus SelectFrame();

  FrameRing* ring_;
  const MovieClock* clock_;
  const AudioRing* audio_;
  const VqaConfig* config_;

  // The player's own image buffer, when it has one.
  std::vector<unsigned char> own_image_;
  // The buffer frames are decoded into, image_width_ x image_height_ pixels.
  std::span<unsigned char> image_buffer_;
  int image_width_ = 0;
  int image_height_ = 0;
  ImagePlacement placement_;

  // The shape of the blocks frames are decoded from; nullopt when nothing is
  // decoded, because kVqaDrawToBuffer is clear or the block size has no
  // decoder.
  std::optional<BlockShape> block_shape_;
  // The image size in blocks, the geometry the decoder walks.
  int blocks_per_row_ = 0;
  int block_rows_ = 0;

  // The palette of the last frame skipped with one, and its size, set with
  // the next frame drawn while palette_pending_. It stays here after that,
  // since the palette hook may read it until the frame callback returns.
  std::array<unsigned char, kMaxPaletteBytes> saved_palette_{};
  int32_t saved_palette_bytes_ = 0;
  bool palette_pending_ = false;

  // Number of the last frame selected for drawing. The drawer draws
  // regardless of the clock once frame_rate / 5 frames have passed since, so
  // at least 5 frames a second reach the screen.
  int last_selected_frame_ = 0;
  int last_drawn_frame_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_MOVIE_DRAWER_H_
