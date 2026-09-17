#include "ra/passedit.h"

#include <cstring>
#include <span>
#include <string>
#include <string_view>

#include "base/array.h"
#include "ra/woledit.h"

void PassEditClass::Set_Focus() {
  if (bClearOnNextSetFocus) {
    // The buffer holds the saved, scrambled password. Throw it away rather
    // than let the player edit characters they never typed.
    bClearOnNextSetFocus = false;
    if (!String.empty()) {
      base::At(String, 0) = '\0';
    }
    Length = 0;
  }
  WOLEditClass::Set_Focus();
}

void PassEditClass::Draw_Text(const char* text) {
  const std::string mask(text != nullptr ? std::string_view(text).size() : 0,
                         '*');
  WOLEditClass::Draw_Text(mask.c_str());
}
