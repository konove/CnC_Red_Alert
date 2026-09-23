# Remove the Font Globals

## Findings

Measured on the tree at 16b44240.

`src/sdllib/font.h` still exports six mutable globals — `g_font`, `g_font_x_spacing`,
`g_font_y_spacing`, `g_font_max_width`, `g_font_max_height`, `g_font_palette` — plus the setters
`SetFont()` / `SetFontPalette()`. Every print and every measurement reads "whatever font the last
code selected", which makes layout depend on drawing order (list line heights measured before the
list picks its font, the RA briefing spacing inherited from the last button, ~48 select-only
`Fancy_Text_Print(TXT_NONE, …)` / `Select_Text_Font` calls that exist only to set the font before
measuring) and has already produced a leak bug (`td/sidebar.cc:398-400` restores the x spacing and
then overwrites it with -1). GLOBALS_PLAN never covered them.

**Decision (user): fully explicit — no "current font" state anywhere at the end.** A font is a value
(`FontStyle`) that each print and measurement receives. This follows the `LogicPage` precedent in
`docs/LOGIC_PAGE_PLAN.md` (delete a global by threading a parameter), not the `Installed<T>`
subsystem route.

Census facts the plan relies on (verified):

- Main thread only; nothing serialized; no save-format impact.
- One central writer per game: `Select_Text_Font` (`src/ra/dialog.cc:402`, overload `:921`;
  `src/td/dialog.cc:359`) writes all four at `ra :631-634` / `td :590-593`. It always resets
  `xspace = 1; yspace = 0` (ra `:468`, td `:453`); the only carried-over state is the font data for
  `TPF_LASTPOINT` (`font = g_font`, ra `:552`, td `:515`). No literal flag lacks a point size; only
  flags held in variables could.
- Bypass writers (save/restore or palette effects): ra/td `score.cc`, `ra/scenario.cc:1456-1478`,
  `ra/iconlist.cc:204-217`, `ra/installation.cc:261-288`, `ra/tooltip.cc`, `ra/cell.cc:1101/1110`,
  `td/cell.cc:944/950`, `td/sidebar.cc:326-400`, `td/mapsel.cc`, `td/intro.cc`, `td/ending.cc`,
  `td/conquer.cc:3107-3139`, and the initial `SetFont(k8Point)` in `ra/assets.cc:51` /
  `td/assets.cc:58`.
- Readers: `g_font_max_height` ~59 sites of layout code, `g_font_max_width` only `menus.cc`,
  `StringPixelWidth` ~114, `CharPixelWidth` ~16, identity checks `ra/special.cc:300`,
  `td/edit.cc:348`. Direct `PixelView::Print` callers: 10 RA, 27 TD (score screens, briefing).

## Plan

### Ordering principle

Move **readers first, writers last**. `Select_Text_Font` and the bypass writers keep updating the
globals until every reader is explicit, so each commit is behaviour-identical and the build stays
green; a single deletion phase ends it. While both exist, a temporary runtime check compares each
explicit style with the globals and logs mismatches — the only practical way to find sites that
silently depend on a font chosen by earlier drawing.

### Phase 0 (optional, 1 commit) — headless page dump

`-DUMPFRAME<n>` in both games writes `visible_view()` as PCX at frame n, reusing `Write_PCX_File` as
`src/ra/conquer.cc:625` does. Pages hold palette indices even under `SDL_VIDEODRIVER=dummy`, so
`cmp` of dumps is an exact before/after pixel diff for in-game text (sidebar, radar names,
messages). Menus still need the real-display walkthrough below.

### Phase 1 — sdllib value type (2 commits)

**1a.** In `src/sdllib/font.{h,cc}`:

```cpp
struct FontStyle {
  std::span<const std::byte> font;   // MIX-owned (TheAssets().font(...))
  int x_spacing = 0;
  int y_spacing = 0;
  std::array<uint8_t, 16> palette = kIdentityFontPalette;
  int max_height() const;  int max_width() const;   // via FontView
  int line_height() const { return max_height() + y_spacing; }
};
int CharPixelWidth(const FontStyle& style, char character);
int StringPixelWidth(const FontStyle& style, const char* text);
```

- `PixelView::Print` / `PrintLocked` overloads taking `const FontStyle&` first (both the `text` and
  the `int value` forms, `src/sdllib/pixel_buffer.h:204-207`, `.cc:706`). Fore/back go into a
  **local** palette copy; nothing shared is written.
- The legacy API becomes wrappers that build a `FontStyle` from the globals and forward (one drawing
  loop). Legacy `Print` keeps writing `g_font_palette[0..1]` for exactness.
- Reuse `FontView`, `Slice()`, `Advance()` already in `font.{h,cc}`. Keep `font.cc` /
  `pixel_buffer.cc` free of `ww_mouse` / `tech` references (tests rely on that).
- Tests in `src/sdllib/font_test.cc`: old and new API give identical buffers and widths on the
  synthetic font.

**1b.** Temporary check: the explicit functions take
`std::source_location loc = std::source_location::current()`; compare the style with the globals
(font, spacings, palette entries 2-15 for Print) and `LOG(ERROR)` once per location on mismatch
(`LOG`, not `DLOG`, since RelWithDebInfo compiles DLOG out).

### Phase 2 — split `Select_Text_Font` (RA, then TD: 2 commits)

In `src/ra/dialog.{h,cc}` and `src/td/dialog.{h,cc}`:

- `FontStyle TextFontStyle(TextPrintType flags)` — font and spacing only (point and shadow bits,
  after the `TPF_3POINT` shadow fixup). No palette, so widget constructors can call it without
  `ThePalettes()`.
- `TextStyle TextStyleFor(flags, fore, back)` — extends today's `TextStyle {flag, forecolor}`
  (`ra/dialog.h:76-79`, `td/dialog.h:50`) with `FontStyle font`, palette built exactly as today.
- `Select_Text_Font` = `TextStyleFor` + copy into the globals (temporary).
- `Simple_Text_Print` draws with `view.Print(style.font, …)` and centres with
  `StringPixelWidth(style.font, …)`; `Conquer_Clip_Text_Print` measures with its style.
  `Format_Window_String(_New)` gains a `const FontStyle&` overload.
- `TPF_LASTPOINT`: still `font = g_font`, plus a `LOG(ERROR)` naming the flag and current `FontType`
  so runtime tells whether it is ever reached.

### Phase 3 — move readers (≈10 commits, one game per commit, green each time)

Each site derives its style from flags it already stores; nothing cached (`TextFontStyle` is a
switch plus an asset lookup).

| Commit                          | Scope                                                                                                                 | Pattern                                                                                                                                                                         |
| ------------------------------- | --------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 3a RA widgets                   | textbtn, statbtn, list.cc/list.h, edit, drop, iconlist, woledit, special, msglist, tooltip                            | `LineHeight(TextFontStyle(flags).line_height() - 1)`; iconlist TPF_TYPE uses a local copy with `x_spacing = -2`; PWEdit identity check compares `TextFontStyle(TextFlags).font` |
| 3b TD widgets                   | textbtn, edit, list, msglist                                                                                          | same                                                                                                                                                                            |
| 3c RA dialogs                   | msgbox, nulldlg, nullmgr, netdlg, menus (`max_width()`), goptions, wol\_\*, mplayer, egos, seditdlg, help, display, … | select-only calls become `const FontStyle font = TextFontStyle(kTpf…);` beside the measurements                                                                                 |
| 3d TD dialogs                   | msgbox, nulldlg, nullmgr, netdlg, menus, goptions, confdlg, mplayer, help, display, …                                 | same                                                                                                                                                                            |
| 3e in-game                      | ra/td cell.cc, td/sidebar.cc widths, radar                                                                            | measure/print with the explicit style                                                                                                                                           |
| 3f RA score/briefing            | score.cc, scenario.cc:1456-1478, installation.cc                                                                      | `ScoreAnimClass` family holds a score-font `FontStyle` (x spacing 0); `SetFontPalette(p) + Print` → `Print(WithPalette(score, p), …)`; briefing picks its font explicitly       |
| 3g TD score/mapsel/intro/ending | score.cc, mapsel.cc, intro.cc, ending.cc                                                                              | same                                                                                                                                                                            |

Per commit: `tools/strict_tu.py` while editing, plain build + ctest, headless runs with zero check
lines.

### Phase 4 — delete (3-4 commits)

- **4a RA:** drop the global writes from `Select_Text_Font` (fold into `TextStyleFor`), delete the
  `SetFont`/`SetFontPalette`/spacing writes in score, iconlist, cell, tooltip, installation and
  `ra/assets.cc:51`. Save/restore locals disappear (they only protected globals).
- **4b TD:** same, incl. sidebar, `td/conquer.cc:3107-3139` palette save/restore, mapsel, intro,
  ending, score, `td/assets.cc:58`.
- **4c `TPF_LASTPOINT`:** replace `font = g_font` with `TheAssets().font(FontType::k8Point)` (what
  Assets installs at startup) — own commit so it reverts alone; or delete the enumerator if phase 2
  logging proved it unreachable.
- **4d sdllib:** delete the six globals, `SetFont`, `SetFontPalette`, legacy wrappers, the checks
  and the `source_location` parameters; replace `SetFontTest`/`SetFontPaletteTest` with `FontStyle`
  tests; `grep -rE 'g_font|SetFont\b|SetFontPalette' src` is empty (outside unbuilt `winvq/vqm32`,
  `winvq/vplay32` under `#if CAPTIONS`).

### Behaviour changes to flag (each its own commit or left as-is)

1. `TPF_LASTPOINT` fixed to 8-point (no visible change if phase-2 logging never fires with another
   font current).
2. TD sidebar spacing leak disappears (unobservable: later prints re-select spacing; checks
   confirm).
3. `ra/edit.cc:79`, `td/edit.cc:128` count x spacing twice — **keep** as
   `CharPixelWidth(font, 'X') + font.x_spacing` with a comment; fixing it resizes auto-sized edits.
4. `cell.cc` `±2` spacing is a no-op overwritten by the next select — delete; pixels identical.
5. List line heights and edit identity checks follow the widget's own flags; if a check fires,
   decide per site.

## Verification

- `cmake --build build --parallel 22 && ctest --test-dir build`; `tools/strict_tu.py` during edits;
  full `cmake --build build-strict --parallel 14` before each phase's last commit.
- `tools/ra_saveload_smoke.sh`, `tools/td_saveload_smoke.sh`; ASan `-NEWGAME … -QUITFRAME`.
- `-DUMPFRAME` PCX identical to the pre-phase baseline; zero check-log lines in headless runs.
- **Before phase 4:** one real-display walkthrough per game with checks live — main menu, options,
  load/save, skirmish/network setup, message box, edit box, briefing, score screen, TD map select,
  RA help/tooltips. Any logged mismatch is a site that depended on earlier drawing; resolve it
  first.

## Risks

- Checks only cover paths actually run; walkthrough coverage bounds the proof.
- `Conquer_Clip_Text_Print` measures and `Simple_Text_Print` draws — both must derive from the same
  flags.
- `ra/installation.cc` has early returns inside a loop between save and restore — don't add control
  flow there before 4a removes the pair.
- Legacy `Print` can only go after all 37 direct callers are converted (3f/3g).

## Progress

- **2026-09-23: plan written** (c172fab1).

- **2026-09-23: phases 1 and 2 done** (0fe7d3f0..96e30c30). `FontStyle` holds a `FontView`, the two
  spacings and the glyph palette; the Google struct rule keeps behaviour out of it, so the line
  height is the free function `FontLineHeight()` rather than a member, and max height and width are
  read through `style.font` (`FontView` gained a default constructor and `data()`). `TextStyle`
  names its `FontStyle` member `font_style`, since `style.font.font` read badly. `TextFontStyle()`,
  `TextStyleFor()` and a shim `Select_Text_Font()` exist in both games, and `Simple_Text_Print()`
  draws with the explicit style.

  Phase 0 (the page dump) was skipped for now: the new tests compare the legacy and explicit paths
  pixel for pixel, and the runtime check covers the rest. Headless SCG01EA runs to frame 200 in both
  games log no mismatch and no flag without a point size. The strict build caught one thing
  `tools/strict_tu.py` did not: `TextStyle` needed initializers for its other two fields once it
  gained a member with its own.
