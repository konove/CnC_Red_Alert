#include "ra/passedit.h"

#include <cstring>
#include <string>

void PassEditClass::Set_Focus() {
  if (bClearOnNextSetFocus) {
    // The buffer holds the saved, scrambled password. Throw it away rather
    // than let the player edit characters they never typed.
    bClearOnNextSetFocus = false;
    if (String != nullptr) {
      *String = '\0';
    }
    Length = 0;
  }
  WOLEditClass::Set_Focus();
}

void PassEditClass::Draw_Text(const char* text) {
  const std::string mask(text != nullptr ? std::strlen(text) : 0, '*');
  WOLEditClass::Draw_Text(mask.c_str());
}
