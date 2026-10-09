# Gadget list plan (T-224)

## Goal

Replace the intrusive `game::Link` chain under every gadget with a list object, `game::GadgetList`,
that holds its gadgets in a `std::vector<GadgetClass*>`. A dialog then owns one list, adds gadgets
to it and clears it in one call. `Link::ForgetNeighbors()` and, at the end, `Link` itself are
deleted.

Decided with the user on 2026-10-08: a list object rather than a whole-chain unlink or `Remove()` at
every call; the whole tree under T-224 rather than only the lists that call `ForgetNeighbors()`; a
vector rather than a head over the intrusive chain.

## Why

`ForgetNeighbors()` (the legacy `Zap()`) clears one node's pointers and leaves its neighbours
pointing at it. Dialogs call it on every gadget before rebuilding their list, which is correct only
when no gadget is missed. The cause is that there is no list: a list is "whichever node you hold",
so nothing can enumerate or empty it. With a list object the stale pointer cannot be written.

## Where things stand (measured 2026-10-08)

- `GadgetClass` is the only class that derives from `Link`.
- 677 add calls outside tests in 54 files: 562 `AppendTo`, 107 `InsertAfter`, 8 `PrependTo`.
- About 40 list heads, 34 of them a local `GadgetClass* commands`; on a head pointer there are 78
  `Input()`, 47 `Draw_All()`, 30 `Flag_List_To_Redraw()` and 6 `Extract_Gadget()` calls.
- About 150 `ForgetNeighbors()` calls: `ra/net/netdlg.cc` (33), `ra/wol/wol_gsup.cc` (33),
  `ra/wol/wolapiob.cc` (20), `td/editor/mapeddlg.cc` (13), `td/net/netdlg.cc` (12), the two
  `sidebar.cc` (19), `ra/net/udpaddr.cc` (3), the two `display.cc` (2), `ra/ui/drop.{h,cc}`.
- Composite gadgets add their parts by overriding the add functions and `Remove()`: `ListClass`
  (`game/ui/list.h`, scroll gadgets), `TListClass` (`ra/ui/tlist.h`), `DropListClass` and
  `TDropListClass` (`ra/ui/drop.{h,cc}`, the drop button, and the list while dropped).
- `GadgetClass::Input()` identifies a list by its head (`LastList != this`): a new head forces a
  redraw and drops `StuckOn` and `Focused`.
- `GadgetClass::Remove()` clears the gadget's keyboard focus; `ForgetNeighbors()` does not, and the
  network dialogs rebuild their list on every redraw while an edit field has the focus.

## Design

### `game::GadgetList` (`game/ui/gadget_list.h`, `.cc`, target `game_ui`)

```cpp
class GadgetList {
 public:
  GadgetList() = default;
  ~GadgetList();  // Clear()
  // Not copyable or movable: its gadgets point back at it.

  void Add(GadgetClass& gadget);        // at the end
  void AddFront(GadgetClass& gadget);   // first to be offered input
  void AddAfter(GadgetClass& gadget, const GadgetClass& predecessor);
  void Remove(GadgetClass& gadget);     // no-op when it is not in this list
  void Clear();

  engine::window::InputEvent Input(PixelView& view);
  void DrawAll(PixelView& view, bool forced = true);
  void FlagToRedraw();
  [[nodiscard]] bool IsToRedraw() const;
  [[nodiscard]] ControlClass* Extract(unsigned id) const;

  [[nodiscard]] bool empty() const;
  [[nodiscard]] std::span<GadgetClass* const> gadgets() const;

 private:
  std::vector<GadgetClass*> gadgets_;
};
```

- **One list per gadget.** `GadgetClass` gets a private `GadgetList* list_` (nullptr when in none)
  with a `list()` accessor. `Add*()` on a gadget that is in a list takes it out of that list first,
  so the "still linked" `DCHECK` and its release-build corruption have no equivalent. This is also
  how the WOL dialogs hand their ten shared buttons from one dialog's list to the other's.
- **Destruction.** `~GadgetClass()` removes the gadget from `list_`; `~GadgetList()` clears, which
  resets every member's `list_`. Either may die first.
- **Focus.** `Remove()` clears the gadget's focus and stuck state, as `GadgetClass::Remove()` does
  today. `Clear()` keeps them, as `ForgetNeighbors()` does today, so a rebuild on every redraw does
  not take the focus from an edit field.
- **Composite gadgets.** `GadgetClass` gets two protected virtual hooks, `OnAdded(GadgetList&)` and
  `OnRemoved(GadgetList&)`, called by the list after it added or removed the gadget (and `OnRemoved`
  for every member on `Clear()`). `ListClass`/`TListClass` add and remove their scroll gadgets
  there, `DropListClass`/`TDropListClass` their button and, while dropped, their list; `OnRemoved`
  also collapses a drop list. These replace the four overrides of each add function.
- **Input.** The body of `GadgetClass::Input()` moves to `GadgetList::Input()`. `LastList` becomes a
  `const GadgetList*`, so a list keeps its identity when its first gadget changes: adding in front
  no longer forces a redraw or drops the focus. Each phase checks the call sites that relied on that
  (`PrependTo` has 8). The sweep iterates by index and re-reads the size, because a gadget's action
  may add or remove gadgets (a drop list expanding, the message list).
- **Statics.** `StuckOn`, `Focused` and the key hook stay on `GadgetClass`; `LastList` moves to
  `GadgetList`.

### Call sites

| Before                                              | After                                                          |
| --------------------------------------------------- | -------------------------------------------------------------- |
| `GadgetClass* commands = &a; b.AppendTo(*commands)` | `game::GadgetList commands; commands.Add(a); commands.Add(b);` |
| `b.InsertAfter(a)`                                  | `commands.AddAfter(b, a)`                                      |
| `commands = &b.PrependTo(*commands)`                | `commands.AddFront(b)`                                         |
| `commands->Input(view)`, `->Draw_All(view)`         | `commands.Input(view)`, `.DrawAll(view)`                       |
| `commands->Flag_List_To_Redraw()`                   | `commands.FlagToRedraw()`                                      |
| `x.ForgetNeighbors()` × n, then the rebuild         | `commands.Clear()`, then the rebuild                           |
| `x.ForgetNeighbors(); Add_A_Button(x)`              | `Add_A_Button(x)` (it already removes first)                   |
| `x.Remove()`                                        | `commands.Remove(x)`                                           |
| a walk with `Next()`                                | `for (GadgetClass* g : commands.gadgets())`                    |

Functions that take a `GadgetClass*` list head take a `GadgetList&`.

## Phases

Each phase builds, passes `ctest` and the strict build, and is one or more commits.

1. **`GadgetList` beside `Link`.** Add the class, `list_`, the hooks and `gadget_list_test.cc`
   (order, move between lists, either destruction order, `Clear()` keeps focus, `Remove()` drops it,
   mutation during `Input()`, the composite gadgets). `GadgetClass::Input()` and the list's share
   one implementation meanwhile. Nothing uses the list yet; composite gadgets carry both the
   overrides and the hooks until phase 5.
2. **The map's button list**, both games: `GScreenClass::Buttons` becomes a `GadgetList`;
   `Add_A_Button()`/`Remove_A_Button()` forward to it; the sidebar, `display.cc`, the message list
   and everything else that reaches `Buttons` follow. The 21 redundant `ForgetNeighbors()` calls in
   front of `Add_A_Button()` go.
3. **The lists that call `ForgetNeighbors()`:** `ra/net/netdlg.cc`, `td/net/netdlg.cc`,
   `ra/net/udpaddr.cc`, `td/editor/mapeddlg.cc`, `ra/wol/wol_gsup.cc`, `ra/wol/wolapiob.cc`,
   `ra/wol/wol_chat.cc`. Done by hand: these are the rebuilds and the shared buttons.
4. **Every other dialog,** by folder (`screens/`, `ui/`, `editor/`, `net/`, `wol/`, `ai/`) in both
   games: mechanical, by parallel agents on disjoint files with the table above as the recipe.
5. **Delete `Link`.** Remove the add/remove overrides from the composite gadgets, `GadgetClass`'s
   list functions (`Input`, `Draw_All`, `Extract_Gadget`, `Flag_List_To_Redraw`,
   `Is_List_To_Redraw`, `Remove`, `Next`, `Previous`), its `Link` base, `link.h`, `link.cc` and
   `link_test.cc`. Fix the three comments a `Zap` → `ForgetNeighbors` rename damaged
   (`ra/map/vortex.cc:707`, `ra/net/netdlg.cc:4501`, `engine/video/vqa/vqa_format.h:155,161`).
   Update `CLAUDE.md` (the `game/ui/` line) and the open TODO items anchored in `link.h`.

## Verification

- Unit tests: `gadget_list_test.cc`; the existing `UiFixture` dialog tests keep passing unchanged in
  what they check.
- `tools/ui_check.py` for both games after phases 2, 3 and 4: no screenshot hash may change.
- `tools/net_check.py` after phase 3.
- By hand, since no script reaches them: the RA and TD network join/new-game dialogs (type a name
  while the list refreshes: the field keeps the focus), the TD trigger editor, a drop list opened
  and closed in the RA lobby. The WOL dialogs cannot be run (no server); they are converted by
  reading and covered only by the build.

## Risks

- **Offer order.** `Input()` gives an event to the first gadget that takes it, so every
  `InsertAfter` and `PrependTo` site must keep its position. The 107 `InsertAfter` calls are
  reviewed one by one, not rewritten by pattern.
- **List identity.** Code that relied on a new head to force a redraw needs an explicit
  `FlagToRedraw()`.
- **Gadgets in a list nobody holds.** A chain can exist today without a head variable (a gadget
  linked to a peer). Each such case needs a list to live in; phase 4 reports them instead of
  guessing.
