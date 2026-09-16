// File: Splitting a writable C string into tokens without strtok's hidden
// cursor.
//
// The INI, phone-book and network-message parsers read fields one at a time
// and hand each to a `const char*` consumer (`From_Name`, `ParseInteger`).
// Tokenizer keeps strtok's splitting rules -- runs of delimiters are skipped,
// each token is terminated in place, and nullptr marks the end -- but keeps
// the cursor in the object, so a consumer may itself tokenize and two parsers
// may be live at once.
//
// Example:
//   char entry[] = "GDI,E1,256,1234";
//   port::Tokenizer tokens(entry, ",");
//   const HousesType house = HouseTypeClass::From_Name(tokens.Next());
//   const InfantryType type = InfantryTypeClass::From_Name(tokens.Next());
//   const int strength = tech::ParseInteger<int>(tokens.Next()).value_or(0);

#ifndef CNC_RED_ALERT_PORT_TOKENIZER_H_
#define CNC_RED_ALERT_PORT_TOKENIZER_H_

#include <span>

#include "absl/base/attributes.h"

namespace port {

class Tokenizer {
 public:
  // Splits `text` on any character in `delimiters`. Both must outlive the
  // tokenizer; `text` is modified in place as tokens are cut from it.
  Tokenizer(char* text ABSL_ATTRIBUTE_LIFETIME_BOUND,
            const char* delimiters ABSL_ATTRIBUTE_LIFETIME_BOUND);

  // Returns the next token, or nullptr once none remain. A token is never
  // empty. Once nullptr has been returned, every later call returns nullptr.
  char* Next();

  // Same as Next(), but splits on `delimiters` for this call only.
  char* Next(const char* delimiters);

  // Returns the unread text following the delimiter that ended the last
  // token, for fields whose length is given by the token before them.
  // Returns "" once the text is exhausted.
  [[nodiscard]] char* Remaining() const { return cursor_.data(); }

 private:
  std::span<char> cursor_;  // Includes the terminating NUL, even once exhausted.
  const char* delimiters_;
};

}  // namespace port

#endif  // CNC_RED_ALERT_PORT_TOKENIZER_H_
