/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Keyboard Library *
 *                                                                                             *
 *                    File Name : KEYBOARD.H *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow *
 *                                                                                             *
 *                   Start Date : 10/16/95 *
 *                                                                                             *
 *                  Last Update : October 16, 1995 [PWG] *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions: *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
 *- - - - - - - */

#ifndef CNC_RED_ALERT_SDLLIB_KEYBOARD_H_
#define CNC_RED_ALERT_SDLLIB_KEYBOARD_H_

#include <cstdint>

union SDL_Event;

// Modifier and state bits combined with a key number. They are flags, not an
// enumeration, so they are unsigned bit masks that mix with KeyNumType freely.
inline constexpr uint32_t WWKEY_SHIFT_BIT = 0x100;
inline constexpr uint32_t WWKEY_CTRL_BIT = 0x200;
inline constexpr uint32_t WWKEY_ALT_BIT = 0x400;
inline constexpr uint32_t WWKEY_RLS_BIT = 0x800;
inline constexpr uint32_t WWKEY_VK_BIT = 0x1000;
inline constexpr uint32_t WWKEY_DBL_BIT = 0x2000;
inline constexpr uint32_t WWKEY_BTN_BIT = 0x8000;

class WWKeyboardClass {
 public:
  /*===================================================================*/
  /* Define the base constructor and destructors for the class */
  /*===================================================================*/
  WWKeyboardClass();

  /*===================================================================*/
  /* Define the functions which work with the Keyboard Class
   */
  /*===================================================================*/
  // Returns the key number at the head of the buffer without removing it, or 0
  // when no key is pending. Also pumps the SDL event loop, so callers that only
  // need that side effect may discard the result.
  int Check();
  int Get();          // gets a meta key from the keybuffer
  bool Put(int key);  // dumps a key into the keybuffer
  bool Put_Key_Message(
      unsigned vk_key,
      bool release = false);  // handles keyboard related message
                              //   and mouse clicks and dbl clicks
  static int To_ASCII(int num);  // converts keynum to ascii value
  void Clear();               // clears all keys from keybuffer
  static bool Down(int key);  // tests to see if a key is down

  /*===================================================================*/
  /* Define the main hook for the message processing loop.
   */
  /*===================================================================*/
  // void Message_Handler(HWND hwnd, UINT message, UINT wParam, long lParam);

  /*===================================================================*/
  /* Define public routines which can be used on keys in general.
   */
  /*===================================================================*/
  static bool Is_Mouse_Key(int key);

  bool Event_Handler(SDL_Event* event);

  /*===================================================================*/
  /* Define the public access variables which are used with the */
  /*   Keyboard Class.
   */
  /*===================================================================*/
  int MouseQX = 0;
  int MouseQY = 0;

 private:
  /*===================================================================*/
  /* Define the private access functions which are used by keyboard
   */
  /*===================================================================*/
  int Buff_Get();

  /*===================================================================*/
  /* Define the private access variables which are used with the
   */
  /*   Keyboard Class.
   */
  /*===================================================================*/
  uint16_t Buffer[256]{};      // buffer which holds actual keypresses
  int Head = 0;                // the head position in keyboard buffer
  int Tail = 0;                // the tail position in keyboard buffer
};

extern WWKeyboardClass* ActiveKeyboard;

// Both peek at the pending key number. Check_Key deliberately does not mirror
// Get_Key's ASCII translation: its callers test whether any key is waiting, and
// To_ASCII reports 0 for key releases and every non-alphanumeric key.
inline int Check_Key() { return ActiveKeyboard->Check(); }
inline int Check_Key_Num() { return ActiveKeyboard->Check(); }
inline int Get_Key() {
  return WWKeyboardClass::To_ASCII(ActiveKeyboard->Get());
}
inline int Get_Key_Num() { return ActiveKeyboard->Get(); }
inline bool Key_Down(int key) { return WWKeyboardClass::Down(key); }
inline void Clear_KeyBuffer() { ActiveKeyboard->Clear(); }
inline int KN_To_KA(int key) { return WWKeyboardClass::To_ASCII(key); }
inline int KN_To_VK(int key) { return key; }

// these are (mostly) SDL_SCANCODE_x values
#define VK_NONE 0
#define VK_LBUTTON 1
#define VK_RBUTTON 2
#define VK_MBUTTON 3

#define VK_A 4
#define VK_B 5
#define VK_C 6
#define VK_D 7
#define VK_E 8
#define VK_F 9
#define VK_G 10
#define VK_H 11
#define VK_I 12
#define VK_J 13
#define VK_K 14
#define VK_L 15
#define VK_M 16
#define VK_N 17
#define VK_O 18
#define VK_P 19
#define VK_Q 20
#define VK_R 21
#define VK_S 22
#define VK_T 23
#define VK_U 24
#define VK_V 25
#define VK_W 26
#define VK_X 27
#define VK_Y 28
#define VK_Z 29

#define VK_1 30
#define VK_2 31
#define VK_3 32
#define VK_4 33
#define VK_5 34
#define VK_6 35
#define VK_7 36
#define VK_8 37
#define VK_9 38
#define VK_0 39

#define VK_RETURN 40
#define VK_ESCAPE 41
#define VK_BACK 42
#define VK_TAB 43
#define VK_SPACE 44

#define VK_OEM_MINUS 45
#define VK_OEM_PLUS 46  // =
#define VK_OEM_4 47     // [
#define VK_OEM_6 48     // ]
#define VK_OEM_5 49     // backslash
#define VK_OEM_1 51     // ;
#define VK_OEM_7 52     // '
#define VK_OEM_3 53     // `
#define VK_OEM_COMMA 54
#define VK_OEM_PERIOD 55
#define VK_OEM_2 56

#define VK_CAPITAL 57

#define VK_F1 58
#define VK_F2 59
#define VK_F3 60
#define VK_F4 61
#define VK_F5 62
#define VK_F6 63
#define VK_F7 64
#define VK_F8 65
#define VK_F9 66
#define VK_F10 67
#define VK_F11 68
#define VK_F12 69

#define VK_SNAPSHOT 70
#define VK_SCROLL 71
#define VK_PAUSE 72
#define VK_INSERT 73

#define VK_HOME 74
#define VK_PRIOR 75
#define VK_DELETE 76
#define VK_END 77
#define VK_NEXT 78
#define VK_RIGHT 79
#define VK_LEFT 80
#define VK_DOWN 81
#define VK_UP 82

#define VK_NUMLOCK 83

#define VK_DIVIDE 84
#define VK_MULTIPLY 85
#define VK_SUBTRACT 86
#define VK_ADD 87
#define VK_NUMPAD1 89
#define VK_NUMPAD2 90
#define VK_NUMPAD3 91
#define VK_NUMPAD4 92
#define VK_NUMPAD5 93
#define VK_NUMPAD6 94
#define VK_NUMPAD7 95
#define VK_NUMPAD8 96
#define VK_NUMPAD9 97
#define VK_NUMPAD0 98
#define VK_DECIMAL 99

#define VK_CLEAR VK_NUMPAD5

#define VK_SELECT 119
#define VK_CONTROL 224
#define VK_SHIFT 225
#define VK_MENU 226

#define VK_UPLEFT VK_HOME
#define VK_UPRIGHT VK_PRIOR
#define VK_DOWNLEFT VK_END
#define VK_DOWNRIGHT VK_NEXT
#define VK_ALT VK_MENU

// A key code is a bit pattern (the code, modifier bits and the KN_BUTTON
// composite) that the keyboard buffer stores as an integer, so it stays
// unscoped.
// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
typedef enum KeyASCIIType {

  KA_NONE = 0,
  KA_MORE = 1,
  KA_SETBKGDCOL = 2,
  KA_SETFORECOL = 6,
  KA_FORMFEED = 12,
  KA_SPCTAB = 20,
  KA_SETX = 25,
  KA_SETY = 26,

  KA_SPACE = 32,       /*   */
  KA_EXCLAMATION = 33, /* ! */
  KA_DQUOTE = 34,      /* " */
  KA_POUND = 35,       /* # */
  KA_DOLLAR = 36,      /* $ */
  KA_PERCENT = 37,     /* % */
  KA_AMPER = 38,       /* & */
  KA_SQUOTE = 39,      /* ' */
  KA_LPAREN = 40,      /* ( */
  KA_RPAREN = 41,      /* ) */
  KA_ASTERISK = 42,    /* * */
  KA_PLUS = 43,        /* + */
  KA_COMMA = 44,       /* , */
  KA_MINUS = 45,       /* - */
  KA_PERIOD = 46,      /* . */
  KA_SLASH = 47,       /* / */

  KA_0 = 48,
  KA_1 = 49,
  KA_2 = 50,
  KA_3 = 51,
  KA_4 = 52,
  KA_5 = 53,
  KA_6 = 54,
  KA_7 = 55,
  KA_8 = 56,
  KA_9 = 57,
  KA_COLON = 58,        /* : */
  KA_SEMICOLON = 59,    /* ; */
  KA_LESS_THAN = 60,    /* < */
  KA_EQUAL = 61,        /* = */
  KA_GREATER_THAN = 62, /* > */
  KA_QUESTION = 63,     /* ? */

  KA_AT = 64, /* @ */
  KA_A = 65,  /* A */
  KA_B = 66,  /* B */
  KA_C = 67,  /* C */
  KA_D = 68,  /* D */
  KA_E = 69,  /* E */
  KA_F = 70,  /* F */
  KA_G = 71,  /* G */
  KA_H = 72,  /* H */
  // Key names spell their key's label, so I/1, O/0 and l/1 look alike by
  // design. NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_I = 73, /* I */
  KA_J = 74, /* J */
  KA_K = 75, /* K */
  KA_L = 76, /* L */
  KA_M = 77, /* M */
  KA_N = 78, /* N */
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_O = 79, /* O */

  KA_P = 80,         /* P */
  KA_Q = 81,         /* Q */
  KA_R = 82,         /* R */
  KA_S = 83,         /* S */
  KA_T = 84,         /* T */
  KA_U = 85,         /* U */
  KA_V = 86,         /* V */
  KA_W = 87,         /* W */
  KA_X = 88,         /* X */
  KA_Y = 89,         /* Y */
  KA_Z = 90,         /* Z */
  KA_LBRACKET = 91,  /* [ */
  KA_BACKSLASH = 92, /* \ */
  KA_RBRACKET = 93,  /* ] */
  KA_CARROT = 94,    /* ^ */
  KA_UNDERLINE = 95, /* _ */

  KA_GRAVE = 96, /* ` */
  KA_a = 97,     /* a */
  KA_b = 98,     /* b */
  KA_c = 99,     /* c */
  KA_d = 100,    /* d */
  KA_e = 101,    /* e */
  KA_f = 102,    /* f */
  KA_g = 103,    /* g */
  KA_h = 104,    /* h */
  KA_i = 105,    /* i */
  KA_j = 106,    /* j */
  KA_k = 107,    /* k */
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KA_l = 108, /* l */
  KA_m = 109, /* m */
  KA_n = 110, /* n */
  KA_o = 111, /* o */

  KA_p = 112,      /* p */
  KA_q = 113,      /* q */
  KA_r = 114,      /* r */
  KA_s = 115,      /* s */
  KA_t = 116,      /* t */
  KA_u = 117,      /* u */
  KA_v = 118,      /* v */
  KA_w = 119,      /* w */
  KA_x = 120,      /* x */
  KA_y = 121,      /* y */
  KA_z = 122,      /* z */
  KA_LBRACE = 123, /* { */
  KA_BAR = 124,    /* | */
  KA_RBRACE = 125, /* ] */
  KA_TILDA = 126,  /* ~ */

  KA_ESC = '\x1b',
  KA_RETURN = '\r',
  KA_BACKSPACE = '\b',
  KA_TAB = '\t',

  KA_SHIFT_BIT = WWKEY_SHIFT_BIT,
  KA_CTRL_BIT = WWKEY_CTRL_BIT,
  KA_ALT_BIT = WWKEY_ALT_BIT,
  KA_RLSE_BIT = WWKEY_RLS_BIT,
} KeyASCIIType;

// NOLINTNEXTLINE(cppcoreguidelines-use-enum-class)
typedef enum KeyNumType {
  KN_NONE = 0,

  KN_0 = VK_0,
  KN_1 = VK_1,
  KN_2 = VK_2,
  KN_3 = VK_3,
  KN_4 = VK_4,
  KN_5 = VK_5,
  KN_6 = VK_6,
  KN_7 = VK_7,
  KN_8 = VK_8,
  KN_9 = VK_9,
  KN_A = VK_A,
  KN_B = VK_B,
  KN_BACKSLASH = VK_OEM_5,
  KN_BACKSPACE = VK_BACK,
  KN_C = VK_C,
  KN_CAPSLOCK = VK_CAPITAL,
  KN_CENTER = VK_CLEAR,
  KN_COMMA = VK_OEM_COMMA,
  KN_D = VK_D,
  KN_DELETE = VK_DELETE,
  KN_DOWN = VK_DOWN,
  KN_DOWNLEFT = VK_END,
  KN_DOWNRIGHT = VK_NEXT,
  KN_E = VK_E,
  KN_END = VK_END,
  KN_EQUAL = VK_OEM_PLUS,
  KN_ESC = VK_ESCAPE,
  KN_E_DELETE = VK_DELETE,
  KN_E_DOWN = VK_NUMPAD2,
  KN_E_END = VK_NUMPAD1,
  KN_E_HOME = VK_NUMPAD7,
  KN_E_INSERT = VK_INSERT,
  KN_E_LEFT = VK_NUMPAD4,
  KN_E_PGDN = VK_NUMPAD3,
  KN_E_PGUP = VK_NUMPAD9,
  KN_E_RIGHT = VK_NUMPAD6,
  KN_E_UP = VK_NUMPAD8,
  KN_F = VK_F,
  KN_F1 = VK_F1,
  KN_F10 = VK_F10,
  KN_F11 = VK_F11,
  KN_F12 = VK_F12,
  KN_F2 = VK_F2,
  KN_F3 = VK_F3,
  KN_F4 = VK_F4,
  KN_F5 = VK_F5,
  KN_F6 = VK_F6,
  KN_F7 = VK_F7,
  KN_F8 = VK_F8,
  KN_F9 = VK_F9,
  KN_G = VK_G,
  KN_GRAVE = VK_OEM_3,
  KN_H = VK_H,
  KN_HOME = VK_HOME,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_I = VK_I,
  KN_INSERT = VK_INSERT,
  KN_J = VK_J,
  KN_K = VK_K,
  KN_KEYPAD_ASTERISK = VK_MULTIPLY,
  KN_KEYPAD_MINUS = VK_SUBTRACT,
  KN_KEYPAD_PLUS = VK_ADD,
  KN_KEYPAD_RETURN = VK_RETURN,
  KN_KEYPAD_SLASH = VK_DIVIDE,
  KN_L = VK_L,
  KN_LALT = VK_MENU,
  KN_LBRACKET = VK_OEM_4,
  KN_LCTRL = VK_CONTROL,
  KN_LEFT = VK_LEFT,
  KN_LMOUSE = VK_LBUTTON,
  KN_LSHIFT = VK_SHIFT,
  KN_M = VK_M,
  KN_MINUS = VK_OEM_MINUS,
  KN_N = VK_N,
  KN_NUMLOCK = VK_NUMLOCK,
  // NOLINTNEXTLINE(misc-confusable-identifiers)
  KN_O = VK_O,
  KN_P = VK_P,
  KN_PAUSE = VK_PAUSE,
  KN_PERIOD = VK_OEM_PERIOD,
  KN_PGDN = VK_NEXT,
  KN_PGUP = VK_PRIOR,
  KN_PRNTSCRN = VK_SNAPSHOT,
  KN_Q = VK_Q,
  KN_R = VK_R,
  KN_RALT = VK_MENU,
  KN_RBRACKET = VK_OEM_6,
  KN_RCTRL = VK_CONTROL,
  KN_RETURN = VK_RETURN,
  KN_RIGHT = VK_RIGHT,
  KN_RMOUSE = VK_RBUTTON,
  KN_RSHIFT = VK_SHIFT,
  KN_S = VK_S,
  KN_SCROLLLOCK = VK_SCROLL,
  KN_SEMICOLON = VK_OEM_1,
  KN_SLASH = VK_OEM_2,
  KN_SPACE = VK_SPACE,
  KN_SQUOTE = VK_OEM_7,
  KN_T = VK_T,
  KN_TAB = VK_TAB,
  KN_U = VK_U,
  KN_UP = VK_UP,
  KN_UPLEFT = VK_HOME,
  KN_UPRIGHT = VK_PRIOR,
  KN_V = VK_V,
  KN_W = VK_W,
  KN_X = VK_X,
  KN_Y = VK_Y,
  KN_Z = VK_Z,

  KN_SHIFT_BIT = WWKEY_SHIFT_BIT,
  KN_CTRL_BIT = WWKEY_CTRL_BIT,
  KN_ALT_BIT = WWKEY_ALT_BIT,
  KN_RLSE_BIT = WWKEY_RLS_BIT,
  KN_BUTTON = WWKEY_BTN_BIT,
} KeyNumType;

// Returns the KeyNumType that GadgetClass::Input() reports when the gadget with
// the given ID is triggered. The KN_BUTTON bit distinguishes a gadget event
// from a real keypress, letting both share one switch on the input value.
//
// Gadget IDs are dialog-local — the same numeric ID means a different button in
// each dialog — so they are passed in rather than enumerated here.
//
// Example:
//   switch (input) {
//     case KN_ESC:
//     case ButtonKey(BUTTON_CANCEL):
//       ...
//   }
constexpr KeyNumType ButtonKey(const int id) {
  // A gadget ID is not a key code, so the value never names an enumerator.
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumType>(static_cast<uint32_t>(id) | KN_BUTTON);
}

// A key number is a key code in the low bits with the KN_*_BIT modifier and
// release bits above it, so a combined or masked value rarely names an
// enumerator. These operators define that representation for KeyNumType in
// place of the games' generic enum operators, and are the only place the
// analyzer's named-enumerator model of the type is set aside.
constexpr KeyNumType operator|(const KeyNumType a,
                               const KeyNumType b) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumType>(static_cast<uint32_t>(a) |
                                 static_cast<uint32_t>(b));
}
inline KeyNumType operator&(const KeyNumType a, const KeyNumType b) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumType>(static_cast<uint32_t>(a) &
                                 static_cast<uint32_t>(b));
}
inline KeyNumType operator~(const KeyNumType a) noexcept {
  // NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)
  return static_cast<KeyNumType>(~static_cast<uint32_t>(a));
}

#endif  // CNC_RED_ALERT_SDLLIB_KEYBOARD_H_
