# Plan: replace the legacy containers in game/core

Written 2026-10-01 against `main` at 2d8a614a.

**Status: complete (2026-10-01).** `VectorClass` and `DynamicVectorClass` are gone; the pool is one
non-virtual class; `QueueClass` is trimmed. Headless 600-frame logic logs (RA SCU05EA, TD SCG01EA)
match the pre-plan code line for line, every save/load smoke fixture passes, and the checked-in
pre-plan RA save loads. See Progress for the commits and the deviations from the plan.

## Context

`src/game/core/` still holds five hand-written containers from 1995:

| File                | Class                                                   | What it is                                                     |
| ------------------- | ------------------------------------------------------- | -------------------------------------------------------------- |
| `vector.h`          | `VectorClass<T>`                                        | Resizable array that can borrow a caller's buffer              |
| `vector_dynamic.h`  | `DynamicVectorClass<T>`                                 | `VectorClass` plus an active count: a growable list            |
| `owned_vector.h`    | `OwnedVector<T>`                                        | Already a wrapper over `std::vector<std::unique_ptr<T>>`       |
| `queue.h`           | `QueueClass<T, N>`                                      | Fixed-capacity ring buffer (the network event queues)          |
| `heap.h`, `heap.cc` | `FixedHeapClass`, `FixedIHeapClass`, `TFixedIHeapClass` | Fixed-slot object pool behind every game object's operator new |

The two vectors duplicate `std::vector`. The pool and the queue do something the standard library
cannot, but each carries legacy weight that nothing uses. This plan deletes the vectors and trims
the pool and the queue. `OwnedVector` stays as it is (see "Out of scope").

## Findings

### The vectors

- **The borrowed-buffer feature is unused.** The constructor's and `Resize`'s `std::span<T> array`
  argument is passed only by `game/core/heap_test.cc:27` (`VectorStorageTest`). Without it,
  `VectorClass` is a `std::vector` with a different spelling.
- **Every dynamic vector grows on its own.** `Set_Growth_Step` has no callers, so `GrowthStep` is
  always 10 and `Add` fails only for a borrowed buffer. `push_back` is a drop-in.
- **Save games store counts and elements, never capacity.** `SerializeObjectList`
  (`ra/saveload/serialize.cc:177`, `td/saveload/serialize.cc:316`), `SerializeTriggerList`
  (`ra/saveload/saveload.cc:166`) and `BaseClass::Serialize` (`ra/ai/base.cc:432`) write `Count()`
  and then each element. `Resize(count)` there only pre-sizes before `Add`, so it becomes
  `reserve(count)`. **The save format does not change** and `kSaveGameVersion` is not bumped.
- **Explicit instantiations.** `ra/core/vector.cc`, `ra/core/dynavec.cc` and `td/core/vector.cc`
  contain only `template class ...` lines, several for element types nothing uses any more
  (`TeamMissionClass*`, `EventChoiceClass*`, `EgoClass*`, ...). They are deleted with the templates.
  The CMake globs pick up the deletions.

Declarations, by kind:

| Kind                            | Where                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| ------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Plain lists                     | `World::current_object_` (271 uses across both games), `logic_triggers_`, `map_triggers_`, the per-house trigger `EnumArray` (both games), `BaseClass::Nodes` (both), `ColorListClass::Colors` (RA `RemapControlType*`, TD `char`), `TListClass<T>::List` (`ra/ui/tlist.h`), TD `Session::scenarios_`/`scenario_files_`, `WinsockUdp...::BroadcastAddresses`/`LocalAddresses` (`ra/net/wspudp.h`), `ar_szScenarios[4]`/`ar_szScenIndexes[4]` (`ra/wol/wol_gsup.h`), the `trignames`/`teamnames` locals in `td/editor/mapeddlg.cc`, `FixedIHeapClass::ActivePointers` |
| A list used as a fixed array    | TD `World::cell_triggers_`: `Resize(MAP_CELL_TOTAL)` (`td/map/map.cc:452`) and then indexed by cell with `Count() == 0`, which reads the capacity slots that `VectorClass` value-initialises. **This must become `resize`, not `reserve`.**                                                                                                                                                                                                                                                                                                                          |
| Base class of a game class      | `LayerClass : DynamicVectorClass<ObjectClass*>` (both games; `LogicClass` derives from it). The two `layer.cc` files are identical apart from headers. TD `GScreenClass : VectorClass<CellClass>` is the cell array of TD's map chain.                                                                                                                                                                                                                                                                                                                               |
| Member that pokes the internals | RA `MapClass::Array` (`VectorClass<CellClass>`). `LayerClass::Sorted_Add` reads `IsAllocated`/`VectorMax`/`GrowthStep`, and TD `MapClass::Alloc_Cells` (`td/map/map.cc:493`) zeroes `Vector`/`VectorMax`/`IsAllocated` by hand.                                                                                                                                                                                                                                                                                                                                      |

`LayerClass` is called as a container: `Count`/`at` 34 times each through `DisplayClass::Layer`, and
`Count`/`at`/`Delete`/`ID`/`Submit`/`Sort`/`Init`/`One_Time`/`AI`/`Serialize` through
`TheWorld().logic()`.

### The queue

`QueueClass` is used only for the network event queues, `out_list_` and `do_list_` (RA
`ra/net/network.h:161`, TD `td/net/network.h:229`), plus tests. The calls are
`Add`/`Next`/`First`/`at`/`Count`/`Init`. `std::deque` would drop the capacity bound that `Add`
returning false enforces, and C++23 has no fixed ring buffer, so the class stays. Dead weight:

- `Get_Head`, `Get_Tail` and `Get_Array` have no callers.
- The "size MUST be an exact power of two" warning is stale. Indexing uses `%`, and the tests use
  size 3.
- `T Array[size]` is a C array, and the comments are mangled 1994 banner blocks.

### The pool

Slot identity is game state: TARGET values and `CCPtr` encode the slot index,
`TFixedIHeapClass::Save` and `Load` put each object back in the same slot, and the `ActivePointers`
order is the order the game iterates objects in, which multiplayer sync depends on. So the pool
stays. What can go:

- **`FixedHeapClass` as a separate class.** Nothing uses it except as the base of `FixedIHeapClass`.
  All pointers to a heap are `FixedIHeapClass*` (the `CCPtr<T>::Heap` statics in `ra/core/ccptr.h`).
- **Every `virtual`.** Nothing overrides a member past `FixedIHeapClass`, and nothing deletes a heap
  through a base pointer. `TFixedIHeapClass`'s typed members already hide the base ones rather than
  override them.
- **The external-buffer path.** All 104 `Set_Heap` callers pass a count only. That makes
  `std::span<char> buffer`, `IsAllocated` and the manual `new char[]`/`delete[]` dead; the heap can
  own its storage in a `std::unique_ptr<char[]>`.
- **`ActivePointers` as a public `DynamicVectorClass<void*>`.** Its only outside reader is
  `td/saveload/ioobj.cc:221` (`ActivePointers.ID(House) < 0`), which is what `Logical_ID(House) < 0`
  says.

Behaviour that must survive the collapse:

- **Same-count `Set_Heap` reuse.** `Set_Heap` with the same count keeps the existing buffer
  (`heap.cc:411`). Its comment says this prevents use-after-free during rule loading.
- **Zeroed slots.** `Allocate` zero-fills the slot before returning it.
- **Lowest free slot first.** `Allocate` takes the lowest free slot (`base::first_false`).
- **Order-preserving removal.** `Free` removes from `ActivePointers` without disturbing the order of
  the rest.

Bug found: `FixedIHeapClass::Free` (`heap.cc:476`) returns `false` even when it freed the slot. No
caller reads the result today.

## Plan

Each step builds both games, passes `ctest`, and is committed on its own. Run
`tools/strict_tu.py <files>` on the files a step touches during the step, and do a full strict build
before the final commit (see Verification).

### Step 0: commit this plan

- [ ] Commit `docs/GAME_CONTAINERS_PLAN.md` alone, and add a "Current Work" line to the memory
      index.

### Step 1: `std::vector` helpers in base

`std::vector::at` takes a `size_t`, and the tree indexes with `int`/`base::ssize`. Without a helper,
every `list.at(i)` would become `list.at(base::ToSize(i))`.

- [ ] In `base/array.h`, add checked access for vectors next to the span overload:
  ```cpp
  // Returns an element of a vector. Negative and past-the-end indices fail in
  // every build mode.
  template <class T, class A>
  constexpr T& At(std::vector<T, A>& vector ABSL_ATTRIBUTE_LIFETIME_BOUND,
                  std::integral auto index) {
    return At(std::span(vector), index);
  }
  template <class T, class A>
  constexpr const T& At(const std::vector<T, A>& vector ABSL_ATTRIBUTE_LIFETIME_BOUND,
                        std::integral auto index) {
    return At(std::span(vector), index);
  }
  ```
  `std::vector<bool>` has no span and does not compile here, which is correct: `FreeFlag` keeps its
  `.at(base::ToSize(i))`.
- [ ] In `base/algorithm.h`, add two helpers:
  ```cpp
  // Returns the index of the first element equal to value, or -1.
  template <class Container, class T>
  ssize IndexOf(const Container& c, const T& value);
  // Removes the first element equal to value, keeping the rest in order.
  // Returns false if there is none. Unlike std::erase, it removes one match.
  template <class T, class A, class U>
  bool EraseFirst(std::vector<T, A>& vector, const U& value);
  ```
- [ ] Tests in `base/array_test.cc` and a new `base/algorithm_test.cc` (add it to base's `add_gtest`
      list):
  - `At` on a vector returns a reference to the element, including through a `const` vector.
  - `At(v, -1)` and `At(v, v.size())` die with "Check failed".
  - `IndexOf` returns -1 for a missing value and the first index when the value appears twice.
  - `EraseFirst` removes only the first of two equal values and keeps the order of the others.

### Step 2: trim `QueueClass`

- [ ] Replace `T Array[size]` with `std::array<T, size>`, and add `static_assert(size > 0)`.
- [ ] Delete `Get_Head`, `Get_Tail` and `Get_Array`.
- [ ] Rewrite the comments in Google style, and drop the power-of-two warning and the banner blocks.
- [ ] Keep the name and the API (`Add`/`Next`/`First`/`at`/`Count`/`Init`). That leaves the ~300
      network call sites alone.
- [ ] Move the queue tests out of `game/core/heap_test.cc` into a new `game/core/queue_test.cc`, and
      add two cases: `Add` returns false once the queue is full and leaves `Count()` at capacity,
      and a queue whose size is not a power of two wraps in FIFO order through several full cycles.

### Step 3: collapse the pool

- [ ] Fold `FixedHeapClass` into `FixedIHeapClass` and delete `FixedHeapClass`. Keep the names
      `FixedIHeapClass`/`TFixedIHeapClass`: `CCPtr` and ~290 lines refer to them, and a Google-style
      rename is a separate pass (see "Out of scope").
- [ ] Remove every `virtual` and `override`. The destructor becomes non-virtual. `TFixedIHeapClass`
      stays derived from `FixedIHeapClass`, because `CCPtr` binds the base.
- [ ] Storage becomes `std::unique_ptr<char[]> storage_` with `std::span<char> Buffer` viewing it.
      `Set_Heap(int count)` loses its buffer parameter and `IsAllocated` goes.
  - A same-count call keeps `storage_`: it clears `FreeFlag`/`ActivePointers` and the counts, then
    returns.
  - In `TFixedIHeapClass`, add `static_assert(alignof(T) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__)`,
    since `new char[]` only guarantees that alignment.
- [ ] Change `ActivePointers` to a private `std::vector<void*>`.
  - `Set_Heap` calls `reserve(count)`, so gameplay never reallocates it.
  - `Free` uses `base::EraseFirst`, which keeps the order.
  - `Free_All` uses `clear()`.
  - `Active_Ptr` uses `base::At`.
  - Change `td/saveload/ioobj.cc:221` to use `Logical_ID(House) < 0`.
- [ ] Make `Free` return true when it freed something.
- [ ] `heap.h` stops including `vector.h`/`vector_dynamic.h`.
- [ ] Tests in `game/core/heap_test.cc`:
  - Freeing the middle of three objects leaves `Ptr(0)` and `Ptr(1)` as the first and third, in that
    order.
  - `Free` returns true for a live object and false for a second free of it.
  - A second `Set_Heap` with the same count keeps `Raw_Ptr(0)` at the same address and leaves
    `Count() == 0`.
  - `Allocate` after `Free_All` returns slot 0 again, zero-filled.
- [ ] Delete `VectorStorageTest` (it tests the borrowed buffer, which Step 5 deletes). It must stop
      using `DynamicVectorClass` here anyway.

### Step 4: plain lists become `std::vector`

Mechanical. Fork agents on disjoint file groups, per [[parallel-forks-for-mechanical-sweeps]]: one
group per game for the `World` lists and their callers, one for the UI/net/WOL/editor lists. Each
agent applies this table, runs `git clang-format` on its own files, and reports any signature change
that crosses into another group.

| Legacy                                  | Replacement                                                                                 |
| --------------------------------------- | ------------------------------------------------------------------------------------------- |
| `game::DynamicVectorClass<T> x;`        | `std::vector<T> x;`                                                                         |
| `x.Count()`                             | `std::ssize(x)`                                                                             |
| `x.at(i)` / `x[i]`                      | `base::At(x, i)`                                                                            |
| `x.Add(v)`                              | `x.push_back(v)` (drop any `if (!...)` check)                                               |
| `x.Add_Head(v)`                         | `x.insert(x.begin(), v)`                                                                    |
| `x.Delete(index)`                       | `x.erase(x.begin() + index)` after a range check if the old code relied on the false return |
| `x.Delete(value)`                       | `base::EraseFirst(x, value)`                                                                |
| `x.ID(value)`                           | `base::IndexOf(x, value)`                                                                   |
| `x.Clear()` / `x.Delete_All()`          | `x.clear()`                                                                                 |
| `x.Resize(n)` before `Add`s (save load) | `x.reserve(n)`                                                                              |
| `ActiveElements()`                      | `std::span(x)`                                                                              |

- [ ] RA and TD `World` lists and the save-load helpers that take them (`SerializeObjectList`,
      `SerializeTriggerList`, `BaseClass` node serialization). Each helper's parameter type changes,
      as do the forward declarations of `DynamicVectorClass` in both `serialize.h` files.
- [ ] TD `cell_triggers_` becomes `std::vector<TriggerClass*>` and is sized with
      `resize(MAP_CELL_TOTAL)`, which, like the old `Resize`, keeps existing entries. Do not use
      `reserve`: the cells are indexed without being added.
- [ ] `Nodes`, `Colors`, `TListClass::List`, the session scenario lists, the WinSock address lists,
      the WOL scenario arrays and the map editor's name lists.
- [ ] Before converting a `.Length()` call, check whether the variable is a dynamic vector. On a
      `DynamicVectorClass`, `Length()` is the capacity, not the count; the grep for this plan found
      capacity reads only in `cell_triggers` and `LayerClass::Sorted_Add`. To be sure, mark
      `DynamicVectorClass::Length` `[[deprecated]]` in a scratch overlay and compile every
      translation unit with `-fsyntax-only` (see [[deprecated-overlay-finds-member-callers]]).

### Step 5: game classes stop inheriting from the vectors, then delete the vectors

- [ ] `LayerClass` (both games) holds a private `std::vector<ObjectClass*> objects_` instead of
      inheriting.
  - Its interface is the methods called on it: `Count()`, `at()`, `Delete(ObjectClass*)`,
    `Delete(int)`, `ID()`, `Clear()`, `Submit()`, `Sorted_Add()`, `Sort()`, `Init()`, `One_Time()`
    and `Serialize()`. These forward to the vector, so the 34+14 layer and logic call sites stay as
    they are.
  - `Sorted_Add` keeps the linear search for the first element that is greater than the new object.
    `std::upper_bound` is wrong here, because `Sort()` is one incremental bubble pass and the list
    is not guaranteed sorted. `Sort()` stays one bubble pass and is not replaced by `std::sort`;
    both changes would alter draw and logic order.
- [ ] RA `MapClass::Array` becomes `std::vector<CellClass> cells_`.
  - `Alloc_Cells` calls `resize(Size)`; RA's old `Resize` kept existing cells.
  - `ID(const CellClass*)` is `ptr - cells_.data()`.
  - `at(CELL)` uses `base::At`.
- [ ] TD `GScreenClass` stops deriving from `VectorClass<CellClass>`. It gets a
      `std::vector<CellClass> cells_` member with the accessors its callers use (`at`, `operator[]`,
      `Length`, `ID`, `Resize`). TD `Alloc_Cells` becomes `cells_.clear(); cells_.resize(Size);`,
      which is what zeroing `Vector`/`VectorMax` meant: start from fresh cells.
- [ ] Delete `game/core/vector.h`, `game/core/vector_dynamic.h`, `ra/core/vector.cc`,
      `ra/core/dynavec.cc` and `td/core/vector.cc`. Also delete the `VectorClass` cases in
      `heap_test.cc` and `td/net/queue_alignment_test.cc`, and the mention of `DynamicVectorClass`
      in `owned_vector.h`.
- [ ] Update the `game/core/` line in CLAUDE.md's Architecture section. It should list
      `OwnedVector`, the object pool (`FixedIHeapClass`/`TFixedIHeapClass`) and `QueueClass`.
- [ ] Mark this plan done and update the memory line.

## Review focus

Things the unit tests do not cover. Check each in review:

1. **Iteration order.** Any reordering of `ActivePointers`, a layer or `current_object` changes
   simulation or draw order. Removal is `erase`/`EraseFirst`, never swap-and-pop; `Sorted_Add` and
   `Sort` keep their exact algorithms.
2. **Indexing past `Count()`.** `cell_triggers` is the known case. Any other list read past
   `Count()` now dies in `base::At` instead of reading a value-initialised slot. That is a correct
   failure, but the code needs to size the list with `resize`.
3. **Cell pointer stability.** `resize` can reallocate, exactly as the old `Resize` did, and
   `CellClass*` values must not be cached across `Alloc_Cells`. Nothing changes here, but do not add
   a `reserve` that hides the issue.
4. **Heap alignment.** `alignof(T)` for every pooled class must fit `new char[]`; the
   `static_assert` in Step 3 enforces it.
5. **Loading corrupt saves.** The load helpers still validate the count before `reserve(count)`. A
   hostile count must not become a huge allocation.

## Verification

Per step:

```bash
cmake --build build --parallel 22
ctest --test-dir build --output-on-failure
tools/strict_tu.py <touched .cc files>
```

Before the Step 5 commit:

- Run a full strict build of both games (`cmake --build cmake-build-strict-ra-clang --parallel 14`,
  in the foreground).
- Run the save/load smoke tests with the binary passed explicitly:
  - `tools/ra_saveload_smoke.sh build/src/ra/rasdl`
  - `tools/td_saveload_smoke.sh build/src/td/tdsdl SCG01EA` with each fixture (`--factory`,
    `--team`, `--world`, `--building`, `--mobile`, `--map`, `--globals`)
- Diff the logic logs against the pre-plan binary, built from 2d8a614a (see
  [[headless-logic-diffs]]). Use RA `-NEWGAMESCU05EA -QUITFRAME600` and TD
  `-SEED1 -NEWGAMESCG01EA -QUITFRAME600`, and run the old binary twice first to measure noise. A
  line that differs beyond the noise means iteration order changed.
- Load a save game written before the plan with the new binary. The format is unchanged, so it must
  load.

## Out of scope

- **`OwnedVector`.** It already owns its objects in a `std::vector<std::unique_ptr<T>>`. Replacing
  it with the bare vector changes 16 call sites for no gain in safety.
- **Renaming the pool to Google style.** For example `FixedIHeapClass` → `ObjectPool`, `Set_Heap` →
  `Reserve`, `Ptr` → `active`. Do it as a `/rename-google-style` pass after Step 3.
- **Sharing `LayerClass` between the games.** The two `layer.cc` files are identical, so it could
  move to `game/` once it no longer inherits from a game-specific vector instantiation.

## Progress

| Step | Commit(s)          | Notes                                                                                       |
| ---- | ------------------ | ------------------------------------------------------------------------------------------- |
| 0    | 17b104e0           | This plan.                                                                                  |
| 1    | 886e6a17           | `base::At(vector)`, `base::IndexOf`, `base::EraseFirst`.                                    |
| 2    | b7d5127d, 91a0e1fd | Added `base::At(std::array)`: a temporary span over a member fails lifetime analysis.       |
| 3    | 6475411a           | Pool collapsed; `Free()` returns true when it frees.                                        |
| 4    | 3d54a3b0, 49c63675 | One commit per game. `LayerClass` moved here from Step 5 (it shares `SerializeObjectList`). |
| 5    | see `git log`      | Cell arrays, then the legacy vectors deleted.                                               |

Deviations and findings:

- **`LogicClass::AI()` read past the end.** When the last object removed itself during its AI, the
  loop re-read the slot just past `Count()`, which the old vector's spare capacity made legal. The
  stricter `base::At` crashed every save/load smoke run until the loop stopped there instead.
- **TD's map editor tested a stale slot.** Alt-1..4 asked whether `current_object().at(0)` was
  non-null, which read a stale pointer once the selection emptied; it asks `!empty()` now.
- **Cells cannot be resized.** `CellClass` deletes its copy and move constructors (RA's cell takes
  its `ID` from its address), so `std::vector::resize` does not compile under clang (GCC accepted
  it). `Alloc_Cells()` builds a new `std::vector<CellClass>(Size)` instead. RA's old `Resize()` kept
  existing cells, but `Alloc_Cells()` runs only from `One_Time()`, before `Init_Cells()` rebuilds
  every cell. TD's `GScreenClass` gained `Replace_Cells()`; its tests call that and `Free_Cells()`
  instead of the inherited `Resize()`/`Clear()`.
- **TD logic depends on the build directory.** The pre-plan source built in a scratch worktree logs
  a different SCG01EA run (22,384 differing lines from frame 2) than the same source built in
  `build/`, while every `build/` binary from before and after this plan logs the same run. The plan
  changed no logic; the build-directory dependence is pre-existing and unexplained.
