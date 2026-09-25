#include "engine/video/vqa/frame_ring.h"

#include "absl/log/check.h"
#include "engine/base/numeric.h"
#include "engine/base/types.h"

FrameRing::FrameRing(const int frame_count, const int codebook_count,
                     const base::ssize codebook_capacity,
                     const base::ssize pointers_capacity,
                     const base::ssize palette_capacity) {
  CHECK(frame_count >= 1 && codebook_count >= 1);
  codebooks_.reserve(base::ToSize(codebook_count));
  for (int i = 0; i < codebook_count; ++i) {
    codebooks_.emplace_back(codebook_capacity);
  }
  frames_.reserve(base::ToSize(frame_count));
  for (int i = 0; i < frame_count; ++i) {
    frames_.emplace_back(pointers_capacity, palette_capacity);
  }
}

void FrameRing::FinishLoading() {
  load_frame().loaded = true;
  load_index_ = (load_index_ + 1) % frame_count();
}

void FrameRing::FinishDrawing() {
  Frame& frame = draw_frame();
  frame.loaded = false;
  frame.key = false;
  frame.has_palette = false;
  draw_index_ = (draw_index_ + 1) % frame_count();
}

int FrameRing::next_codebook(const int index) const {
  return (index + 1) % static_cast<int>(codebooks_.size());
}
