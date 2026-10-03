// File: Splitting text into tokens with strtok's rules but without its hidden
// cursor or its writes into the text.
//
// The INI, phone-book and network-message parsers read fields one at a time
// and hand each to a consumer (`From_Name`, `ParseIntegerOr`). Tokenizer keeps
// strtok's splitting rules -- runs of delimiters are skipped and a token is
// never empty -- but returns views into the text instead of terminating each
// token in place, and keeps the cursor in the object, so a consumer may itself
// tokenize and two parsers may be live at once.
//
// Example:
//   const std::string entry = ini.GetString("INFANTRY", "000");
//   base::Tokenizer tokens(entry);
//   const HousesType house = HouseTypeClass::From_Name(tokens.Next());
//   const InfantryType type = InfantryTypeClass::From_Name(tokens.Next());
//   const int strength = base::ParseIntegerOr<int>(tokens.Next(), 0);

#ifndef CNC_RED_ALERT_BASE_STRINGS_TOKENIZER_H_
#define CNC_RED_ALERT_BASE_STRINGS_TOKENIZER_H_

#include <string_view>

#include "absl/base/attributes.h"

namespace base {

class Tokenizer {
 public:
  // Splits `text` on any character in `delimiters`, by default the comma that
  // separates INI fields. The characters of both must outlive the tokenizer
  // and the tokens it returns.
  explicit Tokenizer(
      std::string_view text ABSL_ATTRIBUTE_LIFETIME_BOUND,
      std::string_view delimiters ABSL_ATTRIBUTE_LIFETIME_BOUND = ",");

  // Returns the next token, or an empty view once none remain. A token is
  // never empty, and it is not NUL-terminated. Once an empty view has been
  // returned, every later call returns one.
  std::string_view Next();

  // Same as Next(), but splits on `delimiters` for this call only.
  std::string_view Next(std::string_view delimiters);

  // Returns the unread text following the delimiter that ended the last
  // token, for fields whose length is given by the token before them.
  // Returns "" once the text is exhausted.
  [[nodiscard]] std::string_view Remaining() const { return remaining_; }

 private:
  std::string_view remaining_;
  std::string_view delimiters_;
};

}  // namespace base

#endif  // CNC_RED_ALERT_BASE_STRINGS_TOKENIZER_H_
