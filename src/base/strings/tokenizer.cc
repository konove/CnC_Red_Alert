#include "base/strings/tokenizer.h"

#include <string_view>

namespace base {

Tokenizer::Tokenizer(std::string_view text, std::string_view delimiters)
    : remaining_(text), delimiters_(delimiters) {}

std::string_view Tokenizer::Next() { return Next(delimiters_); }

std::string_view Tokenizer::Next(std::string_view delimiters) {
  const auto start = remaining_.find_first_not_of(delimiters);
  if (start == std::string_view::npos) {
    remaining_ = remaining_.substr(remaining_.size());
    return {};
  }
  remaining_.remove_prefix(start);
  const auto end = remaining_.find_first_of(delimiters);
  const std::string_view token = remaining_.substr(0, end);
  // Only the one delimiter that ended the token is consumed, so Remaining()
  // starts right after it.
  remaining_ = end == std::string_view::npos ? remaining_.substr(token.size())
                                             : remaining_.substr(end + 1);
  return token;
}

}  // namespace base
