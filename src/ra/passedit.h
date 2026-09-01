// File: The password box on the Westwood Online login screen.

#ifndef CNC_RED_ALERT_RA_PASSEDIT_H_
#define CNC_RED_ALERT_RA_PASSEDIT_H_

#include "ra/edit.h"
#include "ra/woledit.h"

// An edit box that shows asterisks instead of what was typed.
//
// The login screen fills this in from the saved password in the registry, and
// what it puts there is the already-scrambled form, not something the player
// typed. Editing that in place would send nonsense to the server, so the box
// starts in a state where the first click empties it: set
// bClearOnNextSetFocus after loading a saved password, and read it afterwards
// to find out whether the buffer still holds the saved form (true) or a
// freshly typed password (false).
//
// Example:
//   PassEditClass pass(id, buffer, sizeof(buffer), flags, x, y, w, -1);
//   pass.bClearOnNextSetFocus = LoadSavedPassword(buffer);
class PassEditClass : public WOLEditClass {
 public:
  PassEditClass(int id, char* text, int max_len, TextPrintType flags, int x,
                int y, int w, int h, EditStyle style)
      : WOLEditClass(id, text, max_len, flags, x, y, w, h, style) {}

  // True while the buffer holds a saved password rather than a typed one.
  // Cleared by the first Set_Focus(), which also empties the buffer.
  bool bClearOnNextSetFocus = false;

  void Set_Focus() override;

 protected:
  void Draw_Text(const char* text) override;
};

#endif  // CNC_RED_ALERT_RA_PASSEDIT_H_
