# Plan: replace CCPtr with plain pointers

Written 2026-10-01 against `main` at 2d6ef8ed.

**Status: in progress.** Steps 1 and 2 done.

## Context

`CCPtr<T>` (`src/ra/core/ccptr.h`, `src/ra/core/ccptr.cc`) is a Red Alert pointer that stores a heap
slot index instead of an address. Every dereference goes through a static `Heap` pointer for its
`T`. Joe Bostic wrote it in 1996 so that saving a game could write objects as raw memory without
fixing up pointers.

That reason went away when save/load became field-wise `Serialize()` (bf27db84). Tiberian Dawn never
had `CCPtr`: it uses plain pointers with save-time wrappers (`TypePtr`, `HousePtr`, `TeamTypePtr`,
`ObjectPtr` in `td/saveload/serialize.h`). What Red Alert still pays for it:

- **27 static `Heap` specializations** declared in `ccptr.h`, defined in `object_heaps.cc` and
  `type_heaps.cc`, and bound by `BindHeap` in the `ObjectHeaps`/`TypeHeaps` constructors. Plus 27
  explicit instantiations in `ccptr.cc` and their `extern template` lines.
- **A lookup and a `DCHECK` on every dereference.** `operator->` returns `nullptr` for an empty
  pointer, and the static analyzer follows that branch. `ra/objects/unit.cc:4245` binds `*Class` to
  a reference just to stop it.
- **Implicit conversions both ways** under `NOLINT(*-explicit-constructor)`, and 14
  `static_cast<XClass*>(member)` calls that only unwrap a `CCPtr`
  (`delete static_cast<FactoryClass*>(Factory)`).

## Findings

### The members

25 data members, and nothing else: no function takes or returns a `CCPtr`.

| Group                     | Members                                                                                                                                                                                                                     | Heap                   |
| ------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ---------------------- |
| Type pointers             | `Class` in `AircraftClass`, `AnimClass`, `BuildingClass`, `BulletClass`, `HouseClass`, `InfantryClass`, `OverlayClass`, `SmudgeClass`, `TemplateClass`, `TerrainClass`, `UnitClass`, `VesselClass`                          | `TheTypeHeaps()`       |
| Object pointers, saved    | `TechnoClass::House`, `TeamClass::House`, `FactoryClass::House`, `BuildingClass::Factory`, `TeamClass::Class`, `FootClass::Team`, `ObjectClass::Trigger`, `CellClass::Trigger`, `TeamClass::Trigger`, `TriggerClass::Class` | `TheObjectHeaps()`     |
| Object pointers, from INI | `TEventClass::Team`, `TActionClass::Team`, `TActionClass::Trigger`, `TeamTypeClass::Trigger`                                                                                                                                | `TheObjectHeaps()`     |
| Editor list elements      | `TListClass<CCPtr<TeamTypeClass>>` (`editor/mapedtm.cc:262`), `TListClass<CCPtr<TriggerTypeClass>>` (`editor/mapeddlg.cc:2669`)                                                                                             | (built from the heaps) |

`ccptr.h` is included by 70 files. Most include it only for the member types above.

### Writes through a type pointer

Type objects are game rules, and Tiberian Dawn holds them as `const XTypeClass*`. Two RA members
write through `Class`:

- `AnimClass::Class` (`objects/anim.cc:500`, `:659`, `:672`, `:753`) fills `Stages` and `LoopEnd` on
  the type the first time an animation plays. It stays `AnimTypeClass*` (see Out of scope).
- `TeamClass::Class` (`ai/team.cc:295`, `:342`) counts live teams in `TeamTypeClass::Number`. Team
  types live in the object heaps and are edited by the scenario editor, so this one is not a type
  pointer in that sense and stays non-const.

Every other `Class` becomes `const XTypeClass*`. If the compiler turns up another write, list it
here and keep that member non-const.

### Save format

`CCPtr::Serialize` writes the slot index as an `int32`, -1 for null, and on read rejects an index
outside `[0, Length())` with `ar.Fail`. It does not check that the slot holds a live object; a
reference to a heap that loads later is fine because nothing dereferences it before then.

A wrapper that writes the same `int32` keeps **the save format unchanged**. `kSaveGameVersion` stays
16, and the checked-in fixture `ra/testdata/SAVEGAME.SCG01EA` must still load. That is the test that
the wrapper is byte-compatible.

The existing `ObjectPtr` can't be reused for this. It writes a TARGET (heap kind plus index), a
different encoding.

Slot addresses are fixed for the life of the heap. `Init_Heaps` (`startup/init.cc:1859`) sizes the
object heaps once, and `Set_Heap` with the same count keeps the buffer. So resolving an index to the
slot's address at read time gives the same object that `CCPtr` would find at dereference time.

### INI references to later sections

Four members are read from scenario INI as raw slot indices with `Set_Raw`:

| Site                    | Field                           | Refers to                                                                                   |
| ----------------------- | ------------------------------- | ------------------------------------------------------------------------------------------- |
| `ai/teamtype.cc:1842`   | `TeamTypeClass::Trigger`        | a trigger type. Team types are read (`scenario.cc:2123`) **before** trigger types (`:2154`) |
| `ai/teamtype.cc:1937`   | `TeamTypeClass::Trigger`        | the same, old format                                                                        |
| `ai/taction.cc:320-321` | `TActionClass::Team`, `Trigger` | a team type (already read), and a trigger type, possibly later in the same section          |
| `ai/tevent.cc:675`      | `TEventClass::Team`             | a team type (already read)                                                                  |

The index is the object's `Logical_ID` when the scenario was written (`Build_INI_Entry`,
`taction.cc:292`). Reading back assumes it equals the slot the object will get. That holds because
the scenario is read into cleared heaps and `Allocate` takes the lowest free slot. `CCPtr` already
depends on this.

So a plain pointer can be set at parse time to the slot's address, `heap.Raw_Ptr(index)`, even when
that slot has no object yet. That is exactly what the `CCPtr` would point at once dereferenced. No
second pass is needed. (The old-format path already has one for names: `PendingTriggerName`,
`trigtype.cc:2074`.)

What `CCPtr` never did is check that the slot gets filled. An index past the last trigger type
leaves a pointer to an empty, zeroed slot, and an index past `Length()` fails a `DCHECK`. The plan
adds a check after the scenario is read (step 4).

### The comparison operators

`operator>` (in `ccptr.cc`) and `<`, `<=`, `>=` (built on it) compare by `Name()`, case-insensitive.
Nothing calls them. The editor sorts its lists with `PNBubble_Sort` (`core/jshell.h:201`), which
compares `->Name()` itself. `TListClass` needs only `operator==` (`Remove_Item`,
`Set_Selected_Index`), and pointer equality already gives that.

## Plan

Each step builds both games, passes `ctest`, and is committed on its own. Run `tools/strict_tu.py`
on the files a step touches while working, and do a full strict build (`--parallel 14`) before each
commit.

### Step 0: commit this plan

- [ ] Commit `docs/CCPTR_REMOVAL_PLAN.md` alone and add a "Current Work" line to the memory index.

### Step 1: slot lookup and the save wrapper

- [x] `game::TFixedIHeapClass<T>::SlotAt(int index)` in `game/core/heap.h`: returns `Raw_Ptr(index)`
      for `0 <= index < Length()`, `nullptr` otherwise (including -1). Tested in
      `game/core/heap_test.cc`.
- [x] `game::HeapSlotPtr<T>` in `game/core/heap_slot_ptr.h` (not `ra/saveload/serialize.h`: it needs
      nothing from Red Alert, so it sits beside the heap and is tested without `ra_engine`). Write:
      `heap_.ID(ref_)`, or -1 for null. Read: `heap_.SlotAt(id)`, and `ar.Fail` when `id != -1` and
      that is `nullptr`. The heap is passed explicitly, so there are no statics to bind:
      `ar(HeapSlotPtr(Trigger, TheObjectHeaps().trigger()))`. T may be const.
- [x] `game/core/heap_slot_ptr_test.cc`: null is `FF FF FF FF`, slot 2 is `02 00 00 00` (the
      little-endian int32 `CCPtr::Serialize` wrote), const pointers, an empty slot reading as its
      address, and indices past the end or below -1 failing the reader.

### Step 2: type pointers

The 12 `Class` members in the first group of the table.

- [x] Change each to `const XTypeClass* Class = nullptr;`. `AnimClass::Class` stays non-const.
- [x] Serialize with `game::HeapSlotPtr(Class, TheTypeHeaps().<kind>())` in `saveload/ioobj.cc`.
- [x] Fix what the compiler reports. It was much less than expected: making 11 of the 12 members
      `const` broke nothing, so no code wrote through them, and no `static_cast` unwrapped a type
      pointer. Five `Class.Is_Valid()` calls (`~InfantryClass`, `~UnitClass`, `~VesselClass`,
      `UnitClass::Fire_At`, `VesselClass::Fire_At`) became `!= nullptr` / `CHECK_NE`.
- [x] `unit.cc` reads `Class->PrimaryWeapon` directly again: with a plain pointer the analyzer has
      no null branch to follow, in shallow mode or deep.
- [x] `readability-redundant-nested-if` now sees the unit and vessel recruiting loops in
      `TeamClass::Recruit` (`ai/team.cc`) as plain comparisons and asked for their ifs to be merged.
      (Separately, those loops add `best` inside the loop rather than after it; that is legacy
      behaviour, left alone.)

### Step 3: saved object pointers

The 10 members in the second group of the table.

- [ ] Change each to `XClass* Member = nullptr;` (non-const: houses, factories, teams and triggers
      are game state).
- [ ] Serialize with `HeapSlotPtr(Member, TheObjectHeaps().<kind>())`.
- [ ] `delete static_cast<FactoryClass*>(Factory)` becomes `delete Factory`, and likewise
      `base::DeletePooled(static_cast<TriggerClass*>(Trigger))`, the
      `static_cast<HouseClass*>(House) == ThePlayer()` comparisons and the rest of the 14 unwraps.

### Step 4: INI-parsed pointers

The 4 members in the third group.

- [ ] Change the types as in step 3.
- [ ] Replace each `X.Set_Raw(n)` with `X = TheObjectHeaps().<kind>().SlotAt(n)`.
- [ ] After `TriggerTypeClass::Read_INI` in `Read_Scenario_INI`, walk the team types and trigger
      types. Any reference whose slot holds no live object (`Logical_ID(ptr) < 0`) is reset to
      `nullptr` with a `DLOG(WARNING)`. This is a deliberate change: before, such a reference read a
      zeroed slot. It should never fire on the shipped scenarios. Confirm that with the logic logs,
      and if it does fire, record which scenario here.
- [ ] Serialize `TEventClass::Team`, `TActionClass::Team`/`Trigger` and `TeamTypeClass::Trigger`
      with `HeapSlotPtr` (`saveload.cc`'s trigger/team-type paths).

### Step 5: editor lists

- [ ] `TListClass<CCPtr<TeamTypeClass>>` becomes `TListClass<TeamTypeClass*>`, and the trigger-type
      list likewise. Drop the `CCPtr<...>(...)` wraps at `mapedtm.cc:284`, `:294` and
      `mapeddlg.cc:2690`, `:2696`.

### Step 6: delete CCPtr

- [ ] Delete `ra/core/ccptr.h` and `ra/core/ccptr.cc` (the CMake globs pick it up).
- [ ] Remove the `Heap` definitions and `BindHeap` calls from `core/object_heaps.cc` and
      `core/type_heaps.cc`. If those constructors end up empty, default them.
- [ ] Remove the leftover `#include "ra/core/ccptr.h"` lines, which `misc-include-cleaner` flags.
      Headers that got `game/core/heap.h` only through `ccptr.h` include it directly.
- [ ] Update the comments that name `CCPtr`: `game/core/heap.h:47`, `:53`, `core/object_heaps.h:35`,
      `core/type_heaps.h:21`, and the file comments of `object_heaps.cc` and `type_heaps.cc`. The
      mentions in older plan docs and `CLANG_TIDY_PRIORITIES.md` are history and stay.

## Verification

Before step 1, build the pre-plan binary and keep it as the reference.

- **Every step:** `cmake --build build --parallel 22`, `ctest`, `tools/strict_tu.py` on the touched
  files, and a full strict build before the commit.
- **Steps 2–4:** `tools/ra_saveload_smoke.sh <binary>` on SCG01EA and SCU05EA, and `--load-fixture`.
  The fixture save was written by an older binary, so it shows the format did not change. Also
  cross-load: save SCU05EA at frame 60 with each binary, load it with the other, and compare the
  unit and vessel positions to frame 120 with a continuous run. (Save files cannot be compared byte
  for byte: two runs of the same binary already differ.)
- **Steps 2–4:** headless logic logs, old against new, 600 frames on RA SCU05EA (line-identical
  old-vs-old). SCG10EA is also useful, ignoring its known aircraft noise. Run the old binary twice
  first to measure the noise (memory: headless-logic-diffs).
- **Step 4:** load every shipped RA scenario headless (`-QUITFRAME1`) with `DLOG` on, and confirm
  the new empty-slot check never fires.
- **Step 5:** by hand, on a real display: in the scenario editor, open the team-type and
  trigger-type lists. Confirm they are sorted by name, that the current entry is selected, and that
  edit and delete work.

## Out of scope

- **`AnimTypeClass::Stages`/`LoopEnd` as a lazily filled cache.** These could become `mutable` or be
  computed when the type loads its image, which would let `AnimClass::Class` be const. It is a
  separate change.
- **`TeamTypeClass::Number`**, the live-team count stored on the type. It could move to a count over
  `TheObjectHeaps().team()`, but that is unrelated to `CCPtr`.
- **Sharing the save wrappers with Tiberian Dawn.** TD resolves type pointers by enum through
  `As_Reference`, a different scheme. Merging the two belongs with the rest of the `game/`
  consolidation (docs/GAME_NAMESPACES_PLAN.md).
- **`ObjectPtr`'s TARGET encoding** stays as it is.

## Progress

| Step | Commit        | Notes                                                                                                                                                                  |
| ---- | ------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| 1    | d8309a79      | `SlotAt` and `game::HeapSlotPtr` with tests; the wrapper moved to `game/core/`.                                                                                        |
| 2    | (this commit) | Type pointers. Logic logs: SCU05EA line-identical, SCG10EA aircraft noise only (138 lines vs 100 old-vs-old). Smoke tests, `--load-fixture` and both cross-loads pass. |
