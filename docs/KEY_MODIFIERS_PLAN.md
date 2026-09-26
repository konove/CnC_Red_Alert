# Simplify Key Modifiers

## Findings

Measured on the tree at f5b8beeb.

A key number packs the key's scancode in the low byte with flag bits above it: Shift, Ctrl and Alt
(`KN_*_BIT`), release (`KN_RLSE_BIT`) and gadget button (`KN_BUTTON`). The packed value has to stay.
The games `switch` on it (`case ButtonKey(BUTTON_OK):`) and pass it through the gadget chain by
reference (146 `KeyNumber&` signatures).

What is cumbersome is that every caller does the bit arithmetic itself, in several spellings. Counts
are for `src/ra` and `src/td`:

| Pattern                                                                                         | Uses |
| ----------------------------------------------------------------------------------------------- | ---- |
| `case (KN_X \| KN_ALT_BIT):`, `== (KN_LMOUSE \| KN_RLSE_BIT)`                                   | 107  |
| which key: `input & 0xff`, `& 0xFFU`, `& ~KN_RLSE_BIT`, `KeyCode()`, `& ~(kKeyShiftBit \| ...)` | ~30  |
| two spellings of each bit: `kKeyAltBit` (uint32) and `KN_ALT_BIT`                               | 22   |
| gadget ID back out: `input & ~KN_BUTTON`                                                        | 7    |
| live modifier state: `IsDown(KN_LSHIFT) \|\| IsDown(KN_RSHIFT)`, where both name one key        | 19   |

## Step 1: helpers

Add `constexpr` free functions to `engine/window/keyboard.h`, next to `ButtonKey()`, so they work as
`case` labels:

```cpp
constexpr KeyNumber Shift(KeyNumber key);     // key | KN_SHIFT_BIT
constexpr KeyNumber Ctrl(KeyNumber key);
constexpr KeyNumber Alt(KeyNumber key);
constexpr KeyNumber Released(KeyNumber key);  // key | KN_RLSE_BIT
constexpr KeyNumber KeyCode(KeyNumber key);   // the scancode alone
constexpr bool HasShift(KeyNumber key);
constexpr bool HasCtrl(KeyNumber key);
constexpr bool HasAlt(KeyNumber key);
constexpr bool IsRelease(KeyNumber key);
constexpr int ButtonId(KeyNumber key);        // inverse of ButtonKey()
bool IsShiftDown();                           // live state, either side
bool IsCtrlDown();
bool IsAltDown();
```

`KeyCode()` changes from `int(int)` to `KeyNumber(KeyNumber)`. Unit tests go in `keyboard_test.cc`.

## Step 2: sweep the call sites

Rewrite each pattern in the table:

- `(KN_X | KN_ALT_BIT)` becomes `Alt(KN_X)`, and nested bits nest: `Alt(Shift(KN_UP))`.
- The "which key" masks become `KeyCode()`.
- `input & ~KN_BUTTON` becomes `ButtonId(input)`.
- The live modifier pairs become `IsShiftDown()`, `IsCtrlDown()` and `IsAltDown()`.
- Game code stops naming `kKey*Bit`, which stays in the header for the engine's own mask arithmetic.

**Check each `ToAscii(input & 0xff)` site.** The mask strips the release bit, so a release returns a
character there, while `ToAscii(input)` returns nothing for a release. Where the mask exists only to
drop the modifiers, write `ToAscii(input)`. Where a release really does reach that site, keep what
it does today and note it.

## Step 3: live mouse state leaves `KeyBuffer::IsDown()`

`KeyBuffer::IsDown(KN_LMOUSE)` (18 uses) asks the mouse, not the keyboard. Give the mouse module a
`IsMouseButtonDown()` for the left and right buttons. `IsDown()` then covers keys only.

## Mouse clicks in `KeyNumber`: analysis, not scheduled

Clicks live in the key enum because the games read one ordered queue of input:

- `KeyBuffer` holds key presses and clicks in arrival order.
- `Read()` returns either kind as a key number.
- The click's position travels on a side channel, `click_x()`/`click_y()` (125 reads).

`GadgetClass::Input()` turns the value into its press and release flags. Anything no gadget consumes
stays in `input` for the caller, and 28 files test it for a click (`KN_LMOUSE` 51 times, `KN_RMOUSE`
30).

Taking clicks out means replacing the value with an event, for example
`struct InputEvent { KeyNumber key; MouseButton button; bool release; int x, y; }`. That would:

- change the type of `input` in the 146 `KeyNumber&` signatures,
- rewrite the click tests and the dialog `switch`es that mix `case KN_LMOUSE:` with key cases,
- retire `click_x()`/`click_y()`, since the position would ride in the event.

It is worth doing: it would end the side channel and the "is this key a mouse button" tests. But it
is a redesign of the games' input loop and needs its own plan. Step 3 is the part that stands alone.

## Verification

For each step:

- Build `build/` and the full strict build (`--parallel 14`).
- Run `ctest`.
- Run `tools/td_saveload_smoke.sh` and `tools/ra_saveload_smoke.sh`.
- After step 2, `grep -rnE "KN_[A-Z]+_BIT|kKey[A-Z][a-z]+Bit|& 0x[fF]{2}" src/ra src/td` shows only
  what the notes explain.

Manual check on a real display:

- dialogs with Alt shortcuts,
- click, release and drag on gadgets,
- Shift-, Ctrl- and Alt-clicks on the map,
- typing in edit boxes.

## Progress

- [x] Step 1: helpers
- [x] Step 2: sweep the call sites. Two additions on the way:
  - Five sites strip the modifiers but keep the release bit (RA's `Do_Menu()`, the "plain" hotkey
    value, `KN_To_Facing()` in both games), so there is also `WithoutModifiers()`.
  - The `ToAscii(input & 0xff)` sites became `ToAscii(KeyCode(input))`, which masks exactly the same
    bits, so a release still yields its character there.

  What is left spells the bits only in commented-out code and in `td/msgbox.cc`'s `#ifdef NEVER`
  block.

- [x] Step 3: live mouse state leaves `KeyBuffer::IsDown()`. The games ask `IsLeftButtonDown()` and
      `IsRightButtonDown()` in `ww_mouse.h`; `IsDown()` still answers for a mouse button by asking
      them, because a hotkey binding may name one.
