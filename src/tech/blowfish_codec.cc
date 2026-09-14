#include "tech/blowfish_codec.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>

#include "base/numeric.h"
#include "base/types.h"
#include "tech/blowfish.h"
#include "tech/byte_sink.h"

void BlowfishCodec::Key(const void* key, int length) {
  if (!engine_.has_value()) {
    engine_.emplace();
  }
  engine_->Submit_Key(key, length);
}

bool BlowfishCodec::Process(std::span<const std::byte> in, ByteSink& out) {
  if (!engine_.has_value()) {
    return out.Write(in);
  }
  BlowfishEngine& engine = *engine_;
  bool written = true;
  while (!in.empty()) {
    const base::ssize take = std::min(kBlockSize - count_, std::ssize(in));
    std::memcpy(block_.data() + count_, in.data(), base::ToSize(take));
    count_ += take;
    in = in.subspan(base::ToSize(take));
    if (count_ == kBlockSize) {
      if (mode_ == CipherMode::kDecrypt) {
        engine.Decrypt(block_.data(), kBlockSize, block_.data());
      } else {
        engine.Encrypt(block_.data(), kBlockSize, block_.data());
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
  return engine_.has_value() ? kBlockSize - count_ : output_needed;
}
