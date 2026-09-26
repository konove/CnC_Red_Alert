# Simplify the Key Enums

## Findings

Measured on the tree at f52087e1.

`engine/window/keyboard.h` names keys three times over:

- about 100 `VK_*` macros,
- `enum KeyAscii` (111 `KA_*` names),
- `enum KeyNumber` (113 `KN_*` names).

It also spells each modifier bit three ways: `kKeyAltBit`, `KN_ALT_BIT` and `KA_ALT_BIT`.

- **`KeyAscii` is a character with names.**
  - About 100 of its names appear only as `case` labels in four switches: `td/edit.cc` and
    `ra/edit.cc` `Handle_Key()`, and `td/msglist.cc` and `ra/msglist.cc` `Input()`. Every one of
    those labels falls through to `default:`, so the lists do nothing.
  - That includes the text printer's command codes (`KA_MORE`, `KA_SETX`, ...). The printer does not
    use these names.
  - Real uses number about 20: `KA_RETURN`, `KA_BACKSPACE`, `KA_ESC` and `KA_TILDA` in the two
    `score.cc` and `queue.cc` files, `KA_a`/`KA_1` in `td/mapedit.cc`, and `KA_NONE` in
    `keyboard.cc`.
  - `ToAscii()` returns 0 or a character no higher than `'z'`.
- **The `VK_*` macros exist to give `KN_*` its values.** Outside the header they appear in
  `keyboard.cc` (the mouse buttons), `keyboard_test.cc`, TD's debug hotkeys in `td/conquer.cc` (27),
  and two comments in `ra/wolapiob.cc`.
- **26 `KN_*` names are never used:**
  - the keypad keys: `KN_E_*` (except `KN_E_HOME`), `KN_KEYPAD_ASTERISK/MINUS/PLUS/SLASH`
  - lock and system keys: `KN_CAPSLOCK`, `KN_NUMLOCK`, `KN_SCROLLLOCK`, `KN_PAUSE`, `KN_PRNTSCRN`
  - punctuation: `KN_BACKSLASH`, `KN_EQUAL`, `KN_MINUS`, `KN_LBRACKET`, `KN_RBRACKET`,
    `KN_SEMICOLON`, `KN_SQUOTE`
  - `KN_F8`
- **The SDL port never sets `kKeyVirtualBit`.** `PutKey()` adds only Shift, Ctrl, Alt and release.
  Every test of the bit (about 69) therefore always takes the same branch. Examples are the
  keypad-to-digit branches in both `edit.cc` files and TD's `conquer.cc` hotkey masking.

**Kept:** `kKey*Bit` (`uint32_t`, for mask arithmetic) and `KN_*_BIT` (the `KeyNumber` spelling that
combines through `KeyNumber`'s operators). Each has about 30 or more uses in its own kind of
expression, and merging them would only trade casts. The `KA_*_BIT` copies go with `KeyAscii`.

## Step 1: `KeyAscii` becomes `char`

- Delete the fall-through `case KA_*:` labels in the four switches. Behavior is unchanged: they all
  reach `default:`.
- `KeyBuffer::ToAscii()` returns `char`, with `'\0'` for "no character". `EditClass::Handle_Key()`
  takes a `char` in both games.
- Replace the remaining names with character literals, including the two `ascii <= KA_TILDA` bounds:

  | Name           | Literal  |
  | -------------- | -------- |
  | `KA_RETURN`    | `'\r'`   |
  | `KA_BACKSPACE` | `'\b'`   |
  | `KA_ESC`       | `'\x1b'` |
  | `KA_TAB`       | `'\t'`   |
  | `KA_TILDA`     | `'~'`    |
  | `KA_a`         | `'a'`    |
  | `KA_1`         | `'1'`    |
  | `KA_NONE`      | `'\0'`   |

- Drop the `static_cast<KeyAscii>(...)` wrappers, and drop the `& 0xff` masks on `ToAscii()`
  results, which are no-ops now.
- Delete `enum KeyAscii` and the `using enum engine::window::KeyAscii;` lines.

## Step 2: fold `VK_*` into `KeyNumber`

- Give each `KN_*` its SDL scancode as a literal. Keep the header comment explaining that key
  numbers are SDL scancodes and that the mouse buttons take 1-3.
- Replace the outside uses: `keyboard.cc`/`keyboard_test.cc` take `KN_LMOUSE`, `KN_RMOUSE` and
  `KN_MMOUSE`. `KN_MMOUSE` is new, for the middle button. `td/conquer.cc` takes `KN_*` (`KN_HOME`,
  `KN_F7`...`KN_F10`, `KN_0`...`KN_9`, ...). The `ra/wolapiob.cc` comments go with their dead
  `GetAsyncKeyState` code.
- Reword the comments that say "VK\_\* code" (`PutKey`, `PutClick`, the class comment) to say key
  number.
- Delete every `#define VK_*`.

## Step 3: drop the unused `KeyNumber` names

Delete the 26 names listed above. The remaining aliases (`KN_RSHIFT` = `KN_LSHIFT`, the diagonals =
the navigation keys, ...) are used and stay.

## Step 4: remove `kKeyVirtualBit`

- Confirm nothing sets the bit, including the key-binding options that RA and TD load from INI.
- Delete the branches it guards and simplify the conditions: a test for the bit is always false, and
  a test for its absence is always true.
- `kKeyCodeMask` becomes `kScancodeMask`, and `KeyCode()` masks only the scancode.

## Verification

For each step:

- Build `build/` and the full strict build of both games (`--parallel 14`).
- Run `ctest` (840 tests, including `engine_window_keyboard_test` and `cpplint_test`).
- Run `tools/td_saveload_smoke.sh` and `tools/ra_saveload_smoke.sh`.
- After step 1, `grep -rn "KA_\|KeyAscii" src` is empty. After step 2, `grep -rn "VK_" src` finds
  only the unbuilt `ra/winstub.cc`, if anything.

Manual check on a real display: typing, backspace, Enter and Esc in an edit box and in the in-game
message line; the hall-of-fame name entry on the score screen; TD's map editor waypoint keys
(`a`..., `1`...); TD's hotkeys (Home, F7-F10, 0-9).

## Progress

- [ ] Step 1: `KeyAscii` becomes `char`
- [ ] Step 2: fold `VK_*` into `KeyNumber`
- [ ] Step 3: drop the unused `KeyNumber` names
- [ ] Step 4: remove `kKeyVirtualBit`
