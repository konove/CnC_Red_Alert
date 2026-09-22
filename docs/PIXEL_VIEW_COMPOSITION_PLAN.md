# PixelBuffer / PixelView Composition Plan

`src/sdllib/pixel_buffer.h` declares two classes for the 8-bit paletted pages the games draw on:

- `PixelBuffer` owns the pixels — `owned_pixels_` or a caller's span, or, for the one page the
  window shows, an SDL surface and texture plus the palette and the present path.
- `PixelView` is a clipped rectangle onto such a buffer and carries every drawing primitive.

The split itself is right. A stack-local `draw_window` in `radar.cc` or `techno.cc` has no business
carrying `window_texture_`, `palette_surface_` and a `std::vector<uint8_t>` it never uses, and SDL
forces the back-pointer anyway: locking the surface moves the pixels, so a sub-rectangle must be
able to re-derive its `offset_` from whoever owns them.

What is wrong is the relation. `PixelBuffer : public PixelView` is inheritance for storage reuse —
the buffer borrows the view's `offset_`, `width_`, `height_` and `lock_count_` and then spends three
special cases undoing the inheritance.

## Findings

Measured on the tree at f1068921.

**1. Three `this == buffer_` guards exist only to cancel the base class.**

- `PixelView::Attach()` (`pixel_buffer.cc:72`) early-returns when the view is its own buffer, so a
  public base-class method is documented as a no-op on the derived class — "Has no effect on a
  PixelBuffer, which is permanently the view covering itself."
- `PixelView::Lock()` skips the reattach for the same reason.
- `PixelView::pixels()` branches to `buffer_->bytes()`. That branch is dead: `Init()` sets
  `x_pos_ = y_pos_ = 0` on a buffer, so the general path already computes `subspan(0)`.

**2. `lock_count_` lives in the wrong class, and a caller already reads the wrong one.**

The member sits in `PixelView` with the comment "Only the buffer's own count is used, so a view
carries this member without ever changing it" — dead storage on every view.
`PixelView::lock_count()` therefore returns the view's permanent zero, and `Any_Locked()` in
`ra/winstub.cc:71` and `td/winstub.cc:413` asks exactly that of two views:

```cpp
if (TheScreen().visible_view().lock_count() ||
    TheScreen().hidden_view().lock_count()) {
```

Both operands are always 0, so `Any_Locked()` always returns false. Composition forces the accessor
to forward to the buffer, which fixes it.

**3. `DrawScaledRotated` reads the base's fields as if they were its own.** It iterates
`width_`/`height_` and indexes `bytes()` by `y * width_`, with a comment explaining that this only
works because `Init()` leaves `x_add_` and `pitch_` zero. After the split those are the buffer's own
dimensions and the comment is no longer needed.

**4. Call-site cost.** Drawing calls made directly on a `PixelBuffer`, which have to grow a
`.view()`:

| Method     | Calls on the `Screen` pages |
| ---------- | --------------------------: |
| `Clear`    |                         137 |
| `Blit`     |                          24 |
| `FillRect` |                           9 |
| `Scale`    |                           8 |
| `GetPixel` |                           1 |

plus the local and static buffers (`IconStage`, `TileStage`, `temp_page`, `backpage`,
`pseudoseenbuff`, the test fixtures) and the places that pass a `PixelBuffer` where a `PixelView&`
is expected. `bytes()`, `width()`, `height()` and `ReleaseSurfaces()` stay on the buffer and are
untouched.

## Plan

**Step 1 — split the members.** `PixelBuffer` stops deriving from `PixelView` and gains its own
`width_`, `height_`, `pitch_` and `lock_count_` alongside `bytes_`, `owned_pixels_` and the SDL
handles. `PixelView` keeps `offset_`, `x_pos_`, `y_pos_`, `width_`, `height_`, `x_add_`, `pitch_`
and `buffer_`, and loses `lock_count_`.

**Step 2 — compose.** `PixelBuffer` holds a `PixelView whole_` covering all of itself and exposes
`PixelView& view()`. `Init()` re-attaches it after sizing; `LockSurface()`/`UnlockSurface()`
re-attach it so its `offset_` follows the surface, which is what `PixelView::Lock()` already does
for every other view. `PixelView::lock_count()` forwards to `buffer_->lock_count()`.

**Step 3 — drop the guards.** Delete the `this == buffer_` branches in `Attach()`, `Lock()` and
`pixels()`, and the comments that explained them.

**Step 4 — call sites.** Insert `.view()` wherever a drawing primitive is invoked on a buffer or a
buffer is passed as a `PixelView&`. The compiler enumerates every one of them once step 1 lands.

**Step 5 — `Any_Locked()`.** With `lock_count()` forwarding, both games' `Any_Locked()` starts
reporting real lock depth. Check its callers still do the right thing when it can return true.

`src/winvq/vqaview/` is not in any `CMakeLists.txt` and already calls removed APIs
(`Get_DD_Surface`); it is left alone.

## Verification

- `cmake --build build --parallel 22` for both games, then a full strict build at `--parallel 14`.
- `ctest` in the strict dir: `pixel_buffer_test`, `display_test`, `screen_test`, `winbits_test`,
  `intro_test`, `pcx_test`, `wsa_animation_test`, `keyframe_test` all exercise these classes.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh`.
- Run both games and confirm the menu, the map, the radar and a movie still draw.

## Progress

- [ ] Steps 1-4
- [ ] Step 5
