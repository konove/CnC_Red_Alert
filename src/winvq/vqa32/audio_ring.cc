#include "winvq/vqa32/audio_ring.h"

#include <algorithm>
#include <atomic>
#include <span>

#include "absl/log/check.h"
#include "base/buffer.h"
#include "base/numeric.h"
#include "winvq/vqa32/vqa_format.h"

AudioFormat AudioFormat::FromHeader(const VqaHeader& header) {
  if (header.version < kVqaVersion2) {
    return {.sample_rate = 22050, .channels = 1, .bits_per_sample = 8};
  }
  return {.sample_rate = header.sample_rate,
          .channels = header.channels,
          .bits_per_sample = header.bits_per_sample};
}

AudioRing::AudioRing(const int block_count, const int block_bytes,
                     const int staging_bytes)
    : block_count_(block_count),
      block_bytes_(block_bytes),
      ring_(base::ToSize(block_count * block_bytes)),
      staging_(base::ToSize(staging_bytes)),
      loaded_(base::ToSize(block_count)) {
  CHECK(block_count > 0 && block_bytes > 0 && staging_bytes >= 0);
}

bool AudioRing::CopyStaged() {
  if (staged_bytes_ == 0) {
    return true;
  }

  // The blocks the write starts and ends in. end_block is partly filled at
  // most, so it is not marked loaded below; the next copy completes it.
  const int start_block = write_offset_ / block_bytes_;
  const int end_block =
      (write_offset_ + staged_bytes_) / block_bytes_ % block_count_;

  // The unplayed blocks are one run starting at play_block_, so if the last
  // block the write reaches is free, so is every block before it.
  if (block_loaded(end_block)) {
    return false;
  }

  // Fill towards the end of the ring, and continue at its start with what
  // does not fit.
  const auto staged = std::as_bytes(std::span(staging_));
  const int tail_bytes = std::min(staged_bytes_, capacity() - write_offset_);
  const int head_bytes = staged_bytes_ - tail_bytes;
  base::CopyBytes(std::as_writable_bytes(
                      std::span(ring_).subspan(base::ToSize(write_offset_))),
                  staged, tail_bytes);
  base::CopyBytes(std::as_writable_bytes(std::span(ring_)),
                  staged.subspan(base::ToSize(tail_bytes)), head_bytes);

  write_offset_ = (write_offset_ + staged_bytes_) % capacity();
  staged_bytes_ = 0;

  for (int block = start_block; block != end_block;
       block = (block + 1) % block_count_) {
    loaded_.at(base::ToSize(block)) = true;
  }
  return true;
}

void AudioRing::CommitPreload(const int bytes) {
  write_offset_ = (write_offset_ + bytes) % capacity();
  for (int block = 0; block < bytes / block_bytes_; ++block) {
    loaded_.at(base::ToSize(block)) = true;
  }
}

bool AudioRing::block_loaded(const int block) const {
  return loaded_.at(base::ToSize(block));
}

std::span<const unsigned char> AudioRing::play_block_bytes() const {
  return std::span(ring_).subspan(base::ToSize(play_block_ * block_bytes_),
                                  base::ToSize(block_bytes_));
}

void AudioRing::Advance() {
  const int next_block = (play_block_ + 1) % block_count_;
  if (block_loaded(next_block)) {
    loaded_.at(base::ToSize(play_block_)) = false;
    play_block_ = next_block;
    blocks_played_++;
    return;
  }
  // A repeat advances the clock only once the whole movie is loaded. Until
  // then the frames wait for the loader to catch up; after, the sound has
  // simply run out, and the last frames must still come due.
  if (movie_loaded_.load(std::memory_order_relaxed)) {
    blocks_played_++;
  }
  underrun_.store(true, std::memory_order_relaxed);
}
