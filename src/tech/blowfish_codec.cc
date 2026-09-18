#include "tech/blowfish_codec.h"

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "tech/blowfish.h"
#include "tech/byte_sink.h"

void BlowfishCodec::Key(std::span<const std::byte> key) {
  if (!engine_.has_value()) {
    engine_.emplace();
  }
  engine_->Submit_Key(key);
}

bool BlowfishCodec::Process(std::span<const std::byte> in, ByteSink& out) {
  if (!engine_.has_value()) {
    return out.Write(in);
  }
  BlowfishEngine& engine = *engine_;
  bool written = true;
  while (!in.empty()) {
    const base::ssize take = std::min(kBlockSize - count_, std::ssize(in));
    base::CopyBytes(std::span(block_).subspan(base::ToSize(count_)), in, take);
    count_ += take;
    in = in.subspan(base::ToSize(take));
    if (count_ == kBlockSize) {
      if (mode_ == CipherMode::kDecrypt) {
        engine.Decrypt(block_, block_);
      } else {
        engine.Encrypt(block_, block_);
      }
      written = out.Write(block_) && written;
      count_ = 0;
    }
  }
  return written;
}

bool BlowfishCodec::Flush(ByteSink& out) {
  // Blowfish only works on whole blocks, so the tail goes out as it came in.
  const base::ssize tail = count_;
  count_ = 0;
  return tail == 0 || out.Write(std::span(block_).first(base::ToSize(tail)));
}

base::ssize BlowfishCodec::BytesWanted(base::ssize output_needed) const {
  // Unkeyed, bytes pass straight through, so take no more than the reader
  // asked for.
  if (!engine_.has_value()) {
    return output_needed;
  }
  // Keyed, only whole blocks can be deciphered. Finish the block already
  // started; otherwise take every block the reader's request reaches into and
  // not one byte more, so a reader that stops mid-file leaves the source where
  // the next one expects it -- MixArchive::Open takes data_start_ from the
  // file position right after the index. Asking for one block at a time would
  // be just as correct but would drag the whole file through the chain eight
  // bytes per virtual call.
  if (count_ != 0) {
    return kBlockSize - count_;
  }
  const base::ssize needed = std::max<base::ssize>(output_needed, 1);
  return ((needed + kBlockSize - 1) / kBlockSize) * kBlockSize;
}
