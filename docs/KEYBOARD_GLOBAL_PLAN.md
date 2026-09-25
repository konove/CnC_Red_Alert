# Remove the Active Keyboard Global

Measured on the tree at 5e0db665.

## Findings

`engine/window/keyboard.h:146` declares `extern KeyBuffer* g_active_keyboard`. Each game's `Input`
constructor points it at the game's keyboard, and the destructor nulls it. It exists only to back
four legacy free functions (`PeekKey`, `ReadKey`, `ReadKeyAscii`, `ClearKeys`) and about 50 direct
`g_active_keyboard->click_x()/click_y()` reads.

What uses it (surveyed 2026-09-25):

- **Engine:** nothing reads it; `keyboard.cc:19` only defines it.
- **RA:** only sets it (`ra/input.cc`). RA code goes through `TheKeyboard()`, an `Installed<Input>`
  accessor, and through `struct KeyboardClass : KeyBuffer` in `ra/jshell.h:74`. That wrapper adds
  little:
  - `Get()`/`Check()` just cast `Read()`/`Peek()` to `KeyNumber`.
  - `To_ASCII()`/`Down()` are casts of `ToAscii()`/`IsDown()`.
  - `Mouse_X/Y()` forward to `Get_Mouse_X/Y()`.
  - `IsLibrary` is never read.
  - The one `Message_Handler` call is in `winstub.cc`, which the build excludes.
  - RA call sites: `TheKeyboard().Check` 35, `.Get` 12; `KeyboardClass::Down` 45, `::To_ASCII` 11,
    `::Mouse_X/Y` 1 each.
- **TD:** every reader.
  - A static facade, `class Keyboard` in `td/jshell.h:70`, with 105 calls: `Clear` 50, `Check` 25,
    `Down` 20, `Get` 9, `To_ASCII` 6, `Mouse_X/Y` 1 each. `Stuff()` is a no-op nobody calls.
  - About 45 calls to the free functions in `init.cc`, `score.cc`, `ending.cc`, `queue.cc`,
    `conquer.cc`, `startup.cc` and `findpath.cc`.
  - About 50 direct `g_active_keyboard->click_x/y` reads in 14 files, among them `mapeddlg.cc`,
    `nulldlg.cc`, `netdlg.cc`, `intro.cc`, `menus.cc`, `gadget.cc`, `radar.cc` and `display.cc`.

TD's `TheKeyboard()` (`td/input.h`) already returns the same `KeyBuffer`, and it lives exactly as
long as the global did. `Game` declares `input_scope_` right after `input_` (`td/game.h:79-80`,
`ra/game.h:88-89`). Reading keys outside a `Game` now fails a CHECK instead of dereferencing a null
pointer.

**Decision (user):** both games share the engine's `engine::window::KeyBuffer` directly. The
`KeyNumber` narrowing moves into the engine; RA's `KeyboardClass` and TD's static `Keyboard` are
deleted, and neither game keeps a per-game wrapper.

## Step 1: engine returns the game's key types

- `engine/window/keyboard.h/.cc`: `Peek()` and `Read()` return `KeyNumber` (0 is `KN_NONE`), and
  `ToAscii()` returns `KeyAscii`.
  - The cast from a buffer entry takes the same
    `NOLINTNEXTLINE(clang-analyzer-optin.core.EnumCastOutOfRange)` the `KeyNumber` operators use.
  - `IsDown(int)` and `IsMouseKey(int)` stay as they are; `KeyNumber` converts to `int` implicitly.
- Update the class example in the header, and `keyboard_test.cc` if the gtest comparisons need it.
- Game code still compiles unchanged: the wrappers' casts become no-ops.

## Step 2: RA drops `KeyboardClass`

- Rewrite the call sites across `src/ra` mechanically:
  - `TheKeyboard().Check()` becomes `.Peek()`
  - `TheKeyboard().Get()` becomes `.Read()`
  - `KeyboardClass::Down(` becomes `engine::window::KeyBuffer::IsDown(`
  - `KeyboardClass::To_ASCII(` becomes `engine::window::KeyBuffer::ToAscii(`
  - `KeyboardClass::Mouse_X()/Mouse_Y()` become `Get_Mouse_X()/Get_Mouse_Y()`
- Delete `KeyboardClass` from `ra/jshell.h`.
- `ra/input.h/.cc`: hold `engine::window::KeyBuffer keyboard_` by value, as TD does, and make
  `keyboard()`/`TheKeyboard()` return `KeyBuffer&`. Include `engine/window/keyboard.h` in place of
  `ra/jshell.h` if nothing else needs it.
- Leave `winstub.cc` alone: it is not built, and its `Message_Handler` call already names a method
  that no longer exists.

## Step 3: TD drops the static `Keyboard` and stops reading the global

Across `src/td`:

- `Keyboard::Check()`, `Get()` and `Clear()` become `TheKeyboard().Peek()`, `.Read()` and
  `.Clear()`.
- `Keyboard::Down(` and `To_ASCII(` become `engine::window::KeyBuffer::IsDown(` and `ToAscii(`.
  `Keyboard::Mouse_X/Y()` become `Get_Mouse_X/Y()`.
- `engine::window::g_active_keyboard->` becomes `TheKeyboard().`
- The free functions:
  - `PeekKey()` becomes `TheKeyboard().Peek()`.
  - `ReadKey()` becomes `TheKeyboard().Read()`.
  - `ClearKeys()` becomes `TheKeyboard().Clear()`.
  - `ReadKeyAscii()` becomes `TheKeyboard().Read()` where its result is discarded (`conquer.cc:508`,
    `startup.cc:516`, `init.cc:1124`). Where the result is used (`queue.cc:3416`, `score.cc`), it
    becomes `KeyBuffer::ToAscii(TheKeyboard().Read())`.
- Delete `class Keyboard` from `td/jshell.h`.
- Add `#include "td/input.h"` wherever `misc-include-cleaner` asks, and drop includes that are no
  longer used.

## Step 4: delete the global and the free functions

- `engine/window/keyboard.h`: remove `g_active_keyboard` and the `PeekKey`, `ReadKeyAscii`,
  `ReadKey` and `ClearKeys` inlines, together with their comments.
- `engine/window/keyboard.cc`: remove the definition.
- `ra/input.cc`, `td/input.cc`:
  - The constructors no longer set the global, so they become `= default`.
  - The destructors become `= default` in the `.cc`, which the `unique_ptr<WWMouseClass>` to an
    incomplete type needs.
- `ra/input.h`, `td/input.h`: drop the paragraph about `g_active_keyboard`, and point the examples
  at `TheKeyboard().Peek()`.

Commit each step on its own. Steps 2 and 3 are mechanical sed runs; run `git clang-format` on the
touched files afterward.

## Verification

- `grep -rnE "g_active_keyboard|PeekKey|ReadKeyAscii|ClearKeys|\bReadKey\(|KeyboardClass|\bKeyboard::" src`
  returns nothing, apart from the unbuilt `ra/winstub.cc`.
- While editing, run `tools/strict_tu.py` on the touched files.
- Before each commit, do a full strict build of both games at `--parallel 14`, then run `ctest`,
  including `keyboard_test`, `engine_layers_test` and `cpplint_test`.
- Run the headless smoke tests, `tools/td_saveload_smoke.sh` and `tools/ra_saveload_smoke.sh`.
- Keys cannot be exercised headless, so the user checks them by hand:
  - TD: dismiss the score screen with a key; press Esc in a movie and in the ending; click a menu
    item (`menus.cc` click_y); click a color in the multiplayer dialog (`nulldlg.cc`/`netdlg.cc`).
  - RA: hotkeys in battle, typing in an edit box (`edit.cc` `ToAscii`), and Ctrl+Q on map select
    (`IsDown`).

## Progress

- [x] Step 1: engine returns the game's key types (92c9ada2)
- [x] Step 2: RA drops KeyboardClass (ef2ff748)
- [x] Step 3: TD drops the static Keyboard and stops reading the global (cf88407e)
- [x] Step 4: delete the global and the free functions

The key handling still wants a manual check on a real display (see Verification).
