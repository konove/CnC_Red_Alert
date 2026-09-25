// File: AudioRing, the buffer a VQA movie's sound goes through between the
// loader and the audio thread, and AudioFormat, the format of that sound.

#ifndef CNC_RED_ALERT_ENGINE_VIDEO_VQA_AUDIO_RING_H_
#define CNC_RED_ALERT_ENGINE_VIDEO_VQA_AUDIO_RING_H_

#include <atomic>
#include <cstdint>
#include <span>
#include <vector>

#include "absl/base/attributes.h"
#include "engine/video/vqa/vqa_format.h"

// AudioFormat: the sample format of a movie's sound track.
struct AudioFormat {
  int sample_rate = 0;
  int channels = 0;
  int bits_per_sample = 0;  // 8 or 16

  // The format of a movie's sound track. Version 1 movies only had 22050 Hz
  // 8-bit mono sound, and their headers do not say.
  static AudioFormat FromHeader(const VqaHeader& header);

  [[nodiscard]] int bytes_per_second() const {
    return sample_rate * channels * (bits_per_sample / 8);
  }
};

// A ring of equal blocks of sound that the loader fills and the audio thread
// plays, plus a staging buffer the loader decodes each sound chunk into first.
//
// The loader stages a chunk, then CopyStaged() moves it into the ring and
// marks the blocks it completed as loaded. The audio thread plays the block at
// play_block() and, through Advance(), frees it and moves on once the next
// block is loaded; if it is not, the block plays again. The unplayed blocks
// are always one run starting at play_block().
//
// The ring itself does no locking. While the sound plays, the loader side
// must hold the audio thread off (the device lock) around CopyStaged().
class AudioRing {
 public:
  // A ring of block_count blocks of block_bytes each, and staging_bytes of
  // staging. Both counts must be positive.
  AudioRing(int block_count, int block_bytes, int staging_bytes);

  ~AudioRing() = default;
  AudioRing(const AudioRing&) = delete;
  AudioRing& operator=(const AudioRing&) = delete;
  AudioRing(AudioRing&&) = delete;
  AudioRing& operator=(AudioRing&&) = delete;

  // Loader side.

  // Where the loader decodes a chunk's sound before Stage()-ing it.
  std::span<unsigned char> staging() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return staging_;
  }
  [[nodiscard]] int staging_capacity() const {
    return static_cast<int>(staging_.size());
  }
  // Declares the first bytes of staging to hold sound for CopyStaged().
  void Stage(int bytes) { staged_bytes_ = bytes; }
  [[nodiscard]] int staged_bytes() const { return staged_bytes_; }

  // Moves the staged sound into the ring at write_offset(), wrapping at its
  // end, and marks the blocks it completes as loaded. Returns false, with the
  // sound still staged, when that would overwrite a block not played yet.
  // Nothing staged is nothing to do, and returns true.
  bool CopyStaged();

  // The whole ring, which a movie's first sound chunk may be decoded straight
  // into when it is larger than staging.
  std::span<unsigned char> ring() ABSL_ATTRIBUTE_LIFETIME_BOUND {
    return ring_;
  }
  // Accounts for bytes of sound written at the start of the ring: moves the
  // write position past them and marks the whole blocks they fill. The next
  // copy completes a partial last block.
  void CommitPreload(int bytes);

  [[nodiscard]] int write_offset() const { return write_offset_; }
  [[nodiscard]] bool block_loaded(int block) const;

  // Tells the ring the loader has read the whole movie. From then on a replayed
  // block counts as played, so a movie paced by its sound still reaches its
  // last frames after the sound has run out.
  void MarkMovieLoaded() {
    movie_loaded_.store(true, std::memory_order_relaxed);
  }

  // Audio thread side.

  // The block to play now. The span is into the ring, which clang's
  // lifetimebound-violation check cannot see through subspan().
  // NOLINTBEGIN(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  [[nodiscard]] std::span<const unsigned char> play_block_bytes() const
      ABSL_ATTRIBUTE_LIFETIME_BOUND;
  // NOLINTEND(clang-diagnostic-lifetime-safety-lifetimebound-violation)
  // Frees the block just played and moves on to the next one if the loader
  // has filled it. Otherwise stays, to play the block again - the loader has
  // fallen behind, and a repeat is less jarring than a gap - and records an
  // underrun.
  void Advance();

  [[nodiscard]] int play_block() const { return play_block_; }
  // Blocks played since ResetBlocksPlayed(); the audio clock is made of it.
  [[nodiscard]] int blocks_played() const { return blocks_played_; }
  void ResetBlocksPlayed() { blocks_played_ = 0; }

  // Whether the sound has ever run dry. Readable from any thread.
  [[nodiscard]] bool underran() const {
    return underrun_.load(std::memory_order_relaxed);
  }

  [[nodiscard]] int block_count() const { return block_count_; }
  [[nodiscard]] int block_bytes() const { return block_bytes_; }
  [[nodiscard]] int capacity() const { return block_count_ * block_bytes_; }

 private:
  int block_count_;
  int block_bytes_;
  std::vector<unsigned char> ring_;
  std::vector<unsigned char> staging_;
  // One entry per block: true while it holds sound not played yet.
  std::vector<bool> loaded_;
  int write_offset_ = 0;
  int staged_bytes_ = 0;
  int play_block_ = 0;
  int blocks_played_ = 0;
  // Written on the main thread, read on the audio thread; they carry no other
  // data, so relaxed ordering is enough.
  std::atomic<bool> movie_loaded_ = false;
  std::atomic<bool> underrun_ = false;
};

#endif  // CNC_RED_ALERT_ENGINE_VIDEO_VQA_AUDIO_RING_H_
