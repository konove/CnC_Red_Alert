# Take Mouse Clicks out of KeyNumber

## Findings

Measured on the tree at 35a300cd.

The games read one ordered queue of input. `engine::window::KeyBuffer` stores both kinds of entry:

- a key press or release is one entry;
- a mouse click is three: the button as a key number (`KN_LMOUSE`, `KN_RMOUSE`, `KN_MMOUSE`), then
  the x and y position.

`Read()` returns either kind as a `KeyNumber`. For a click it leaves the position in `click_x()` and
`click_y()`, a side channel that the next `Read()` overwrites.

How the value travels:

- **Gadget chain.** `GadgetClass::Input()` (`ra/gadget.cc`, `td/gadget.cc`) reads one entry. It
  turns a click into press/release flags and a position, and offers it to each gadget's
  `Clicked_On(key, flags, x, y)`. A gadget that acts replaces `key` with its `ButtonKey(id)`.
  Whatever is left is returned. About 99 dialog call sites read that return value.
- **Map chain.** `GScreenClass::Input(KeyNumber&, int& x, int& y)` runs the map's gadgets (or reads
  the queue itself) and hands the result to the `AI(KeyNumber&, int, int)` chain of the map layers.
- **Unconsumed clicks.** A click that no gadget took stays in `input` for the caller. That is why 27
  files test `input == KN_LMOUSE` / `KN_RMOUSE` (63 tests) and 25 files read `click_x()` /
  `click_y()` (125 reads).

So "which key" and "which mouse button, where" share one int. Every consumer has to ask whether its
key is really a mouse button, and the position comes from a global that is only right just after the
matching `Read()`.

## Target

```cpp
namespace engine::window {

enum class MouseButton { kNone, kLeft, kRight, kMiddle };

// One entry from the input queue: a key or a click.
struct InputEvent {
  KeyNumber key = KN_NONE;           // key press or release, or a gadget's ButtonKey()
  MouseButton button = MouseButton::kNone;
  bool release = false;              // for a click
  int x = 0;                         // for a click, in game pixels
  int y = 0;
  // Helpers such as IsPress(MouseButton) and IsRelease(MouseButton).
};

}  // namespace engine::window
```

- `KeyBuffer` becomes a ring of `InputEvent`s, and `Read()`/`Peek()` return one.
- `GadgetClass::Input()` returns the event, with `key` replaced by the gadget's `ButtonKey()` when
  one acts. `GScreenClass::Input()` fills one for the map chain.
- `KeyNumber` names keys and gadget IDs only. `KN_LMOUSE`, `KN_RMOUSE`, `KN_MMOUSE`,
  `KeyBuffer::IsMouseKey()` and `click_x()`/`click_y()` are deleted.

## Phases

Every phase compiles and runs. Until the last phase, a click's event still carries its old key
number, so code that has not moved yet keeps working.

1. **Engine event.**
   - Add `MouseButton` and `InputEvent`.
   - `KeyBuffer` stores events. `ReadEvent()` returns one, and `Read()` returns its `key` (the
     click's `KN_*MOUSE` for a click), so every caller is unchanged.
   - Add unit tests.
2. **The two funnels return events.**
   - `GadgetClass::Input()` builds its flags and position from `ReadEvent()` and returns the event.
     Its callers take `.key`.
   - `GScreenClass::Input()` produces an event for the map chain.
   - Every `Read()` that can see a click moves to `ReadEvent()`: `intro.cc`, `ending.cc`,
     `mapsel.cc`, `menus.cc`, ...
3. **Consumers move to the event.**
   - Replace each `input == KN_LMOUSE` and each `click_x()` with the event, area by area: map and
     display, map editor, dialogs (`nulldlg`, `netdlg`, `mapeddlg`, ...), gadgets (`toggle`, `tab`,
     `msglist`), menus.
   - This is the bulk of the work. It is mechanical, and forks can take disjoint files.
4. **Flip.**
   - A click's event gets `key = KN_NONE`.
   - Delete `KN_LMOUSE`, `KN_RMOUSE`, `KN_MMOUSE`, `IsMouseKey()` and `click_x()`/`click_y()`.
   - `Read()` returns the event, and `ReadEvent()` folds into it.
   - The compiler finds whatever phase 3 missed.
   - `KeyFromWindowsKey()` stops mapping the Windows mouse codes (1, 2, 4), so an INI hotkey can no
     longer name a mouse button. The games never offered that binding. `KeyBuffer::IsDown()` then
     covers keys only.

## Risks

- **Gadget `Action(flags, key)` overrides that test `key` for a click** (81 `KeyNumber& key`
  parameters) see `KN_NONE` after the flip. The compiler catches any that name `KN_LMOUSE`. A test
  written as `key & ...` would not be caught, so phase 3 greps for them.
- **Order.** A click and a key typed in the same frame must still come out in the order they
  happened. A single ring of events keeps that.
- **Wrong position.** A consumer that reads a position after a later `Read()` currently gets the
  newer click's position. Taking it from the event fixes that, but can move where a stray click
  lands. Review such sites.

## Verification

For each phase:

- Build `build/` and do a full strict build (`--parallel 14`).
- Run `ctest`, including the keyboard unit tests.
- Run `tools/td_saveload_smoke.sh` and `tools/ra_saveload_smoke.sh`.
- After phase 4, `grep -rnE "KN_[LRM]MOUSE|click_[xy]|IsMouseKey" src` is empty.

Manual check on a real display:

- clicking and dragging gadgets and sliders,
- map clicks and drag-select,
- right-click to deselect,
- the dialogs' color pickers,
- the map editor's placement and drag handles,
- the intro and ending click zones.

## Progress

- [ ] Phase 1: engine event
- [ ] Phase 2: the two funnels return events
- [ ] Phase 3: consumers move to the event
- [ ] Phase 4: flip
