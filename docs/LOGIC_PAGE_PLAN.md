# sdllib Display and `LogicPage` Removal Plan

## Findings

The globals plan (`docs/GLOBALS_PLAN.md`) moved both games' state into `Game`-owned subsystems and
deliberately left sdllib's own globals alone -- its closing note lists `IsTheaterShape` and
`CurrentPalette` as belonging to sdllib. Two more were left for the same reason, and phase 1 only
taught their destructors to null them so a destroyed `Screen` could not leave them dangling:

```cpp
PixelBuffer* WindowBuffer = nullptr;   // src/sdllib/pixel_buffer.cc:57
PixelView* LogicPage = nullptr;        // src/sdllib/pixel_buffer.cc:58
```

They are unrelated problems that happen to share a file.

**`WindowBuffer` is a genuine process singleton.** All 20 uses mean "the one `PixelBuffer` backed by
the SDL window", of which there is exactly one. It self-registers: `PixelBuffer::Init()` assigns
`WindowBuffer = this` when passed `BUFFER_VISIBLE` (`src/sdllib/pixel_buffer.cc:1100`) and
`~PixelBuffer` clears it (`:1070`). Its readers are free functions with no natural owner --
`SetScreenPalette()` (`src/sdllib/palette.cc:7`), `Video_End_Frame()`
(`src/sdllib/pixel_buffer.cc:1442`), `WWMouseClass::Update_Palette()` (`src/sdllib/ww_mouse.cc:286`)
and each game's `interpal.cc`. It does not want to be threaded away; it wants an owner.

It also does not sit alone. `src/sdllib/ww_win.cc` holds the rest of the same concern: `MainWindow`
(`:25`), `SDLRenderer` (`:31`), `ForceRenderEventID` (`:32`) and the present cadence hidden in a
function-static inside `PresentFrame()` (`:75`). Nothing destroys the window or the renderer today.

**`LogicPage` is an implicit parameter.** sdllib never draws through it; it is purely a convention
between the two games, naming the view the next draw lands on. Live counts:

| form                               | RA      | TD      |
| ---------------------------------- | ------- | ------- |
| `LogicPage->...` reads             | 158     | 150     |
| `TheScreen().IsVisible(LogicPage)` | 25      | 20      |
| `*LogicPage` passed by reference   | 6       | 9       |
| `PixelView* old = LogicPage;`      | 2       | 2       |
| **total live reads**               | **191** | **181** |
| `SetLogicPage(...)`                | 83      | 79      |

Of the reads, 25 RA / 17 TD are `Lock()`/`Unlock()` rather than draws: the global also names the
surface whose lock is held, and those locks nest across translation units.

Three facts shape the work:

- **The cost is the fan-out, not the reads.** `Fancy_Text_Print` reaches the global through two
  lines of `Simple_Text_Print` (`src/ra/dialog.cc:669-670`) and has 298 RA call sites in 49 files
  (255/37 in TD). `CC_Draw_Shape` is 106/29 (77/27), `Dialog_Box` 53/31 (44/19), `Draw_Box` 49/19
  (34/15), `Draw_Caption` 48/27 (48/19). Giving the leaf primitives a view parameter touches roughly
  1,100 call sites across both games.
- **Six hierarchies carry the target implicitly.** `GadgetClass::Draw_Me` (`src/ra/gadget.h:155`, 14
  RA + 11 TD overrides), `GScreenClass::Draw_It` (`src/ra/gscreen.h:93`, 7 + 7),
  `ObjectClass::Draw_It` (`src/ra/object.h:246`, 12 overrides, `const` in RA and non-`const` in TD),
  `ObjectTypeClass::Display` (`src/ra/type.h:350`, 9 overrides), `ListClass::Draw_Entry`
  (`src/ra/list.h:143`, 5 + 3), and a duck-typed `Draw_It` required by `TListClass<T>` at
  `src/ra/list.h:492` and implemented by 7 unrelated RA classes with no base class, so a signature
  change there gets no compiler help until instantiation.
- **The global is load-bearing across translation units.** `GScreenClass::Render()`
  (`src/ra/gscreen.cc:384`) sets the hidden view and the entire map renderer below it reads the
  global from other TUs. `GScreenClass::Input()` (`:283`) does the same for nine TUs of `Draw_Me`.
  About 115 one-way `SetLogicPage(TheScreen().visible_view())` calls sit at the top of modal dialog
  functions purely so that gadget draws elsewhere land correctly; several of those functions never
  read the global themselves at all.

**A latent crash.** `~PixelView` (`src/sdllib/pixel_buffer.cc:60-64`) nulls the global when the
dying view is the logic page, and `delete RenderBuffer` (`src/ra/vortex.cc:140,316`) and
`delete PseudoSeenBuff` (`src/td/ending.cc:270`) can trigger exactly that. Nothing in either game
null-checks `LogicPage` before dereferencing it.

**Scope.** `WindowBuffer`, `LogicPage` and the `ww_win.cc` window globals. sdllib's other leftovers
are out of scope and stay: `WindowList` (180 uses) is the legacy text-window table, `IsTheaterShape`
(52) is a shape-decoding flag and `CurrentPalette` (36) is palette state -- none is display state,
and each deserves its own decision.

**What this plan does not claim.** Phase 0 replaces a global pointer with a subsystem reachable
through `TheDisplay()`; that is still process-wide state. The gains are an explicit lifetime, a
window and renderer that are destroyed, and no self-registration from a constructor. Only phases 1-7
remove state outright.

## Pattern

**Phase 0** follows the globals plan's subsystem pattern: a class with Google-style members in a new
`sdllib/display.h/.cc`, a constructor that does no I/O, an `Init()` that opens the window later, a
`base::Installed<Display>::Scope` in each game's `Game`, and a `TheDisplay()` accessor.
`tech/audio_mixer.h`'s `AudioMixer` is the precedent for a subsystem that lives in a shared library
and is installed by each game.

**Phases 1-7** use one mechanism throughout. A leaf primitive gains a leading `PixelView& view`
parameter and uses it instead of the global; every call site that has no view in hand passes
`*LogicPage` explicitly. Textual mentions of the global go up while hidden reads go to zero. That is
the point: the dependency becomes visible and greppable, the compiler enforces every call site, and
each later phase is a local edit turning one `*LogicPage` argument into a real view. The tree builds
and behaves identically at every step.

**The endpoint rule.** A threaded parameter is not always the answer. A leaf primitive called from
many contexts takes a parameter. A dialog that always draws to the visible view names it locally:

```cpp
PixelView& view = TheScreen().visible_view();
```

The goal is that the drawing target is named where it is used, not that everything becomes a
parameter.

## Phases

### 0. `Display`

New `src/sdllib/display.h/.cc`. The class absorbs `MainWindow`, `SDLRenderer`, `ForceRenderEventID`,
the present cadence and the `WindowBuffer` pointer. SDL types stay `void*` in the header with the
casts in the `.cc`, as `PixelBuffer` already does.

```cpp
class Display {
  Display();                                       // no window yet, like Screen
  explicit Display(void* window, void* renderer);  // tests adopt an existing pair
  bool Init(const char* title, int width, int height);  // was SDL_Create_Main_Window
  ~Display();                                      // destroys renderer and window

  void AttachWindowPage(PixelBuffer& page);
  PixelBuffer* window_page();                      // nullable, as WindowBuffer is today

  void EndFrame();                                 // was Video_End_Frame()
  void SetPalette(std::span<const uint8_t> palette);  // was SetScreenPalette()
  bool SetVideoMode(int width, int height, int bits_per_pixel);

  int DisplayIndex();                              // for the mouse
  void SetMouseGrab(bool grab);
};
```

The window handle never escapes `Display`: every use becomes a method -- creating the renderer,
`SDL_GetWindowDisplayIndex` (`src/sdllib/ww_mouse.cc:77-79`), `SDL_SetWindowGrab` (`:275,280`), and
the restore/raise and focus test that `src/ra/wolapiob.cc:110-117` needs. The renderer does escape,
as a `void*`: `pixel_buffer.cc` creates textures and presents with it in six places, so `renderer()`
is public and documented as being for sdllib's own drawing code.

Two parameters that take the handle today ignore it and lose it: `Set_Video_Mode(void* hwnd, ...)`
(`src/sdllib/misc.cc:27`) and `VQA_OpenAudio(VQAHandle*, void* window)`
(`src/winvq/vqa32/audio.cc:272`). Dropping the second also ends winvq's link seam on `MainWindow`,
so `src/winvq/vqa32/vqaplay_test.cc:30-31` stops defining its own copy. The eight remaining
`MainWindow` mentions are comments or live in `winstub.cc`, which `src/ra/CMakeLists.txt:44`
excludes from the build.

`Screen` keeps owning the page. `Screen::Init()` (`src/ra/screen.cc:29`, `src/td/screen.cc:28`)
calls `TheDisplay().AttachWindowPage(visible_page_)` right after `Init(..., BUFFER_VISIBLE)`, which
deletes the self-registration at `src/sdllib/pixel_buffer.cc:1100` and the destructor reset at
`:1070`. `Display` is a `Game` member declared before `Screen`, and `Init()` is called where
`Create_Main_Window()` is today (`src/ra/startup.cc:361`, `src/td/startup.cc:346`), which already
runs before `TheScreen().Init()`.

`Display::SetPalette()` lives in its own translation unit, `src/sdllib/display_palette.cc`. It calls
`Update_Mouse_Palette()`, and with that reference in `display.cc` every test that touches a
`Display` links `ww_mouse.cc`: `ra_intro_test`, which stubs `Hide_Mouse`/`Show_Mouse`, then fails on
duplicate symbols, and `ra_screen_test` needs the LCW decoder the cursor decompresses with. The
deleted `palette.cc` was keeping exactly that separation.

Call sites: `Video_End_Frame()` to `TheDisplay().EndFrame()` (11, including `Wait_Vert_Blank()` at
`src/sdllib/misc.h:92` and the event loop at `src/sdllib/ww_win.cc:108`); `SetScreenPalette()` to
`TheDisplay().SetPalette()` (15); `WindowBuffer` to `TheDisplay().window_page()` (20). Code that can
run outside a `Game` tests `base::Installed<Display>::IsInstalled()`, the pattern
`src/ra/game_clock.h:51` already uses, which preserves the null checks at `src/sdllib/palette.cc:8`
and `src/sdllib/pixel_buffer.cc:1443`. `WWMouseClass::Update_Palette()` stops reading the global and
has the palette pushed in from `Display::SetPalette()`.

Tests: `src/sdllib/keyframe_test.cc:133` assigns `SDLRenderer` a software renderer directly and
instead installs a `Display` built with the adopting constructor. The two `WindowBuffer` assertions
in `src/sdllib/pixel_buffer_test.cc:192-194` move to the installed `Display`.

Deferred deliberately: moving the `BUFFER_VISIBLE` surface machinery (`window_texture_`,
`palette_surface_`, `redraw_timer_`, the scaled-frame texture, `CreateDisplaySurface()`) out of
`PixelBuffer` into `Display`, which would delete the `BUFFER_VISIBLE` special case entirely. It is
the cleaner end state, it is not needed to remove the global, and it re-opens code that 3c99c784 and
4be9d904 just consolidated.

### 1. Free deletions and latent bugs

- Delete `src/ra/bar.cc` and its header. `ProgressBarClass` has no constructor call and no caller
  anywhere in `src/` -- `ProgressBarClass` has zero mentions outside `src/ra/bar.*` -- and it
  compiles only because `src/ra/CMakeLists.txt:22-24` globs the directory. TD has no counterpart.
  Removes 8 reads.
- Delete the 21 RA and 14 TD commented-out `LogicPage->` lines.
- Fix `src/td/radar.cc:465`. The save is `const PixelView* oldpage = SetLogicPage(...)`, so it
  cannot be passed back, and the restore at `:496` is commented out; the variable's only remaining
  use is the behavioural test at `:489`. RA restores correctly at `src/ra/radar.cc:548`. Restoring
  TD's is a deliberate behaviour change and needs its own smoke run.
- Resolve the repair code at `src/td/netdlg.cc:4652-4656` and `:5271-5275`, which logs
  `"C&C95 - Logic page invalid"` and forces the page back to the visible view. Something in the
  network path leaves the global pointing at a third page; find it before the global can go.

### 2. Leaf primitives take a view

Each gains a leading `PixelView& view`; callers without one pass `*LogicPage`.

`Simple_Text_Print` (`src/ra/dialog.cc:398`) and its wrappers `Fancy_Text_Print`,
`Conquer_Clip_Text_Print` and `Plain_Text_Print`; `CC_Draw_Shape` (`src/ra/shape_draw.cc:48`,
`src/td/conquer.cc:2390`); `CC_Texture_Fill` (`src/td/conquer.cc:2334`, TD only); `Draw_Box`,
`Draw_Beveled_Box`, `Draw_Caption` and `Window_Box` (`src/ra/dialog.cc`); `Dialog_Box`
(`src/ra/dialog.cc:108`, which then stops touching the global at all); `LockedWindow` and its
`SaveSurfaceRect` / `RestoreSurfaceRect` / `DrawDib` entry points (`src/ra/winbits.cc:15`, RA only);
`Fat_Put_Pixel` (TD only).

About 1,100 call sites, mechanical and compiler-enforced. Disjoint files, so it parallelizes across
fork agents with a shared recipe.

### 3a. The GScreen render chain

`GScreenClass::Draw_It(bool)` becomes `Draw_It(PixelView&, bool)` through all 7 overrides in each
game (`display.h:131`, `radar.h:91`, `power.h:68`, `sidebar.h:98`, `tab.h:60`, `help.h:69`,
`mapedit.h:187` in RA), with every override forwarding the argument to its base call. The satellites
in the same chain follow: `DisplayClass::Redraw_Shadow()`, `SidebarClass::StripClass::Draw_It()`,
`RadarClass::Radar_Cursor` / `Radar_Anim` / `Plot_Radar_Pixel` / `Render_*` / `Draw_Names`, and
`MessageListClass::Draw` (called from `src/ra/gscreen.cc:404`).

`GScreenClass::Render()` (`:384`) then passes the hidden view instead of setting the global, and its
save/restore pair at `:384`/`:414` goes.

### 3b. The map object chain

`CellClass::Draw_It` (`src/ra/cell.cc:1067`, non-virtual), then `ObjectClass::Draw_It` and its 12
overrides, then `ObjectTypeClass::Display` (9 overrides) and the two `ObjectTypeClass::Draw_It`
declarations (`src/ra/type.h:1921,1994`). RA's `ObjectClass::Draw_It` is `const` and TD's is not, so
the two games need separate patches. TD additionally has `ObjectClass::Render(bool)`
(`src/td/object.cc:861`) reading the global directly.

### 4. The gadget hierarchy

`GadgetClass::Draw_Me(bool)` becomes `Draw_Me(PixelView&, bool)` across 14 RA and 11 TD overrides,
along with `GadgetClass::Draw_All()` and the non-virtual helpers
`StaticButtonClass::Draw_Background` and `TextButtonClass::Draw_Background`. The load-bearing change
is `GadgetClass::Input()` (`src/ra/gadget.cc:451`), which calls `Draw_Me` seven times and has no
page argument at all; it is the entry point every modal dialog loop uses.

`ListClass::Draw_Entry` (5 RA + 3 TD overrides) follows. The RA-only duck-typed protocol at
`src/ra/list.h:492` needs its 7 implementations (`session.h:459`, `taction.h:191`, `teamtype.h:98`,
`teamtype.h:183`, `tevent.h:227`, `trigger.h:100`, `trigtype.h:163`) edited in lockstep with the
template, with no diagnostic until instantiation.

The 25 RA / 20 TD `TheScreen().IsVisible(LogicPage)` predicates become `IsVisible(&view)`. They ask
an identity question -- "am I drawing straight to the screen, so hide the mouse?" -- which is why
every gadget draw needs the real view rather than any view of the same pixels.

### 5. Modal dialogs

The roughly 54 RA and 61 TD one-way sets each become one named local, passed to the `Input()`,
`Draw_All()` and leaf-primitive calls below them, and the `SetLogicPage` line is deleted. This is
where the `*LogicPage` arguments introduced in phase 2 turn into real views. The functions are
independent of each other, so this parallelizes as phase 2 does.

### 6. The residue

- `ScoreTimeClass::Update` and `ScoreCredsClass::Update` (`src/ra/score.cc:158,180`,
  `src/td/score.cc:329,354`), which save the global because they are called from a generic anim tick
  loop, and TD's `PseudoSeenBuff` (`src/td/score.h:225`), a third page `new`-ed in
  `src/td/ending.cc:177` and `delete`-d at `:270`.
- RA's `winbits` (29 call sites, none of which set the page) and `src/ra/winbits_test.cc:30-49`,
  whose `TestScreen` RAII guard becomes a plain view.
- `ChronalVortexClass::Render` (`src/ra/vortex.cc:823-953`) and its own `RenderBuffer`, the only
  non-screen `SetLogicPage` target in RA.
- The two simulation-path sets, `TechnoClass::Electric_Zap` (`src/ra/techno.cc:3139`, virtual,
  called from `src/ra/vortex.cc:686`) and `BuildingClass::Fire_At` (`src/td/building.cc:1087`). Both
  reach into the visible page mid-frame and never restore, and game logic has no view in hand to
  convert into a parameter. They name `TheScreen().visible_view()` at the point of use, which is
  exactly the page they set today.
- The modem and network paths: `src/ra/nullmgr.cc:1058,1300,1498`, `src/ra/netdlg.cc:5810,5830`,
  `src/td/netdlg.cc:4378,4401`, and `src/td/internet.cc:498,506,542`.

### 7. Delete

Remove `LogicPage` (`src/sdllib/pixel_buffer.cc:58`), both `SetLogicPage` overloads (`:66,71`), the
reset in `~PixelView` (`:60-64`) and the declarations at `src/sdllib/pixel_buffer.h:76-83`. The two
`pixel_buffer_test.cc` cases that assert on the global (`:165-183`) go with it.

## Constraints

- **Lock nesting crosses translation units.** `PowerClass::Draw_It` takes `LogicPage->Lock()`
  (`src/ra/power.cc:168`) and drops it at `:246`, and in between `CC_Draw_Shape` builds a second
  view on `LogicPage->buffer()`. The same shape appears at `src/ra/tab.cc:121`,
  `src/ra/sidebar.cc:756`, `src/ra/radar.cc:452,520`, `src/ra/help.cc:274`, `src/ra/egos.cc:779` and
  `src/ra/mapeddlg.cc:1158,1256`. Threading is safe because `lock_count_` belongs to the
  `PixelBuffer`, not the view (`src/sdllib/pixel_buffer.h:268`) -- but only while the view passed
  down is a view of the same buffer. Getting this wrong leaks a lock rather than failing a test.
- **Save/restore pairs are not exception-safe.** Only `src/ra/winbits_test.cc:39` is RAII. Every
  production pair is a bare local and a manual restore; `src/ra/installation.cc:262-294` already
  duplicates its restore to cover an early return, inside a loop where a future `continue` would
  leak. Phases that keep a pair alive should not add control flow between set and restore.
- **TD's page set is four-valued** (`visible_view`, `hidden_view`, `sys_mem_page`, `PseudoSeenBuff`)
  where RA's is effectively three (`visible_view`, `hidden_view`, `vortex`'s `RenderBuffer`).
- **The two games diverge** enough that most phases need separate patches: TD has no `bar.cc`, no
  `TListClass`, no `winbits`, no `vortex`, and its `CC_Draw_Shape` lives in `conquer.cc`.

## Verification (per phase)

- Both build directories clean, including the strict one; `tools/strict_tu.py` during the edit loop.
- Full test suite.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh`.
- ASan `-NEWGAME -QUITFRAME` in both games, which also confirms phase 0's window and renderer
  teardown.
- Phases 0, 3a, 3b and 4 additionally need a real-display run into a mission and through a movie.
  Headless never loads palettes, so a rendering regression there is invisible to the smoke scripts.

## Progress

- 2026-09-21: phase 0 done for both games. `Display` (`src/sdllib/display.h/.cc`) owns the window,
  the renderer, the redraw event, the present cadence and the window page, and is a `Game` member
  declared before `Screen`. `WindowBuffer`, `MainWindow`, `SDLRenderer` and `ForceRenderEventID` are
  gone, as is the self-registration in `PixelBuffer::Init()`; `Screen::Init()` attaches its visible
  page and `~PixelBuffer` detaches one that dies attached. `Set_Video_Mode()` and `VQA_OpenAudio()`
  lost the window parameters they ignored, which ended winvq's link seam on `MainWindow`. Deviations
  from the plan above, corrected in it: `wolapiob.cc` is built after all and needed two more methods
  (`Restore()`, `HasInputFocus()`); `renderer()` had to stay public for `pixel_buffer.cc`; and
  `SetPalette()` needed its own translation unit to keep the mouse out of the tests that stub it.

  Verification: both build dirs clean, 689 tests pass (5 of them new `DisplayTest` cases), both
  save/load smoke scripts pass, and ASan `-NEWGAME -QUITFRAME` reports no memory errors in either
  game. Red Alert's leak total is byte-identical to a pre-change baseline (29,199 bytes in 68
  allocations, all `MapEditClass::One_Time()`); Tiberian Dawn's 215 allocations are the
  `HouseClass::Init_Trackers()` leaks phase 6 of the globals plan recorded. Still to do: run Red
  Alert on a real display into a mission and through a movie.

- 2026-09-21: phase 1 done. `src/ra/bar.*` deleted (`ProgressBarClass` had no caller and compiled
  only because the directory is globbed), and the 35 commented-out `LogicPage->` fragments in both
  games are gone, including the three `if (display /*&& LogicPage->Lock()*/)` conditions.

  `src/td/radar.cc` now restores the page it borrows: `oldpage` is a plain `PixelView*` again and
  `SetLogicPage(oldpage)` runs after the blit, matching `src/ra/radar.cc:548`. The unbalanced
  `LogicPage->Unlock()` that the commented-out `Lock()` left behind is harmless --
  `PixelBuffer::UnlockSurface()` returns early when `lock_count_` is zero -- and stays until phase
  3a threads the view through.

  The `"C&C95 - Logic page invalid"` repair code in `src/td/netdlg.cc` is deleted, and the leak it
  compensated for is fixed at the source: `Map_Selection()` (`src/td/mapsel.cc`) sets the page to
  `sys_mem_page()` to draw the country shape and then falls into the same teardown block both score
  presentations use -- except that `ScoreClass::Presentation()` (`src/td/score.cc:1111`) and
  `Multi_Score_Presentation()` (`:2283`) each call `SetLogicPage(TheScreen().visible_view())` right
  before deleting `PseudoSeenBuff`, and `Map_Selection()` did not. It returned to the main menu with
  the global still on the system memory page, which is the third page the netdlg guard tested for;
  the same omission was one `delete` away from the latent crash noted above. `Map_Selection()` now
  restores the visible view before the deletes.

  Verification: both build dirs clean, `tools/strict_tu.py` clean on all 18 touched TUs, 689 tests
  pass, both save/load smoke scripts pass, and ASan `-NEWGAME -QUITFRAME` reports no memory errors
  in either game with leak totals unchanged from the phase 0 baseline (RA 29,199 bytes in 68
  allocations, TD 215 allocations). Still to do: a real-display run of the TD radar full redraw,
  which is the behaviour the restore changes and which headless cannot exercise.

- 2026-09-21: phase 2 done. The leaf primitives take a leading `PixelView& view` and read it instead
  of the global: `CC_Draw_Shape`, `Simple_Text_Print` and its `Fancy_Text_Print` /
  `Conquer_Clip_Text_Print` / `Plain_Text_Print` wrappers, `Draw_Box`, `Draw_Beveled_Box`,
  `Draw_Caption`, `Window_Box`, TD's `CC_Texture_Fill` and `Fat_Put_Pixel`, and RA's `LockedWindow`
  with `SaveSurfaceRect`, `RestoreSurfaceRect`, `DrawDib` and `DrawDibIfLoaded`. 960 call sites now
  pass `*LogicPage` explicitly; the hidden reads inside the primitives are gone.
  `TheScreen() .IsVisible(LogicPage)` inside `Window_Box` and TD's `Draw_Box` became
  `IsVisible(&view)`.

  Two deviations from the plan above. RA's `Dialog_Box` gained no parameter: it always draws to the
  hidden page and blits forward, so it names `TheScreen().hidden_view()` locally and its
  `SetLogicPage` save/restore pair is deleted -- the endpoint rule rather than a threaded argument,
  and its 53 call sites are untouched. TD's `Dialog_Box` is a one-line forward to `Draw_Box` and did
  take the parameter. `Fat_Put_Pixel` already had a `PixelView&`, as its last parameter; it moved to
  the front to match the rest.

  The sweep was scripted (insert `*LogicPage, ` after the call's open paren, skipping comment lines
  and any site whose first argument already reads `PixelView`), then compiler-checked. The strict
  build caught two classes of fallout the plain build did not: `misc-include-cleaner` wanted
  `sdllib/pixel_buffer.h` included directly in the 39 files that now name `LogicPage` themselves,
  and `misc-unused-parameters` caught the forwarding calls _inside_ `Conquer_Clip_Text_Print` and
  `Plain_Text_Print` that the script had pointed back at the global instead of at their own `view`.

  Verification: both build dirs clean, 689 tests pass, both save/load smoke scripts pass, and ASan
  `-NEWGAMESCG01EA -QUITFRAME100` reports no memory errors in either game. Red Alert's leak total is
  byte-identical to a stash-and-rebuild baseline of the same commit (29,823 bytes in 73 allocations:
  `MapEditClass::One_Time()` plus the known `DimensionData` cache); Tiberian Dawn's 215 allocations
  match the phase 0/1 figure. Still to do: a real-display run, which phases 3a, 3b and 4 need as
  well.

- 2026-09-21: phase 3a done. `GScreenClass::Draw_It(bool)` is `Draw_It(PixelView&, bool)` through
  all 7 overrides in each game, and the satellites below it take the view too:
  `DisplayClass::Redraw_Shadow` (and TD's `Redraw_Shadow_Rects`),
  `SidebarClass::StripClass::Draw_It`, `TabClass::Draw_Credits_Tab` and `Hilite_Tab`, and the
  radar's `Plot_Radar_Pixel`, `Cursor_Cell`, `Mark_Radar`, `Radar_Cursor`, `Radar_Anim`,
  `Render_Terrain`, `Render_Infantry`, `Render_Overlay`, `Draw_Names` and RA's `Draw_House_Info`.
  RA's `MessageListClass::Draw` takes one as well. `GScreenClass::Render()` names the hidden view
  once and passes it down; `LogicPage->` reads are down from 127 to 88 in RA and 133 to 102 in TD.

  `Cursor_Cell` and `Mark_Radar` were not in the plan's satellite list but had to join it:
  `Radar_Cursor` reaches `Plot_Radar_Pixel` through them.

  Four of the seven `SetLogicPage` save/restore pairs in the chain are gone -- both radar helpers in
  each game, plus TD's `RadarClass::Draw_It`, whose `oldpage == &TheScreen().visible_view()` test
  became `TheScreen().IsVisible(&view)` and whose stray unbalanced `Unlock()` (noted in phase 1)
  went with it. Three pairs stay, against the plan's expectation that `Render()`'s would go here:
  both `Render()`s and RA's `RadarClass::Draw_It` still call `GadgetClass::Draw_All` or `Draw_Me`,
  which find their page through the global until phase 4. Each carries a comment saying so. Removing
  them now would have left those gadget draws pointing at whatever page the caller happened to leave
  behind.

  The strict build again caught what the plain build did not: the empty `GScreenClass::Draw_It` base
  and TD's `MessageListClass::Draw` had an unused `view`. The base is now `PixelView& /*view*/`;
  TD's `Draw()` gave the parameter back, since unlike RA's it only forwards to `Draw_All()` and has
  nothing of its own to draw. It gets one in phase 4.

  Verification: both build dirs clean, 689 tests pass, both save/load smoke scripts pass, and ASan
  `-NEWGAMESCG01EA -QUITFRAME100` reports no memory errors and leak totals byte-identical to the
  phase 2 run in both games (RA 29,823 bytes in 73 allocations, TD 17,272 in 215). The real-display
  run is still outstanding, and now covers phases 0 through 3a.

- 2026-09-21: phase 3b done. The map object chain takes the view: `CellClass::Draw_It`,
  `ObjectClass::Draw_It` and its 12 overrides (RA's `const`, TD's not, so the two games needed
  separate patches), `ObjectClass::Render(bool)` in both games rather than only TD's,
  `ObjectTypeClass::Display` with its 9 RA and 8 TD overrides, both
  `ObjectTypeClass::Draw_It(int, int, int)` declarations, and the satellites
  `TechnoClass::Techno_Draw_Object`, `TechnoClass::Draw_Pips` and RA's `AircraftClass::Draw_Rotors`.
  `LogicPage->` reads are down from 88 to 78 in RA and 102 to 90 in TD.

  `DisplayClass::Redraw_Icons` and RA's `Redraw_OIcons` joined the list: they sit between
  `DisplayClass::Draw_It` and `CellClass::Draw_It` and had no page argument. RA's `Render(bool)` was
  not in the plan, which named only TD's, but it reaches `Draw_It` the same way.

  `TechnoClass::Electric_Zap` and `ChronalVortexClass::Render` keep reading the global, as phase 6
  says; the vortex's `SmudgeTypeClass::Draw_It` call passes `*LogicPage`, which is the
  `RenderBuffer` the vortex set. The map editor's two `Display()` call sites in each game pass
  `*LogicPage` too.

  The strict build wanted `sdllib/pixel_buffer.h` included directly in 53 more files, and flagged
  two `(view)` parentheses the mechanical rewrite left behind in `cdata.cc`.

  Verification: both build dirs clean, 689 tests pass, both save/load smoke scripts pass, and ASan
  leak totals are byte-identical to phases 2 and 3a in both games (RA 29,823 bytes in 73
  allocations, TD 17,272 in 215) with no memory errors. The real-display run now covers phases 0
  through 3b.

- 2026-09-21: phase 4 done. The gadget hierarchy takes the view:
  `GadgetClass::Draw_Me(PixelView&, bool)` through all 14 RA and 11 TD overrides,
  `GadgetClass::Draw_All`, `GadgetClass::Input`, `ListClass::Draw_Entry` with its 5 RA and 3 TD
  overrides, and the non-virtual leaves `Draw_Background`, `Draw_Text` and `Draw_Thumb`. RA's
  duck-typed `Draw_It` protocol at `src/ra/list.h:492` and its 7 implementations changed in lockstep
  with the template. The 19 RA / 16 TD `TheScreen().IsVisible(LogicPage)` predicates are now
  `IsVisible(&view)`, and `LogicPage->` reads are down from 78 to 41 in RA and 90 to 62 in TD.

  Three leaves were not in the plan's list and had to join it. `Draw_Text` and `Draw_Thumb` are
  siblings of `Draw_Background` -- leaf drawing helpers a `Draw_Me` calls -- and `EditClass`'s
  `Draw_Text` is virtual, so `WOLEditClass`, `PassEditClass` and `PWEditClass` moved with it.

  All three save/restore pairs the render chain still carried are gone: both `GScreenClass::Input`s,
  which existed only so the gadget draws landed on the hidden page and now pass
  `TheScreen().hidden_view()` directly, both `GScreenClass::Render`s, and RA's
  `RadarClass::Draw_It`, whose `oldpage == &TheScreen().visible_view()` test became
  `TheScreen().IsVisible(&view)` as TD's did in phase 3a.

  Removing `Render()`'s pair exposed two readers below it that the global had been feeding
  implicitly, both fixed here rather than left to phase 6: `ChronalVortexClass::Render`, which
  builds its blit target from `LogicPage->buffer()` (`src/ra/vortex.cc:953`) and is called from
  `DisplayClass::Draw_It`, and TD's `MessageListClass::Draw`, which phase 3a had deliberately left
  without a parameter. Both now take the view. The vortex's own `RenderBuffer` save/restore pair is
  self-contained and stays for phase 6. The remaining
  `Fancy_Text_Print(*LogicPage, TXT_NONE, 0, 0, ...)` calls in the draw path set the font for a
  following `String_Pixel_Width` and draw nothing, so they are page-independent and were left alone.

  The sweep was scripted the way phase 2's was, and two scripting hazards are worth recording. A
  brace-depth tracker that decides which function bodies have a `view` in scope must not end the
  body at the first nested block when the scope came from a local `PixelView& view = ...`, must not
  end it on a multi-line signature before the body opens, and must not start one on a declaration
  that ends in `;` -- each mistake silently leaves correct-looking `*LogicPage` behind or leaks
  `view` into the next function. Skipping comment lines by their first character also misses
  continuation lines inside `/* ... */` blocks; two in `src/td/netdlg.cc` and one in
  `src/ra/wol_logn.cc` had to be reverted by hand.

  Verification: both build dirs clean, 689 tests pass, both save/load smoke scripts pass (RA 240
  object positions, TD 5,742 game states identical across save/load), and ASan
  `-NEWGAMESCG01EA -QUITFRAME100` reports no memory errors with leak totals byte-identical to phases
  2, 3a and 3b in both games (RA 29,823 bytes in 73 allocations, TD 17,272 in 215). The real-display
  run now covers phases 0 through 4.

- 2026-09-21: phase 5 done for both games. Every modal dialog that set the page one way now names
  the view it draws to, and so does every dialog that silently inherited one: `Net_Join_Dialog`,
  `Net_New_Dialog`, both `Net_Fake_*_Dialog`s and the nine null-modem dialogs never set the page at
  all, they read whatever their caller left behind. About 340 reads in Red Alert and 400 in Tiberian
  Dawn became a local `PixelView& view = TheScreen().visible_view();` and the argument the leaf
  primitives have taken since phase 2. Both `Main_Menu`s name two pages, since they draw the menu to
  the hidden page and blit it forward; RA's `Show_Who_Was_Responsible` names the hidden view. Nine
  helpers below these dialogs took a leading `PixelView&` instead: `Flash_Line`, `Setup_Menu`,
  `Check_Menu`, `MapEditClass::Draw_Member`, `CreditClass::Graphic_Logic`, `EgoClass::Render` and
  `Wipe`, and `ToolTipClass::Show`, `Unshow` and `Move`.

  Three things the plan did not anticipate, all of them now done:
  - **The plan's own phase 5 count was short.** It listed 54 RA and 61 TD one-way sets, but the
    dialogs that _inherit_ a page are the same work and were not counted; `src/ra/nulldlg.cc` alone
    has five of them, and TD's `Net_Fake_New_Dialog` and `Net_Fake_Join_Dialog` were missed by the
    scripted survey entirely -- `#ifdef VIRTUAL_SUBNET_SERVER` opens a brace in both branches, so a
    brace-depth scan of `src/td/netdlg.cc` never closes another function after line 4810. A sweep
    like this needs two assertions, not better parsing: the scan must end a file at depth zero, and
    every mention must fall inside exactly one function it emitted. `src/td/netdlg.cc` was the only
    file in the tree that failed the first.
  - **Eighteen calls needed a font, not a page** (c59f78d7).
    `Fancy_Text_Print(view, TXT_NONE, 0, 0, ...)` draws nothing -- `Simple_Text_Print` picks the
    font, spacing and palette before it looks at the text and only draws inside `if (text && *text)`
    -- so these calls existed to set up the `String_Pixel_Width` on the next line. Most are in
    gadget constructors, which run before a dialog has a page at all. That half of
    `Simple_Text_Print` is now `Select_Text_Font()`, which takes no view. Without it, removing the
    last sets would have left those constructors dereferencing a global nothing sets.
  - **`src/ra/confdlg.*` and `src/ra/descdlg.*` are deleted** (97a17d47). `src/ra/CMakeLists.txt`
    excludes all four, so they have never been compiled by this build, and nothing refers to
    `ConfirmationClass` or `DescriptionClass` in `src/`. The `ra/bar.cc` case from phase 1, only
    clearer.

  Deviations worth recording: phase 4's sweep had left nine `Draw_Entry` / `Draw_It` bodies drawing
  to `view` in one branch and to the global in the other, fixed first in 40c620dc;
  `ToolTipClass::Show` and `Unshow` pulled two of winbits' 29 call sites forward out of phase 6; and
  `WOL_GameSetupDialog::ProcessGuestRequest` and `ClearAllAccepts` name the visible view locally
  rather than take a parameter, which would have cascaded into `rawolapi.cc`.

  Verification: both build dirs clean, 692 tests pass, both save/load smoke scripts pass (RA 240
  object positions, TD 5,742 game states identical across save/load), and ASan
  `-NEWGAMESCG01EA -QUITFRAME100` reports no memory errors with leak totals byte-identical to phases
  2 through 4 in both games (RA 29,823 bytes in 73 allocations, TD 17,272 in 215). The real-display
  run now covers phases 0 through 5 and is still outstanding.

  What is left for phase 6 is exactly what the plan named, plus two items it did not: the five
  `LogicPage` reads in `src/td/debug.cc` sit inside an `#ifdef NEVER` block that compiles in no
  configuration, and `src/td/findpath.cc`'s debug path holds a save/restore pair of its own.

- 2026-09-21: phase 6 done for both games. No file outside `src/sdllib/pixel_buffer.*` mentions
  `LogicPage`. The residue divided into five kinds, and most of it turned out to be ceremony rather
  than work:
  - **The score screens.** Both games' `ScoreTimeClass::Update` and `ScoreCredsClass::Update` saved
    and restored the page because a generic animation tick calls them, but each then set it
    unconditionally -- RA's to the visible view, TD's to `PseudoSeenBuff` -- so the drawn page never
    depended on what came in. Once those name their view, every set in `Presentation` and
    `Multi_Score_Presentation` had no reader left: of the four `Update` implementations the tick
    loop reaches, the other two already named their page explicitly. TD's `Draw_InfantryMan` and
    `Draw_Bar_Graphs` took a `PixelView&` from the callers that used to set the page for them; which
    of the four pages each draw lands on was established from the blit that follows it, not assumed.
  - **The modem callbacks.** `NullModemClass::Abort_Modem` is the one place a parameter was
    impossible: it is installed through `Set_Abort_Function()`, whose type is `int (*)()`
    (`sdllib/wincomm.h:182`). It names the visible view locally, which is the page its two
    installers have on screen whenever it can fire. (Both implementations of `Set_Abort_Function`
    discard the pointer, so the callback is registered and never invoked -- worth its own dead-code
    pass.)
  - **The simulation path.** `TechnoClass::Electric_Zap` and `BuildingClass::Fire_At` name
    `TheScreen().visible_view()` at the point of use, as the plan prescribed.
  - **The save/restore pairs.** Neither `Force_CD_Available` leaked: the duplicated restore covering
    the early return was doing its job. Both needed no replacement local at all, because their only
    draw is a message box that has named its own page since phase 5. TD's `Map_Selection` lost the
    teardown restore that phase 1 added (b6ea2927): it existed so `~PixelView` would not leave the
    global dangling when `PseudoSeenBuff` died, and with no global that leak is structurally
    impossible rather than repaired. The same applies to the two sets TD's score screen ran
    immediately before `delete PseudoSeenBuff`.
  - **Dead code, deleted rather than converted.** `src/td/debug.cc` lines 341-514, an `#ifdef NEVER`
    block around a dead `case KN_F2:` with three more `#ifdef NEVER` blocks nested inside it, and a
    fourth in `src/td/findpath.cc` referencing a `Debug_ShowPath` that exists in neither game.
    `NEVER` is defined nowhere in the tree.

  `PseudoSeenBuff` and the vortex's `RenderBuffer` remain globals. They are pages, not _the_ logic
  page; phase 6 only stopped them being `SetLogicPage` targets, and giving them owners is a separate
  job. One real bug surfaced: `FootClass::Find_Path`'s two debug blocks read the global _after_
  `Debug_Draw_Map` had restored it, so the path visualiser drew to whatever the simulation happened
  to leave behind and worked by accident.

  Verification: both build dirs clean, 692 tests pass, both save/load smoke scripts pass, and ASan
  leak totals are byte-identical to phases 2 through 5 in both games (RA 29,823 bytes in 73
  allocations, TD 17,272 in 215) with no memory errors.

- 2026-09-21: phase 7 done. `LogicPage`, both `SetLogicPage` overloads and the reset in `~PixelView`
  are deleted, along with the two `pixel_buffer_test.cc` cases that asserted on the global.
  `~PixelView` existed only for that reset and is now `= default` in the header, kept declared so
  `cppcoreguidelines-special-member-functions` stays satisfied next to the deleted copy and move
  operations.

  The deletion is the proof phase 6 was complete: the whole tree builds without the global, so no
  reader was missed. 690 tests pass (688 of the old 692, less the two deleted), both build dirs are
  clean, both save/load smoke scripts pass, and ASan leak totals are byte-identical to every phase
  since 2 (RA 29,823 bytes in 73 allocations, TD 17,272 in 215).

  The plan is complete. Two follow-ups it created rather than closed: `PseudoSeenBuff` (TD) and
  `RenderBuffer` (RA's vortex) are still globals -- pages that want owners, in the manner of phase
  0's `Display` -- and `NullModemClass::Abort_Modem` is dead in both games, since both
  implementations of `Set_Abort_Function` discard the pointer they are given. The real-display run
  covering phases 0 through 7 is still outstanding; headless never loads palettes, so no rendering
  regression in any of this work would have shown up in the checks above.
