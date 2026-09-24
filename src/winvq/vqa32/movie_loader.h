// File: MovieLoader, which reads a VQA movie's frames, codebooks and sound
// from the file into the play buffers ahead of the drawer.

#ifndef CNC_RED_ALERT_WINVQ_VQA32_MOVIE_LOADER_H_
#define CNC_RED_ALERT_WINVQ_VQA32_MOVIE_LOADER_H_

#include <cstdint>
#include <optional>

#include "absl/base/attributes.h"
#include "winvq/vqa32/adpcm_decoders.h"
#include "winvq/vqa32/audio_output.h"
#include "winvq/vqa32/audio_ring.h"
#include "winvq/vqa32/chunk_reader.h"
#include "winvq/vqa32/frame_ring.h"
#include "winvq/vqa32/vqa_format.h"
#include "winvq/vqa32/vqaio.h"

// What MovieLoader::LoadNextFrame() did.
enum class LoadStatus {
  kLoaded,      // A frame was loaded.
  kNoBuffer,    // The next frame buffer still waits to be drawn; try later.
  kAudioFull,   // The audio ring has no room for a sound chunk; try later.
  kEndOfMovie,  // Every frame the header counts is loaded.
  kFailed,      // A read failed, a chunk was malformed, or the file ended
                // early. Loading cannot go on.
};

// Loads a movie a frame at a time, from just after its frame table, into a
// FrameRing, assembling the codebooks and staging the sound on the way.
//
// The codebook arrives in pieces: each frame of a group carries a partial
// codebook for the next group, and the pieces add up to a full codebook just
// before the next group's first frame is read. The first frame carries a full
// one.
//
// Loading is cooperative. A frame whose buffer is still full, or a sound chunk
// that finds the audio ring full, returns without blocking, and the next call
// resumes where this one stopped - inside the sound chunk, if that is where.
//
// Example:
//   MovieLoader loader(io, header, ring, audio);
//   while (loader.LoadNextFrame() == LoadStatus::kLoaded) {}
class MovieLoader {
 public:
  // The sound of the primary track goes into audio, played by output; the
  // alternate track's chunks are skipped. Both are nullptr for a movie played
  // without sound, whose sound chunks are all skipped. The io, header, ring,
  // audio and output must outlive the loader.
  MovieLoader(VqaIo& io ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const VqaHeader& header ABSL_ATTRIBUTE_LIFETIME_BOUND,
              FrameRing& ring ABSL_ATTRIBUTE_LIFETIME_BOUND,
              AudioRing* audio ABSL_ATTRIBUTE_LIFETIME_BOUND,
              AudioOutput* output ABSL_ATTRIBUTE_LIFETIME_BOUND,
              const AudioFormat& format);

  // Loads the next frame into the ring's load_frame().
  LoadStatus LoadNextFrame();

  // Number of the next frame to load.
  [[nodiscard]] int next_frame_number() const { return next_frame_number_; }
  // Index in the ring of the codebook the frames being loaded use.
  [[nodiscard]] int full_codebook() const { return full_codebook_; }

 private:
  // What LoadFramePart() made of a chunk.
  enum class FramePart {
    kNone,            // Not a part of a frame; nothing was read.
    kFailed,          // A part that did not fit its buffer or failed to read.
    kLoaded,          // A codebook or a palette.
    kVectorPointers,  // The vector pointers, which complete a frame.
  };

  // Loads a codebook, palette or vector pointers chunk into the frame being
  // loaded, flagging the frame as the chunk says: key, or carrying a palette.
  FramePart LoadFramePart(const Chunk& chunk);
  // Loads the chunks inside a VQFR or VQFK frame container. Returns false for
  // a bad or unknown chunk, or the end of the file inside it.
  bool LoadFrameContainer(const Chunk& container);

  // The chunk loaders each read one chunk's payload and return false when it
  // does not fit its buffer or the read fails.
  bool LoadFullCodebook(const Chunk& chunk, bool compressed);
  bool LoadPartialCodebook(const Chunk& chunk, bool compressed);
  bool LoadPalette(const Chunk& chunk, bool compressed);
  bool LoadVectorPointers(const Chunk& chunk, bool compressed);
  // Makes the codebook being assembled the full codebook.
  void CompleteCodebook();

  // Loads a primary track sound chunk with the loader for its compression.
  bool LoadSoundChunk(const Chunk& chunk);
  bool LoadSound(const Chunk& chunk);
  bool LoadZapSound(const Chunk& chunk);
  bool LoadAdpcmSound(const Chunk& chunk);
  // Moves the staged sound into the audio ring, holding the audio thread off.
  // Returns false when the ring has no room for it yet.
  bool CopyStagedSound();

  ChunkReader reader_;
  const VqaHeader* header_;
  FrameRing* ring_;
  AudioRing* audio_;
  AudioOutput* output_;
  AudioFormat format_;
  // Carried from one SND2 chunk to the next.
  ImaAdpcmDecoder adpcm_;

  // Index of the codebook the current group's pieces are collected into, to
  // become the next group's codebook.
  int partial_codebook_ = 0;
  // Index of the last complete codebook, used by the frames being loaded.
  int full_codebook_ = 0;
  // Pieces collected into partial_codebook_ so far, and their total size in
  // bytes (compressed or not).
  int partial_count_ = 0;
  int32_t partial_bytes_ = 0;
  // Where compressed pieces collect in partial_codebook_, estimated from the
  // group's first piece.
  int32_t partial_offset_ = 0;
  int next_frame_number_ = 0;
  // The sound chunk the loader stopped inside because the audio ring was
  // full; the next call resumes it instead of reading a new chunk.
  std::optional<Chunk> pending_sound_;
};

#endif  // CNC_RED_ALERT_WINVQ_VQA32_MOVIE_LOADER_H_
