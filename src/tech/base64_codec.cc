#include "tech/base64_codec.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <span>

#include "base/buffer.h"
#include "base/numeric.h"
#include "base/types.h"
#include "tech/base64.h"
#include "tech/byte_sink.h"

bool Base64Codec::Process(std::span<const std::byte> in, ByteSink& out) {
  bool written = true;
  while (!in.empty()) {
    const int take =
        std::min(GroupSize() - count_, static_cast<int>(std::ssize(in)));
    base::CopyBytes(std::span(group_).subspan(base::ToSize(count_)), in, take);
    count_ += take;
    in = in.subspan(base::ToSize(take));
    if (count_ == GroupSize()) {
      written = EmitGroup(out) && written;
    }
  }
  return written;
}

bool Base64Codec::Flush(ByteSink& out) { return count_ == 0 || EmitGroup(out); }

base::ssize Base64Codec::BytesWanted(base::ssize /*output_needed*/) const {
  return GroupSize() - count_;
}

bool Base64Codec::EmitGroup(ByteSink& out) {
  std::array<std::byte, 4> result{};
  // Decoding is given room for three bytes, as the old pipe and straw gave
  // it: the decoder writes no more than that for a group of four.
  const int produced =
      mode_ == Base64Mode::kEncode
          ? Base64_Encode(std::span(group_).first(base::ToSize(count_)), result)
          : Base64_Decode(std::span(group_).first(base::ToSize(count_)),
                          std::span(result).first(3));
  count_ = 0;
  return produced == 0 ||
         out.Write(std::span(result).first(base::ToSize(produced)));
}
