// File: FrameRing, the ring of frame buffers the VQA loader fills ahead of the
// drawer, and the codebooks those frames are decoded with.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_FRAME_RING_H_
#define CNC_RED_ALERT_WINVQ_VQA32_FRAME_RING_H_

#include <vector>

#include "absl/base/attributes.h"
#include "base/numeric.h"
#include "base/types.h"
#include "winvq/vqa32/lcw_buffer.h"

// The largest palette a frame can set: 256 colors of 3 bytes.
inline constexpr int kMaxPaletteBytes = 256 * 3;

// Codebook: the table of pixel blocks a frame's vector pointers index into.
// One codebook serves every frame of a group, and the loader assembles the
// next group's codebook from pieces carried by the current group's frames.
struct Codebook {
  explicit Codebook(base::ssize capacity) : data(capacity) {}

  LcwBuffer data;
};

// Frame: one buffer in the ring of loaded frames.
struct Frame {
  Frame(base::ssize pointers_capacity, base::ssize palette_capacity)
      : pointers(pointers_capacity), palette(palette_capacity) {}

  // The frame's vector pointers, one 16-bit entry per block.
  LcwBuffer pointers;
  // The frame's palette as 8-bit R,G,B triplets. Holds a stale palette when
  // has_palette is false.
  LcwBuffer palette;
  // Index in the ring's codebooks of the one pointers index into.
  int codebook = 0;
  // Number of the frame in the movie.
  int frame_number = 0;
  // Filled by the loader and waiting to be drawn; otherwise free to fill.
  bool loaded = false;
  // A key frame, which the drawer never skips.
  bool key = false;
  // Carries a palette that must be set when the frame is shown.
  bool has_palette = false;
};

// The frames and codebooks of an open movie. The loader fills frames in ring
// order from its cursor and the drawer takes them in the same order from its
// own; a frame's loaded flag is how the two hand it back and forth.
//
// Example:
//   Frame& slot = ring.load_frame();
//   if (!slot.loaded) { Fill(slot); ring.FinishLoading(); }
//   ...
//   Frame& next = ring.draw_frame();
//   if (next.loaded) { Show(next); ring.FinishDrawing(); }
class FrameRing {
 public:
  // Allocates frame_count frames and codebook_count codebooks, both at least
  // 1, with buffers of the given capacities.
  FrameRing(int frame_count, int codebook_count, base::ssize codebook_capacity,
            base::ssize pointers_capacity, base::ssize palette_capacity);

  // The frame the loader fills next.
  Frame& load_frame() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return frames_.at(base::ToSize(load_index_));
  }
  // Marks the loader's frame loaded and moves the loader on to the next one.
  void FinishLoading();

  // The frame the drawer takes next.
  Frame& draw_frame() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return frames_.at(base::ToSize(draw_index_));
  }
  // Frees the drawer's frame for the loader, drawn or skipped, and moves the
  // drawer on to the next one.
  void FinishDrawing();

  Codebook& codebook(int index) ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return codebooks_.at(base::ToSize(index));
  }
  // The index of the codebook after index, wrapping at the end.
  [[nodiscard]] int next_codebook(int index) const;

  [[nodiscard]] int frame_count() const {
    return static_cast<int>(frames_.size());
  }

 private:
  std::vector<Frame> frames_;
  std::vector<Codebook> codebooks_;
  int load_index_ = 0;
  int draw_index_ = 0;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_FRAME_RING_H_
