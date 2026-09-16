// File: BlockCodec, the block framing shared by the LZO, LCW and LZW
// compressors, written once for both directions.
//
// Replaces Westwood's LZOPipe/LZOStraw, LCWPipe/LCWStraw and LZWPipe/LZWStraw
// (Joe L. Bostic, 1996). Saved games and map packs depend on its bytes.

#ifndef CNC_RED_ALERT_TECH_BLOCK_CODEC_H_
#define CNC_RED_ALERT_TECH_BLOCK_CODEC_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <vector>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "tech/byte_sink.h"

// Whether a block codec link compresses or decompresses what passes it.
enum class CodecMode { kCompress, kDecompress };

// Returns whether a block header read from a compressed stream describes a
// block that fits buffers of `capacity` bytes. Encoders never emit empty
// blocks, so a zero count also marks the stream as corrupt.
constexpr bool BlockHeaderFits(int comp_count, int uncomp_count, int capacity) {
  return comp_count > 0 && comp_count <= capacity && uncomp_count > 0 &&
         uncomp_count <= capacity;
}

// What a block compressor supplies to BlockCodec.
template <class B>
concept BlockBackend = requires(B backend, std::span<const std::byte> input,
                                std::span<std::byte> output, int block_size) {
  // The size both scratch buffers need for blocks of block_size bytes, and
  // the largest count a valid block header may hold.
  { B::Capacity(block_size) } -> std::same_as<int>;
  // Compresses input into output, which holds Capacity() bytes, and returns
  // the compressed size.
  { backend.Compress(input, output) } -> std::same_as<int>;
  // Decompresses input into output and returns the decompressed size, or -1
  // if the input is corrupt or does not fit.
  { backend.Decompress(input, output) } -> std::same_as<int>;
};

// A ByteCodec that compresses a stream in blocks of block_size bytes. Each
// block is written as a little-endian uint16_t compressed size, a uint16_t
// decompressed size and the compressed bytes. Decompression stops for good
// at the first block that cannot be decoded, and a stream that ends inside a
// block is corrupt.
template <BlockBackend Backend>
class BlockCodec {
 public:
  explicit BlockCodec(CodecMode mode, int block_size = 8192)
      : mode_(mode),
        block_size_(block_size),
        capacity_(Backend::Capacity(block_size)),
        input_(base::ToSize(capacity_)),
        output_(base::ToSize(capacity_)) {}

  bool Process(std::span<const std::byte> in, ByteSink& out) {
    bool written = true;
    while (!in.empty() && !corrupt_) {
      const auto wanted = static_cast<int>(BytesWanted(std::ssize(in)));
      const auto take =
          static_cast<int>(std::min<base::ssize>(wanted, std::ssize(in)));
      base::CopyBytes(std::span(input_).subspan(base::ToSize(count_)), in,
                      take);
      count_ += take;
      in = in.subspan(base::ToSize(take));
      if (take < wanted) {
        break;
      }
      if (mode_ == CodecMode::kCompress) {
        written = CompressBlock(out) && written;
      } else if (!have_header_) {
        ReadHeader();
      } else {
        written = DecompressBlock(out) && written;
      }
    }
    return written && !corrupt_;
  }

  bool Flush(ByteSink& out) {
    if (mode_ == CodecMode::kCompress) {
      return count_ == 0 || CompressBlock(out);
    }
    // A partial header or block means the stream was cut short. The block
    // cannot be decoded without its end, so it is dropped.
    if (have_header_ || count_ > 0) {
      have_header_ = false;
      count_ = 0;
      corrupt_ = true;
    }
    return !corrupt_;
  }

  [[nodiscard]] bool ok() const { return !corrupt_; }

  [[nodiscard]] base::ssize BytesWanted(base::ssize /*output_needed*/) const {
    if (mode_ == CodecMode::kCompress) {
      return block_size_ - count_;
    }
    return (have_header_ ? comp_count_ : kHeaderSize) - count_;
  }

 private:
  static constexpr int kHeaderSize = 4;

  bool CompressBlock(ByteSink& out) {
    const int packed = backend_.Compress(
        std::span(input_).first(base::ToSize(count_)), output_);
    const std::array<std::byte, kHeaderSize> header = {
        static_cast<std::byte>(packed), static_cast<std::byte>(packed / 256),
        static_cast<std::byte>(count_), static_cast<std::byte>(count_ / 256)};
    count_ = 0;
    const bool header_written = out.Write(header);
    return out.Write(std::span(output_).first(base::ToSize(packed))) &&
           header_written;
  }

  void ReadHeader() {
    comp_count_ = std::to_integer<int>(input_[0]) +
                  (std::to_integer<int>(input_[1]) * 256);
    uncomp_count_ = std::to_integer<int>(input_[2]) +
                    (std::to_integer<int>(input_[3]) * 256);
    count_ = 0;
    have_header_ = true;
    // A corrupt header must not size reads or writes past the buffers.
    if (!BlockHeaderFits(comp_count_, uncomp_count_, capacity_)) {
      corrupt_ = true;
    }
  }

  bool DecompressBlock(ByteSink& out) {
    const int produced = backend_.Decompress(
        std::span(input_).first(base::ToSize(comp_count_)), output_);
    count_ = 0;
    have_header_ = false;
    if (produced != uncomp_count_) {
      corrupt_ = true;
      return false;
    }
    return out.Write(std::span(output_).first(base::ToSize(uncomp_count_)));
  }

  CodecMode mode_;
  int block_size_;
  int capacity_;
  Backend backend_;
  std::vector<std::byte> input_;   // The block being accumulated.
  std::vector<std::byte> output_;  // The block's transformed bytes.
  int count_ = 0;                  // Bytes of input_ filled so far.
  bool have_header_ = false;       // Decompressing: the header has been read.
  int comp_count_ = 0;    // Decompressing: the block's compressed size.
  int uncomp_count_ = 0;  // Decompressing: its decompressed size.
  bool corrupt_ = false;  // Decompressing: the stream cannot be decoded.
};

#endif  // CNC_RED_ALERT_TECH_BLOCK_CODEC_H_
