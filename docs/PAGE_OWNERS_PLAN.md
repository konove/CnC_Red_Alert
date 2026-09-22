# Owners for the Last Loose Pages

`docs/LOGIC_PAGE_PLAN.md` finished on 2026-09-21 and closed with two follow-ups it created rather
than solved: "`PseudoSeenBuff` (TD) and `RenderBuffer` (RA's vortex) are still globals -- pages that
want owners, in the manner of phase 0's `Display`." This plan is those two.

They turn out to be different sizes, and the note above is wrong about one of them.

## Findings

Measured on the tree at 7d9f4b4b.

### RA: `ChronalVortexClass::RenderBuffer` is not a global

It is a member of `ChronalVortexClass` (`ra/vortex.h:280`), which is itself a member of `World`
(`ra/world.h:203`), so it already has the owner the note asked for. What it is is a raw owning
pointer with a hand-written lifetime:

| site            | what it does                                           |
| --------------- | ------------------------------------------------------ |
| `vortex.cc:815` | `if (!RenderBuffer) RenderBuffer = new PixelBuffer(…)` |
| `vortex.cc:140` | `delete` in the destructor                             |
| `vortex.cc:316` | `delete` + null in `Serialize()` when reading          |
| 11 more         | `RenderBuffer->…`                                      |

The page is 96x96 -- `CELL_PIXEL_W * 4` by `CELL_PIXEL_H * 4`, 9,216 bytes -- and there is one
vortex in the game. The lazy allocation buys nothing and costs a null check, two `delete`s and a
reset branch in the save-game path.

### TD: three globals, one concern

`PseudoSeenBuff` is not alone. It travels with `TextPrintBuffer` and `BlitList`, created and
destroyed together at all five sites:

```cpp
PixelBuffer* PseudoSeenBuff;      // td/score.cc:153, declared td/score.h:225
PixelBuffer* TextPrintBuffer;     // td/score.cc:154, declared td/textblit.h:51
TextBlitClass BlitList;           // td/score.cc:297, declared td/textblit.h:52
```

They are one thing: the way Tiberian Dawn shows a full-screen presentation. Art is drawn at the
320x200 the art was made for into `PseudoSeenBuff` and scaled 2x onto the screen by
`Interpolate_2X_Scale()`; text is printed into `TextPrintBuffer` at the screen's own resolution so
captions stay crisp over the doubled pixels, and `BlitList` records which rectangles of it to bring
forward. `TextBlitClass::Update()` (`score.cc:620`) already reaches for `TextPrintBuffer` on its
own, which is the tell.

Five presentations own the trio, each `new`-ing all three and `delete`-ing them at the end:

| function                   | file           | lines      |
| -------------------------- | -------------- | ---------- |
| `ScoreClass::Presentation` | `td/score.cc`  | 702, 1100  |
| `Multi_Score_Presentation` | `td/score.cc`  | 2172, 2270 |
| `Map_Selection`            | `td/mapsel.cc` | 542, 1204  |
| `Choose_Side`              | `td/intro.cc`  | 128, 336   |
| `Nod_Ending`               | `td/ending.cc` | 177, 339   |

Live mentions: `PseudoSeenBuff` 107 (48 of them `->view()`), `TextPrintBuffer` 86 (56 `->view()`),
`BlitList` 47.

**Nothing leaks today.** No `return` sits between any `new` and its matching `delete` in the five
functions, so the manual pairs are correct -- this is a structural change, not a bug fix.

**The generic animation tick is why the state is global.** `Call_Back_Delay()` (`td/score.cc:1495`)
drives `Animate_Score_Objs()`, which walks `ScoreObjs` calling `ScoreAnimClass::Update()` -- a
no-argument virtual with five overrides (`ScoreCredsClass`, `ScoreTimeClass`, `ScorePrintClass`,
`MultiStagePrintClass`, `ScoreScaleClass`), each of which draws to the pages. That is the same shape
`LOGIC_PAGE_PLAN.md` phase 6 hit, and the reason a parameter never got threaded. `Call_Back_Delay`
has 72 call sites, all of them inside the four presentation files, so the thread is closed.

**Dead localization branches.** `#ifdef FRENCH` / `#ifdef GERMAN` blocks in `td/mapsel.cc` (12),
`td/nulldlg.cc` (17), `td/netdlg.cc` (14), `td/init.cc` (12) and ten more files are unreachable:
unlike `src/ra/CMakeLists.txt`, which has a real `RA_LANGUAGE` option, `src/td/CMakeLists.txt`
defines neither symbol in any configuration. Four of those blocks call drawing primitives on a
`PixelBuffer` and so no longer match the API after 7d9f4b4b. They are converted here alongside the
live code rather than deleted; whether Tiberian Dawn keeps unbuildable localization at all is its
own question. (Both localized Red Alert builds, `-DRA_LANGUAGE=german` and `=french`, were checked
and are clean.)

## Plan

### 1. `RenderBuffer` becomes a member by value

`PixelBuffer RenderBuffer` sized in `ChronalVortexClass`'s constructor, replacing the pointer. The
null check at `vortex.cc:815` and both `delete`s go. `Serialize()`'s reset becomes
`RenderBuffer.view().Clear()` -- the page is scratch and the original reallocation happened to zero
it, so clearing keeps a loaded game byte-identical rather than starting from the pre-save contents.

### 2. A `Presentation` class owns TD's three

New `src/td/presentation.h/.cc`. It absorbs `TextBlitClass` outright, since the blit list is only
meaningful with the text page it reads:

```cpp
class Presentation {
 public:
  Presentation();                     // sizes both pages
  PixelBuffer& page();                // 320x200, the art
  PixelBuffer& text_page();           // the screen's size, the captions
  void AddTextRect(int x, int y, int dest_x, int dest_y, int w, int h);
  void ClearTextRects();
  void DrawTextRects();               // blits the queued rectangles forward
};
```

Both pages are exposed as `PixelBuffer&` rather than `PixelView&`, so `Interpolate_2X_Scale()` and
`Bit_It_In()` -- which both take a `PixelBuffer*` -- need no change, and drawing reads
`show.page().view().Clear()` the way every other page in the tree does since 7d9f4b4b.
`src/td/textblit.h` and `src/td/textblit.cc` are deleted.

### 3. Thread it

Each of the five presentations builds a `Presentation` on the stack; the `new`/`delete` pairs and
the three globals go. A `Presentation&` threads down through `Call_Back_Delay()` (72 sites),
`Animate_Score_Objs()`, `ScoreAnimClass::Update()` and its five overrides, the `ScoreClass` helpers
(`Print_Minutes`, `Count_Up_Print`, `Show_Credits`, `Do_GDI_Graph`, `Do_Nod_Casualties_Graph`,
`Do_Nod_Buildings_Graph`, `Input_Name`, `ScoreDelay`, `Pulse_Bar_Graph`, `Print_Graph_Title`) and
the file statics in `score.cc` and `mapsel.cc` that draw (`Draw_Bar_Graphs`, `Draw_InfantryMan`,
`Animate_Cursor`, `Bit_It_In`'s callers).

This is the `LOGIC_PAGE_PLAN.md` mechanism: a leading `PixelView&`-style parameter threaded from the
leaves outward, compiler-enforced at every call site.

### 4. `TextBlitClass::Update`'s null check goes

`if (TextPrintBuffer && …)` exists because the global could be null. A `Presentation` that exists
has both pages, so `DrawTextRects()` just draws.

## Verification

- Both build directories clean, including the strict one; `tools/strict_tu.py` during the edit loop.
- Full test suite.
- `tools/ra_saveload_smoke.sh` (plus `--load-fixture`) and `tools/td_saveload_smoke.sh`.
- `-DRA_LANGUAGE=german` and `-DRA_LANGUAGE=french` still build.
- ASan `-NEWGAMESCG01EA -QUITFRAME100` in both games. Step 1 moves 9,216 bytes from a lazy
  allocation to a `World` member, so RA's allocation count changes by design; TD's should not move.
- A real-display run is required and cannot be skipped: every page here is a rendering path, and
  headless never loads palettes. Tiberian Dawn's score screen, map selection, side choice and
  ending, and Red Alert's chronal vortex, all have to be seen.

## Progress

- [ ] Step 1
- [ ] Steps 2-4
