# Plan: repaint the tactical view every frame

Written 2026-09-27, on the tree at 69f28795.

**Status: in progress.** Step 0 is c19e606f, step 1 34c350e9, step 2 602da368; step 3 needed no
code.

### Progress

- **Step 1 (RA full repaint): done.** `DisplayClass::Draw_It` repaints the whole view whenever it
  repaints at all; `RenderObjects()` draws each object once. Headless `-QUITFRAME600` logs match the
  old binary: SCU05EA line for line, SCG10EA apart from aircraft, which differ between two runs of
  the old binary too. SCG01EA is not usable for this check: two runs of the old binary already
  disagree on units and infantry. `ra_saveload_smoke.sh` passes. A full repaint on unshrouded
  SCG01EA takes 310 µs (max 404), against 445 µs for the cell-sorted full repaint and 263 µs for the
  old algorithm's; this pass also clears the view, sorts a copy and sweeps the flags twice.
- **Step 2 (RA removes the cell-sorted path): done.** `kSortDrawEnabled`, `Redraw_OIcons`,
  `CellClass::Draw_It`'s object half and `Calc_Partial_Window` are gone. `Redraw_Objects` now only
  flags the cell and cuts dead objects from the occupier chain; the `Mark(MARK_CHANGE)` calls it
  made were render-only (`RADIO_REDRAW` is answered with another `Mark`). `kOverlapperCount` is a
  plain 10. Position logs still match; `ra_saveload_smoke.sh` and its `--load-fixture` form pass.
- **Step 3 (pacing): nothing to do.** The spin loop is already capped: every pass of
  `WaitForNextFrame()` ends in `ServiceRealTime()` → `Display::EndFrame()`, whose `PresentFrame()`
  sleeps to hold presents to 70 a second, and each pass renders at most once. That bounds repaints
  at 70 a second, about 22 ms of CPU a second at 310 µs each. Headless SCG10EA to frame 600 used
  2.57 s of user time over 40 s both before step 1 and after step 2.

Red Alert draws the map with the cell-sorted renderer (`config::kSortDrawEnabled`); Tiberian Dawn
still uses the original y-sorted layer. Both exist only to avoid repainting the whole tactical view,
a 1995 constraint. A full repaint now costs about 0.3 ms. This plan replaces both algorithms with
one: repaint everything, drawing each object once in sorted order.

## Findings

### History

The shipped Red Alert did not use `SORTDRAW`: the original `CODE/DEFINES.H:141` has
`//#define SORTDRAW` and `#define PARTIAL` on. 1bb0d8ac ("Fix majority flickering issues by
switching to SORTDRAW") turned `SORTDRAW` on and `PARTIAL` off; 20e650d8 made it
`config::kSortDrawEnabled`. The 69f28795 commit message says the shipped game had it — that is
wrong.

`PARTIAL` computed each object's redraw area from its shape's real dimensions, including the
selection box (`UnitClass::Overlap_List` used `DimensionData`). With it off, the overlap lists are
the coarse fixed-size ones: a unit registers a 24-pixel square (`unit.cc:3102`), and the larger
selected/firing size is only used when refreshing (`Overlap_List(true)`), never when registering in
cells.

### How the two algorithms work

Both paint onto a hidden page that persists between frames and only repaint what is flagged. The
port calls `Render()` many times per logic frame, inside the spin loop in `WaitForNextFrame()`
(`ra/conquer.cc`, `td/conquer.cc`).

**Old algorithm (TD today; RA with the flag off).** Flagged cells repaint their terrain
(`Redraw_Icons`), then every object in the layers is `Render()`ed if its `IsToDisplay` is set, drawn
whole and unclipped, in layer order. The ground layer is kept in y order by `LayerClass::Sort()`,
which is a single bubble pass per logic frame (`ra/layer.cc:92`), so it can be out of order for a
few frames. The pass runs in game logic, since layer order feeds logic and sync
(`td/conquer.cc:1712`, and three calls in a row at `td/unit.cc:1408`).

Two failure modes:

- A marked object is drawn whole, over neighbours that were not repainted. It can land on top of
  something that should be in front of it.
- A cell whose terrain is repainted without its objects being marked loses them until the next mark.
  With many renders per logic frame, that shows up as flicker.

**Cell-sorted algorithm (RA today).** After the terrain, `Redraw_OIcons()` walks the flagged cells
in map order. Each cell gathers its occupiers and up to `kOverlapperCount` (10) overlappers, sorts
them by `Sort_Y()`, and draws each one clipped to the cell (`WINDOW_PARTIAL`, `cell.cc:1358`).
Partial repaints are therefore always self-consistent. That is what fixed the flicker. `Sort_Y()`
compares the packed coordinate, y then x, so two objects sort the same way in every cell they share.

Its own defects:

1. **Only footprint objects are cell-drawn.** Ground-layer anims (fire and smoke attached to an
   object, `IsGroundLayer` anims — `anim.cc:821`), ground bullets and cloaked units are drawn by the
   layer loop after all the cells. They end up on top of units that stand in front of them. The
   surface layer (corpses, torpedoes) had the same bug until 69f28795 special-cased it.
2. **Overlapper cap.** A cell holds 10 overlappers. A newcomer to a full cell is dropped silently
   (`CellClass::Overlap_Down`, `cell.cc:810`), so that part of it is not drawn; a building bumps
   someone else out instead. The cap is part of the save format (`cell.h:193`, `iomap.cc`).
3. **Clipped to the registered cells.** Anything an object draws outside the cells it registered in
   is cut at the cell edge. Health bars and selection brackets of selected units are the likely
   victims, given the 24-pixel registration above. Not yet confirmed on screen.
4. **Each object is drawn once per cell it overlaps.** That makes a full repaint 1.7× the cost of
   drawing each object once (numbers below).
5. **Workarounds:** the frame-end `IsToDisplay` sweep (`display.cc:2104`), the map-editor
   exceptions, and the surface-layer special case.

### Cost

`DisplayClass::Draw_It` timed with `steady_clock` around the whole call; RelWithDebInfo `build/`;
headless (`SDL_VIDEODRIVER=dummy`, `-NOMOVIES -NEWGAME<scenario> -QUITFRAME<n>`), where the game
renders once per logic frame. "Full" forced `forced = true` on every call. "Unshrouded" set
`TheDebugState().set_unshroud(true)`, which also makes `ObjectClass::Render` draw every object on
every call, so "flagged only" is not really incremental in that row.

| Scenario, frames   | Algorithm    | Flagged only, avg | Full repaint, avg (max) |
| ------------------ | ------------ | ----------------- | ----------------------- |
| SCG01EA unshrouded | cell-sorted  | 94 µs             | 445 µs (776)            |
| SCG01EA unshrouded | old (y-sort) | 191 µs            | 263 µs (399)            |
| SCG01EA, 600       | cell-sorted  | 56 µs             | 294 µs (532)            |
| SCU05EA, 600       | cell-sorted  | 15 µs             | 40 µs (207)             |
| SCG10EA, 600       | cell-sorted  | 24 µs             | 47 µs (113)             |

A full repaint with each object drawn once is ~0.26 ms, 1.6% of a 60 Hz frame.

### Other readers of the per-cell lists

`Overlappers` is not render-only: the AI's base lookup (`base.cc:197`) and radar jamming
(`radar.cc:610`) read it. The lists stay; so does the count of 10.

### Contracts the new pass must keep

- **Tethered contacts.** A building draws the unit docked with it and then clears that unit's
  `IsToDisplay` so the layer pass skips it (`ra/building.cc:826`; TD calls
  `Contact_With_Whom()->Render(view, true)` at `td/building.cc:822`). The new pass must keep going
  through `Render()` and `IsToDisplay`, not call `Draw_It` directly.
- **Layer order is game state.** The render pass must sort a copy of the ground layer, never the
  layer itself. RA's logic currently does not sort the layer at all (the sort in `conquer.cc:692` is
  compiled out while the flag is on); TD's does. Neither changes.
- **`AnimClass::Render`** skips delayed anims and only draws visible ones (`anim.cc:244`). Keep
  calling it.

## Plan

### Step 0: commit this document

### Step 1: RA full repaint

In `DisplayClass::Draw_It` (`ra/display.cc`):

1. Repaint when anything is dirty: `IsDisplayToRedraw`, `forced`, any flagged cell, or a change in
   tactical position. Otherwise do nothing, as today.
2. On repaint: flag every cell, clear the tactical rectangle (unmapped and re-shrouded cells are
   otherwise stale), `Redraw_Icons`, the chronal vortex, the object pass, `Redraw_Shadow`, the
   rubber band.
3. **Object pass**, a new `DisplayClass::Render_Objects(PixelView&)`: for each layer in order, copy
   its active objects into a reused, unserialized `std::vector<ObjectClass*>` member. Sort the
   ground layer's copy with `std::ranges::stable_sort` by `Sort_Y()`. Draw the surface layer first,
   as now. Set `IsToDisplay` on each object, then call `Render(view, false)`, so the
   tethered-contact rule still works. Clear leftover flags at the end, as the current sweep does.
4. Delete the scroll-by-blit code and its edge-cell flagging (`display.cc:1791`–~2020).
   `Set_Tactical_Position` stays.

### Step 2: RA removes the cell-sorted path

- Delete `config::kSortDrawEnabled` and every branch on it:
  - `CellClass::Draw_It`'s `objects` parameter and its sort block.
  - `DisplayClass::Redraw_OIcons`.
  - The surface-layer special case and the `LAYER_GROUND` footprint skip in the layer loop.
  - The `!kSortDrawEnabled` sort in `conquer.cc:692`, which stays off.
- `CellClass::Redraw_Objects` keeps flagging the cell and its dead-pointer cleanup (inactive `Next`
  and inactive overlappers set to `nullptr`); the per-object `Mark(MARK_CHANGE)` loops can go.
- `kOverlapperCount` becomes a plain 10, with a comment naming the save format and `base.cc`.

### Step 3: pacing

`WaitForNextFrame()` calls `Render()` in a spin loop. With step 1, a flag anywhere repaints
everything, and cells get flagged between logic frames by the rubber band, placement cursor and help
text. Measure CPU on a real display (idle map, scrolling, a box-select drag) before and after. If
the repaint shows up, cap tactical repaints at one per display refresh.

### Step 4: TD

Apply the same change to `td/display.cc`:

- Repaint everything when dirty, and draw the objects from the sorted copy.
- Delete the scroll-blit block.
- Keep the logic-side `Layer.Sort()` calls: they are sync-relevant.
- Check the tethered-contact double draw via `Render(view, true)` in `td/building.cc:822`.

The on-screen lag of the single bubble pass goes away without touching logic.

### Later, not in scope

With full repaints, `IsToDisplay`, `MARK_CHANGE` and `Refresh_Cells` only tell the renderer
"something changed". A later pass could reduce them to one dirty bit. `Overlap_List(bool redraw)`
then serves only the logic lists.

## Verification

- Build, `tools/strict_tu.py` on each edited file during the edit loop, then a full strict build
  before each commit.
- **Logic must not change.** Diff `-QUITFRAME600` position logs (SCU05EA, SCG10EA; TD SCB05EA,
  SCG10EA) from the old and new binaries. Aircraft differ between two runs of the same binary, so
  compare the other objects; run the old binary twice first to see what is noise. A fresh working
  directory needs one warm-up run: the first run copies MIX files and hangs. Also run
  `tools/ra_saveload_smoke.sh`, `tools/td_saveload_smoke.sh` and their `--load-fixture` forms.
- **Timing.** Re-apply the probe from Findings. A full repaint on unshrouded SCG01EA should come in
  at or under the old algorithm's 263 µs.
- **Human check on a real display** (headless never loads palettes):
  - Corpses under the soldiers standing over them.
  - Fire on a burning building behind a tank passing in front of it.
  - A unit leaving the weapons factory, and a harvester docking at the refinery.
  - The health bar of a selected vehicle at a cell edge.
  - A dense infantry crowd, at least 10 overlappers.
  - Cloaked sub and Phase Transport shimmer.
  - Chronal vortex.
  - Rubber-band select.
  - Multiplayer message lines.
  - Help tooltip.
  - Building placement cursor.
  - Map editor.
  - Gap generator shroud.
  - Fast and slow scrolling.
  - TD: the same, where it has the feature.

## Risks

- **Overlays drawn into the tactical area only on their own flag.** Anything that draws over the map
  only when its own flag is set, and counts on the page keeping it, would be erased by the next full
  repaint. `HelpClass::Draw_It` redraws on every call and `GScreenClass::Render` draws the messages
  on every call; audit the map-editor overlays.
- **Busy spin cost.** Step 3 measures it.
- **Intended visual changes.** Anims and cloaked units now sort with the units around them. The
  distortion effects of cloaked units now apply over what is really behind them.
- **Save format and sync.** Unchanged: no new serialized fields, `kOverlapperCount` stays 10, and
  each game's logic-side sorting stays as it is.
