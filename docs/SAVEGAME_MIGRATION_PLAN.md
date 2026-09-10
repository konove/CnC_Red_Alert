# Migration plan: field-wise save games, delete `NoInitClass`, enable `cppcoreguidelines-pro-type-member-init`

## Resume checkpoint (2026-09-10)

- Steps 0–16 are committed; step 16 is `3dc9d787` (UnitClass/VesselClass). `45013c52` fixes `-NOMOVIES`.
- Step 17 is implemented and verified. Save version is **15**.
  Map/Cell, crate timers, radar state, and sidebar production entries now use field-wise serialization.
  The live map and cell array stay constructed; repair/sell/targeting modes and transient UI animations reset.
- `Init_Cells()` must run before restoring map fields because it clears `TotalValue`.
- `LinkClass`'s NoInit constructor remains until step 18: the raw Carryover load still calls it. The other
  map/UI NoInit constructors, plus VectorClass and TargetClass, are removed in step 17.
- Next implementation step is **18: RA globals**. Keep remaining raw-image globals and their NoInit paths
  intact until then; heap infrastructure cleanup remains step 19.
- Validation: strict build of both games and **148 CTest tests** pass, including four new crate tests.
  Headless smoke tests pass for `SCG01EA` and `SCU01EA` (240 matching vehicle/vessel positions each) and
  `SCG02EA` (120 matching positions). A real-display playthrough remains outstanding.

## Context

The next item in `docs/CLANG_TIDY_PRIORITIES.md` is `cppcoreguidelines-pro-type-member-init` (Tier 2, step 6). A
sweep on 2026-09-04 (clang-tidy 23.1.1, 854 TUs, `cmake-build-strict-ra-clang`) measured **383** sites: 138
`NoInitClass` constructors, 191 other constructors, 54 uninitialized local record types.

The 138 cannot be fixed in place. Save/Load in `ra/heap.cc`, `ra/iomap.cc`, `ra/saveload.cc` (and the TD twins)
read a raw `sizeof(T)` image over each object and then run `new (ptr) T(NoInitClass())` purely to rewrite the vtable
pointer. Any constructor that initializes members destroys the loaded bytes; any that does not is what the check
flags. The user chose the real fix: **replace the raw-image format with field-by-field serialization, delete
`NoInitClass`, then enable the check with zero suppressions.** Both games, RA first. No compatibility with old saves.

Side benefits found during exploration, all fixed by this work: this build cannot currently load its own saves
(`base::ssize` written vs `int` read at `ra/saveload.cc:266-288` / `:761-786` and `ra/ioobj.cc:401` / `:361`);
`ObjectClass`/`TeamClass` NoInit ctors null a pointer that was just loaded (`ra/object.h:143`, `ra/team.h:207`);
TD's load resets `CrewClass::Kills`, four `HouseClass` timers, `Regions[]` and sidebar strips; `EventClass`
non-default ctors leave `IsExecuted` uninitialized in both games; the save format depends on struct layout and
pointer width; saving mutates live objects (`Code_All_Pointers`).

## Design (decided)

### Archive layer — new `src/tech/archive.h` (header-only) + `src/tech/archive_test.cc`

One `template <class Archive> void Serialize(Archive& ar)` per class, instantiated for two archives. Read and write
share one field list, so read/write asymmetry cannot recur.

```cpp
template <class T>
concept ArchiveScalar = (std::integral<T> || std::is_enum_v<T>) &&
                        !std::same_as<T, long> && !std::same_as<T, unsigned long>;  // forces the CLAUDE.md int migration

constexpr uint32_t FourCC(const char (&tag)[5]);

class ArchiveWriter {           // over Pipe&
 public:
  static constexpr bool kIsReading = false;
  explicit ArchiveWriter(Pipe& sink);
  template <class... Ts> void operator()(Ts&&... fields);     // (Field(fields), ...)
  void Section(uint32_t tag);                                  // 4-byte marker between top-level blocks
  void Bytes(const void* data, int size);                      // escape hatch (mixed phase only)
 private:
  template <ArchiveScalar T> void Field(T& v);                 // ints by sizeof; enums always int32_t (std::to_underlying); bool 1 byte
  template <class T> requires requires(T& t, ArchiveWriter& w) { t.Serialize(w); } void Field(T& v);
  template <class T, base::ssize N> void Field(T (&arr)[N]);   // element-wise; char[N] = N raw bytes
};
class ArchiveReader {           // over Straw&; same overloads, plus:
  static constexpr bool kIsReading = true;
  bool Section(uint32_t tag);   // false + Fail() on mismatch
  bool ok() const; std::string_view error() const; void Fail(std::string_view why);  // first error wins, later reads zero-fill
};
template <class T> concept Serializable = requires(T& t, ArchiveReader& r, ArchiveWriter& w) { t.Serialize(r); t.Serialize(w); };
```

- On-disk little-endian; `if constexpr (std::endian::native == std::endian::big) v = std::byteswap(v);`.
- Bit-fields: read into locals, `ar(...)`, assign back. Unions: serialize the integer alias (`Flag.Composite`,
  `Data.Value`).
- Class `Serialize` bodies live in `.cc` (RA: `ra/ioobj.cc`, where Code/Decode_Pointers live today) with explicit
  instantiations for both archives; headers only declare the template. Only `heap.h` includes `tech/archive.h`.
- Primitives get their own `Serialize`: `fixed` (`ar(raw_)`), `Timer<T>`/`Stopwatch<T>` (write `Value()` +
  running; on read re-anchor `start_tick_ = T::Tick()` — **requires `Frame` restored before any object is read**),
  `CCPtr<T>` (`ar(ID)` + range check via `Fail`, not `CHECK`), `FacingClass`.
- Object pointers use proxies in new `src/ra/serialize.{h,cc}`: `ar(ObjectPtr(Next))`, `ar(TechnoTypePtr(x))`.
  Writer emits `p && p->IsActive ? p->As_Target() : kTargetNone`; reader resolves **inline** via kind check on the
  TARGET then `As_Object()` (`ra/target.cc:295`, pure address arithmetic on `Raw_Ptr`, valid for slots not yet
  loaded) + `static_cast`. Add `Is_Target_Techno/Foot(TARGET)` next to `Is_Target_Object` (`ra/target.h:102`).
  Never use `As_Techno()` here (dereferences a possibly unconstructed slot).
- Result: `Code_All_Pointers` **and** `Decode_All_Pointers` (`ra/saveload.cc:1229-1386`) and every per-class
  `Code_Pointers/Decode_Pointers` are deleted at the end. Saving no longer mutates live objects. The
  `PendingObject`/`Set_Cursor_Shape`/`Whom`/`CurrentObject` fixups move to the tail of `Load_Game`.
  `HouseClass::Decode_Pointers`' `Init_Data(...)` call is dropped (its only remaining effect clobbers
  `Control.InitialCredits`). "Houses last/first" ordering is stale (`HouseClass::Code_Pointers` is empty).

### Heap loader — `ra/heap.h`/`heap.cc`

Concept dispatch so heaps flip one class at a time with a working game at every commit:

```cpp
template <class T> concept RawImage = std::constructible_from<T, const NoInitClass&>;   // deleted at the end
int Save(Pipe&) const requires (Serializable<T> || RawImage<T>);   // non-virtual; nothing overrides
int Load(Straw&)      requires (Serializable<T> || RawImage<T>);
// per object, after FreeFlag/ActiveCount/ActivePointers bookkeeping (heap.cc:560-565):
if constexpr (Serializable<T>) { new (ptr) T(); ptr->Serialize(reader); if (!reader.ok() || ptr->ID != idx) return false; }
else                            { file.Get(ptr, sizeof(T)); new (ptr) T(NoInitClass()); }
```

- **Shell constructor**: `protected: T() = default; friend class TFixedIHeapClass<T>;` with NSDMI on every member,
  in every class of the chain down to `AbstractClass` (bases currently only have `(RTTIType, int)` ctors,
  `ra/object.cc:187`). It must be side-effect-free (no `Tracking_Add`, no layer registration); the real ctors keep
  their side effects. `FactoryClass()`, `TeamTypeClass()`, `TriggerTypeClass()` already exist; `TeamClass` and
  `TriggerClass` lose their unused default arguments. This is the only default ctor the check allows: fully
  initializing via NSDMI. `IsActive` NSDMI must be `true` (`operator new` sets it before the ctor;
  `ra/house.cc:610` etc.) — `Serialize` overwrites it anyway.
- The `requires` stops instantiating `Load` for the 15 never-saved type heaps (`heap.cc:637-651`), so the 15
  `type.h` NoInit ctors go in the infra commit. Delete the `BuildChoiceClass` no-op specializations
  (`ra/house.cc:215-229`).
- Split `heap.cc`: member templates into `heap.h`, the 32 explicit instantiations into new `ra/heap_instances.cc`,
  so `ra_heap_test` can link `heap.cc` with a test-local type.
- During the mixed phase `TFixedIHeapClass<T>::Code/Decode_Pointers` are no-ops for `Serializable<T>`.

### Versioning / header (`ra/saveload.h`)

`inline constexpr uint32_t kSaveGameMagic = FourCC("RASV"); inline constexpr int32_t kSaveGameVersion = 1;` — bump
on every format change (every migration commit). Header: `description[kDescripMax]` (must stay first, load dialog
reads it), magic, version, scenario `int32`, house `int32`, digest[20]. Accept only `version == kSaveGameVersion`
(drop the `+1/-1` dance `:469-471`, `:608`, `:1445`). Body: `Section("FRAM")` + `Frame` first, then `Section` tags
around each top-level block (~30). No per-object tags. Keep LZO→Blowfish→SHA pipe chain.

### Serialize-vs-rebuild rule (heap objects)

Serialize every non-static member in declaration order, including transient flags. Only pointers to memory that is
not a fixed heap slot are skipped and rebuilt in the `if constexpr (Archive::kIsReading)` branch. Known:
`HouseClass` `UnitTrackerClass*` ×12 (extract `Init_Trackers()` from `house.cc:836-860`, call from ctor and reader
branch, NSDMI `nullptr`); `HouseClass::Regions[]` IS state (serialize); `FactoryClass::House` → `CCPtr<HouseClass>`
(`factory.h:153`). Pointer proxies needed for exactly: `ObjectClass::Next`, `RadioClass::Radio`,
`CargoClass::CargoHold`, `FootClass::Member`, `TeamClass::Member`, `BulletClass::Payback`, `FactoryClass::Object`,
`TeamMemberClass::Class`. `long` members become `int32_t`/`int64_t` in the commit that migrates their class
(`ArchiveScalar` rejects `long` at compile time).

### Map / Cell (`ra/iomap.cc`)

Key finding: every gadget in the `MouseClass` chain is already `static` (`display.h:296`, `radar.h:183`,
`power.h:90`, `sidebar.h:292-332`). Once Load stops overwriting `this`, the global `Map` stays constructed:
`new (this) MapEditClass(NoInitClass())`, `Free_Cells()`, `Alloc_Cells()` around the load go; `MapClass::Alloc_Cells`
(`map.cc:498`) becomes `Array.Resize(Size)`; `SidebarClass(const NoInitClass&)` (`sidebar.cc:244-253`) goes.

Members to serialize (everything else is reset/recomputed, see `ResetTransientUiState()` helper):
- `MapClass`: `MapCellX/Y/Width/Height`, `TotalValue`, `TiberiumGrowth[]`+Count+Excess, `TiberiumSpread[]`+Count+Excess,
  `TiberiumScan`, `Crates[256]` sparse (count + index + `Cell` + `CrateTimer` for `Is_Valid()`).
- `DisplayClass`: `TacticalCoord`, `DesiredTacticalCoord`, `ZoneCell`, `ZoneOffset`, `PendingObjectPtr` (proxy),
  `PendingHouse`. Reset `IsRepairMode/IsSellMode/IsTargettingMode` (today they survive while the buttons are reset).
- `RadarClass`: `DoesRadarExist`, `IsRadarActive`, `IsRadarJammed`, `IsZoomed`, `IsPlayerNames`, `IsHouseSpy`, `SpyingOn`,
  `ZoomFactor`, `RadarX/Y`, `RadarCell`, `RadarCellWidth/Height`, `BaseX/Y`, `RadarWidth/Height`.
- `SidebarClass`: `IsSidebarActive`; per `Column[i]`: `IsBuilding`, `Flasher`, `TopIndex`, `BuildableCount`,
  `Buildables[0..BuildableCount)` = {`BuildableID`, `BuildableType`, `Factory`}.
- `PowerClass`, `TabClass`, `HelpClass`, `ScrollClass`, `MouseClass`, `MapEditClass`, `GScreenClass`: nothing.
- `CellClass`: flag bits packed in one `uint8`, `Jammed`, `Trigger` (CCPtr), `TType`, `TIcon`, `Overlay`, `OverlayData`,
  `Smudge`, `SmudgeData`, `Owner`, `InfType`, `OccupierPtr` (proxy), count + `Overlappers[]` (proxies),
  `Flag.Composite`, `Land`. Not `ID` (loop supplies it), not `Zones[]` (`Post_Load_Game` → `Zone_Reset`). Keep the
  sparse count+CELL scheme; drop the `Zones` test from `Should_Save` (`iomap.cc:94-98`).

Read sequence: `Reset_Theater_Shapes`/`Init_Theater(Scen.Theater)` + 11 `XTypeClass::Init` (unchanged,
`iomap.cc:246-269`) → members → transient resets → `Init_Cells()` → count → `(CELL, cell.Serialize)` loop →
`LastTheater = Scen.Theater`. Post-load repairs that stay: `Map.Init_IO()` (re-adds the static buttons that
`Clear_Scenario` removed), `Flag_To_Redraw`, `Post_Load_Game`, `Reload_Sidebar` (after `Load_Misc_Values`, needs
`PlayerPtr`), the PendingObject/cursor fixup.

### RA globals

- `ScenarioClass Scen`: everything except `IsFadingBW/IsFadingColor/FadeTimer`, `AutoSonarTimer`,
  `bLocalProposesDraw/bOtherProposesDraw`. Flags packed into `uint16`. `RandomClass` via `seed()`.
- `ScoreClass`: all counters (`ElapsedTime` → `int64_t`), `RealTime` via Stopwatch value semantics; not `ChangingGun`.
  Delete `Code/Decode_Pointers` and the declared-only Load/Save (`score.h:75-76`).
- `CarryoverClass`: drop the `LinkClass` base; global `std::vector<CarryoverClass> Carryover` (`globals.cc:607`);
  callers are five loops in `scenario.cc:613-617, 946-1015` and `saveload.cc:808-834`. Removes `Zap()` and the NoInit ctor.
- `SpecialClass`: lives in the `EventClass` union → must stay trivially constructible, no ctor; `Serialize` writes one `uint8` bitmask.
- `OptionsClass`/`GameOptionsClass`: serialize `GameSpeed` only (raw image today restores the saver's key bindings and
  volumes into the loader — a bug). Call out in the commit message.
- `ChronalVortexClass`: state fields only; remap tables (3.8 KB) and `RenderBuffer` are caches → on read
  `Theater = THEATER_NONE; Setup_Remap_Tables(Scen.Theater)`. Delete the null/restore dance (`vortex.cc:311-353`).
- `SessionClass`/`NodeNameType`: one shared field list for the Pipe and CCFileClass variants; write `Type` in both;
  `NodeNameType` = `Name`, `Address` bytes, Player arm of the union.
- `BaseClass`: `House`, count, per node `Type`, `Cell`; drop the `sizeof(*this)` guard.
- Trigger vectors, `LayerClass`, `CurrentObject`: count `int32` + TARGET via the archive.
- Recording path (`init.cc:2636-2679`): wrap `CCFileClass` in `FilePipe`/`FileStraw` and reuse the same `Serialize`s.

### Tiberian Dawn

TD uses `RawFileClass` directly, no compression. **Wrap, don't template**: in `td/saveload.cc` `Save_Game`/`Load_Game`
keep the raw header on the file, then `FilePipe fpipe(file)` / `FileStraw fstraw(file)` (`RawFileClass` derives from
tech `FileClass`) and pass archives below. `Get_Savefile_Info` stays raw.

Where copy-paste from RA breaks: version `unsigned long` sizeof-sum → `int32_t` constant; `MouseClass::Save` seeks
back to patch the cell count (`:431-467`) → pre-count pass; `Load_Misc_Values` optional trailing `ActionMovie` via
seek (`:698`) → always written; theater lives in `DisplayClass::Theater` (no `Scen`) → theater-first in
`MouseClass::Serialize`; `CellClass::IsTrigger` + global `CellTriggers[]` side table → `if (IsTrigger) ar.Target(...)`
after the cell fields; TD `Should_Save` misses seven fields → adopt RA's test; type refs are raw `const XTypeClass*`
(RA uses `CCPtr`) → helper writing the enum, resolving via `As_Reference`; `BuildingClass::Factory` coded as
`Factories.ID+1` → `int` index; house refs → `HousesType`; `TechnoClass` has a sixth base `CrewClass` (serialize
`Kills`); `TurretClass`/`TarComClass` layers; `TCountDownTimerClass` (`td/ftimer.h`, `long Started, DelayTime`) gets
its own `Serialize`; `HouseClass` serializes `Regions[]`, four timers, `UnitTrackerClass*` ×10 via `Init_Trackers()`;
`TriggerClass` flat (no virtuals), keep the `HouseTriggers` rebuild in `saveload.cc:452-457`, delete the duplicate in
`ioobj.cc:393-397`; private `StageClass` bases (Anim, Factory) → `StageClass::Serialize` public; TD has no
Scen/Carryover/Vortex/Session/Special/Options save path; `Frame` global is `long` → decide `int64_t` once for both games.

TD deletions: `Read_Object`/`Write_Object`/`Get_VTable`/`Set_VTable` (`saveload.cc:932/:987/:1181/:1202`); 13
`static void* VTable` members and their capture sites in `*::Init()` (list in exploration notes: `aircraft.cc:1008`,
`anim.cc:519`, `building.cc:2105`, `bullet.cc:594`, `infantry.cc:915`, `overlay.cc:129`, `smudge.cc:200`,
`team.cc:157`, `teamtype.cc:181`, `template.cc:253`, `terrain.cc:434`, `unit.cc:3063`, `mouse.cc:298`); all 16 dead
`T::Load` bodies + one-liner Saves in `ioobj.cc`; `FuseClass::Fuse_Write/Read`; `StripClass::Load/Save` decls
(`sidebar.h:155-156`); `td/session.h`, `td/wwfile.h` (never compiled/unused); `td/heap_layout_test.cc`; 50 NoInit ctors.

### Check-enable phase (last)

- `.clang-tidy`: delete line 142 (`-cppcoreguidelines-pro-type-member-init`) and line 164 (`-hicpp-member-init`, its
  alias). No `CheckOptions` entry: `IgnoreArrays` stays false (fix arrays with `{}`), `UseAssignment` false.
- Rules (the diagnostic names exactly the fields to touch; touch only those):
  - R0 never add `{}` to a member whose type has a user-provided default ctor (`CCPtr`, `Timer`, ...) —
    enabled `readability-redundant-member-init` flags it.
  - R1 pointers `= nullptr` (`modernize-use-nullptr` rejects `= 0`); scalars `= 0`/`= false`; enums the sentinel the
    other ctor uses; else `{}`. Bit-fields: `unsigned IsExecuted : 1 = 0;`.
  - R2 body assignments already count; do not convert bodies to init lists, just add the missing members.
  - R3 "does not initialize these bases: X" → give X's members NSDMI, unless X must stay trivial (lives in a union):
    then `: X()` in each derived init list (`xTargetClass` in `ra/target.h` ×7).
  - R4 `IsActive` initializes to **`true`** (`operator new` sets it before the ctor: `ra/abstract.h:64`,
    `ra/house.cc:651`, `factory.cc:89`, `teamtype.cc:201`, `trigger.cc:149`, `trigtype.cc:114`; td equivalents).
    `td/power.cc:87 IsActive` is unrelated UI state: `false`.
  - No NSDMI may land in a class that still has a NoInit ctor (it would clobber the raw image). Sequencing is the guard.
- Category fixes: (a) pure NSDMI ~31 (`ra/base.h:57`, `tech/lzw.h:61`, `winvq/vqa32/vqaplayp.h:393`, anonymous union
  in `td/nodename.h:9` → NSDMI on one variant); (b) helper/memset/strncpy ~34 → `{}` on the array/struct member,
  helper stays (`tech/field.h:77 char ID[4]{}` clears 8; six pipe/straw `BlockHeader` structs get in-struct
  initializers and lose the body line; `tech/int.h` `reg[]{}`; `ra/ipxaddr.h`; `tech/sha.h`); (c) unions:
  `EventClass` NSDMI on every field + `} Data{};`, keep `memset(this)` + its `cert-oop57-cpp` NOLINT (reworded), and
  all 23 non-default ctors delegate `: EventClass()` (delegating ctors are exempt; fixes `IsExecuted`);
  `CellClass` `} Flag{};` + `Overlappers[]{}`; composite-union locals get `{}` at the site (`ra/inline.h` ×7,
  `ra/coord.cc` ×5, `ra/defines.h:323`); (d) empty copy ctors `td/cell.h:266`, `td/ccfile.h:121` → `= delete`;
  (f) ~120 ordinary ctors → NSDMI for constants, init list for arg-dependent values; 54 locals → `{}` at site, or
  NSDMI on the type when it is ours and flagged ≥3× (`sdllib/file.h:76 FindFileState` clears 6, `ra/defines.h:2825
  PathType` clears 3, `sdllib/font.h:55 FontHeader`, `sdllib/net_select.cc:13 SocketInfo`); replace `memset(&x,0)`
  with `= {}` (`ra/dialog.cc:876,922`, sockets, `stat` buffers).
- Optional accelerator: `run-clang-tidy-23 -p cmake-build-strict-ra-clang -checks='-*,cppcoreguidelines-pro-type-member-init' -fix -j $JOBS src/tech`
  (dedups header edits; still hand-review every enum/sentinel/`IsActive`; never on `event.*`, `target.*`, `cell.*`).

## Commit sequence

Every commit builds `rasdl`+`tdsdl` under the strict config, passes `ctest`, and (from step 4 on) passes the in-game
save/load smoke test. Bump `kSaveGameVersion` in every format-changing commit.

**Phase 0 — baseline (RA)**
0. Fix the count bugs (`int32_t` both sides, `saveload.cc:266-288/:761-786`, `ioobj.cc:361/:401`); drop
   `Next(nullptr)` (`object.h:143`) and `Member(nullptr)` (`team.h:207`). Smoke: save+load now works — the baseline
   for everything after. **Not** `crate.h:50`/`super.h:51`: their default ctors are de facto NoInit paths for
   `MapClass::Crates[256]` and `HouseClass::SuperWeapon[]`, which the raw image still restores; full initialization
   there would wipe crate and superweapon state on every load. They become `= default` + NSDMI in commits 9 (super) and
   17 (crate).

**Bridge found after commit 4 (`a88622e9`).** `fixed` had a zeroing default constructor, so every `fixed` member a
NoInit constructor did not name was reset by the post-load placement-new: loaded vehicles had `SpeedBias` 0 and never
moved. `fixed` got a NoInit constructor and the raw classes forward it; the same audit found unlisted timers
(`TechnoClass::CloakingDevice`, `HouseClass::RepairTimer`, `VesselClass` countdowns) and array members that cannot be
forwarded and are still reset on every load until their class migrates: `MapClass::Crates[]` (commit 17),
`HouseClass::Regions[]` and `SuperWeapon[]` (commit 9), `TeamTypeClass::MissionList[]` (commit 7),
`ScoreClass::RealTime` (commit 18). Any type given a default member initializer while still inside a raw image
reintroduces this bug; the rule "no NSDMI in a class with a NoInit constructor" applies to member types too.

**Headless smoke test (`4071a5f0`).** `rasdl -LOADGAME<n> -QUITFRAME<f> [-SAVESLOT<m>]` with
`SDL_VIDEODRIVER=dummy` loads a slot, logs every unit's coordinate per frame, and saves before quitting. Run it after
every flip commit: load the reference save, run 30 frames, save to a scratch slot, load that, and check the moving
units keep moving. The in-game test on a real display is still the final check, but this catches the freeze class
without a keyboard.

**Phase 1 — infrastructure**
1. `tech/archive.h` + `archive_test.cc` (widths, enums→int32, bool, char arrays, nested, LE byte layout, short read → `!ok()`, Section mismatch, FourCC).
2. `fixed`, `Timer`, `Stopwatch` `Serialize` + tests (`FakeTick` source; "advancing the clock between write and read keeps `Value()`").
3. RA infra: heap split (`heap_instances.cc`) + concept dispatch + `requires`; `ra/serialize.{h,cc}` proxies;
   `CCPtr`/`FacingClass::Serialize`; `Is_Target_Techno/Foot`; magic/version header + `Frame` first + `Section` tags;
   delete 15 `type.h` NoInit ctors + `BuildChoiceClass` specializations; `ra_heap_test`. No heap flipped yet.

**Phase 2 — RA heap objects, one row per commit** (each: add `Serialize` + new bases/mixins, migrate `long`s, NSDMI +
shell ctor, delete that class's NoInit ctor(s), smoke test)

| # | Heap type(s) | Bases/mixins gaining `Serialize` | NoInit ctors deleted |
|---|---|---|---|
| 4 | FactoryClass (House → CCPtr) | StageClass | factory.h:59 |
| 5 | TriggerClass | TDEventClass | trigger.h:71, tevent.h:121 |
| 6 | TriggerTypeClass | AbstractTypeClass, TEventClass, TActionClass | trigtype.h:111, tevent.h:153, taction.h:134 |
| 7 | TeamTypeClass | TeamMissionClass, TeamMemberClass | teamtype.h:122, type.h:92 |
| 8 | TeamClass | AbstractClass | team.h:207 |
| 9 | HouseClass (Init_Trackers) | HouseStaticClass, SuperClass (`super.h:50-51` → `= default` + NSDMI), RegionClass | house.h:642, :78, :892 |
| 10 | Template, Overlay, Smudge | ObjectClass | template.h:73, overlay.h:74, smudge.h:76 |
| 11 | AnimClass, TerrainClass | — | anim.h:69, terrain.h:78 |
| 12 | BulletClass | FlyClass, FuseClass | bullet.h:87, fuse.h:55 |
| 13 | BuildingClass | MissionClass, RadioClass, TechnoClass, FlasherClass, CargoClass, DoorClass, CrewClass | building.h:235 |
| 14 | InfantryClass | FootClass | infantry.h:132 |
| 15 | AircraftClass | — | aircraft.h:83, fly.h:62 |
| 16 | UnitClass, VesselClass | DriveClass | unit.h:139, vessel.h:95, drive.h:102, foot.h:283, techno.h:257, radio.h:82, mission.h:79, object.h:143, abstract.h:69, stage.h:70, flasher.h:66, cargo.h:56, door.h:76, crew.h:59 |

Commit 16 also deletes all per-object `Code/Decode_Pointers`, the heap loops in `Code_All/Decode_All_Pointers`, and
the `RawImage` branch rows of `ra/heap_layout_test.cc`.

**Phase 3 — RA Map/Cell and globals**
17. `CellClass` + `MouseClass` chain (`iomap.cc`), `CrateClass` (`crate.h:50` → `= default` + NSDMI), `VectorClass`/`MapClass::Array` NoInit, `SidebarClass` NoInit, UI-chain NoInit ctors (gscreen…mapedit, gadget, control, credits), `TargetClass` NoInit. Keep `LinkClass` NoInit until Carryover migrates in step 18.
18. `Scen`, `Score`, `Carryover` (→ `std::vector`), `Special`, `Options` (GameSpeed only), `ChronalVortex`, `Session`/`NodeNameType`, `Base`, trigger vectors, layers, misc/MP values, recording path. Delete `Code_All/Decode_All_Pointers` drivers and the now-unused `LinkClass` NoInit constructor; move fixups to `Load_Game` tail.
19. RA cleanup: `CCPtr`/`Timer`/`Stopwatch` NoInit ctors (`tech/ftimer.h` is RA-only; TD uses `td/ftimer.h`), `RawImage` constraint, `ra/heap_layout_test.cc` + its `add_gtest`, every `#include "tech/noinit.h"` in `src/ra` and `src/tech`, `SAVEGAME_VERSION` macro. Add `RA_SAVE_DUMP=<path>` env tee of the plaintext stream (before LZO) as a debugging aid.

**Phase 4 — TD** (mirrors Phase 1–3; ⚠ = not copy-paste, see TD section)
20. Plumbing ⚠: FilePipe/FileStraw wrap, `int32_t` version constant, seek-free cell count, unconditional `ActionMovie`, all `Save/Load(FileClass&)` signatures → archives. Still raw bytes inside: checkpoint.
21. Leaf value types: `TCountDownTimerClass`, `FacingClass`, `StageClass` (public), `FlyClass`, `FuseClass` (delete Fuse_Read/Write), `CargoClass`, `DoorClass`, `FlasherClass`, `CrewClass`, `CloakingClass`; type-ref helper ⚠.
22–27. Heap hierarchy in the same base-first order as RA (Techno + Crew ⚠, Turret/TarCom ⚠, Building Factory index ⚠, Trigger ⚠, House ⚠). Delete `ioobj.cc` bodies, `Read/Write_Object`, `VTable` statics, `heap.cc` raw paths.
28. Map/Cell ⚠ (theater-first, `CellTriggers`, full `Should_Save`).
29. Globals: Score, Base, Layers, Misc.
30. TD cleanup: remaining NoInit ctors incl. `td/ftimer.h:60`, `td/heap_layout_test.cc`, `td/session.h`, `td/wwfile.h`, `td_saveload_test` round-trip (BufferPipe→BufferStraw for Cell, Map members, House, Unit).

**Phase 5 — delete the header, enable the check**
31. Delete `src/tech/noinit.h` and every remaining include (`ra/jshell.h` includes it unused). Re-run the single-check sweep: expect ~245; any *new* site is a rewrite defect to fix, not to suppress.
32. `tech` (29 sites). 33. `sdllib` + `winvq` (15). 34a. `td` behaviour-relevant: `event.*` delegation, `cell.*`, two `= delete`. 34b. `td` rest. 35a. `ra` behaviour-relevant: `event.*`, `target.*`, `cell.*`, composite locals. 35b. `ra` rest (`IsActive(true)` sites named in the message).
36. Enable: `.clang-tidy` lines 142 + 164, doc edits below, full-config sweep, strict build of both targets + `ctest`.

### `docs/CLANG_TIDY_PRIORITIES.md` edits (commit 36)
- `:11-15` overview: six of seven in; only `switch-missing-default-case` left. Fix stale hash `e0e9d2bf` → `5d4b53ee` (here and `:736`; `e0e9d2bf` does not exist).
- `:150-157` Progress §2 table: add the missing `bugprone-narrowing-conversions | 5d4b53ee | 510` row and the new `pro-type-member-init | <hash> | 383` row (138 via NoInitClass deletion, 245 fixed, 0 NOLINT, `IsExecuted` bug).
- `:190-199` lessons bullet: read the check's matcher first — delegating ctors exempt, body assignments count, `memset(this)` does not, base findings fixed at the base, `IsActive` written by `operator new`, union members cannot take NSDMI.
- `:574-583` Tier 2 table: `:580` narrowing → ✅ Done `5d4b53ee`; `:581` `349` → `383`, ✅ Done.
- `:645-647` ⚠️ paragraph → what was done (NoInitClass deleted, not annotated).
- `:689` Type.6 → Enabled; `:696` rules 1–3 and 6. `:737` item 6 strikethrough. `:516` hicpp count 16 → 15. `:825` summary row; `:828` disabled count `→ 240`. `:136`/`:303` mentions of `heap_layout_test.cc` reworded (deleted).

## Files

New: `src/tech/archive.h`, `src/tech/archive_test.cc`, `src/ra/serialize.h`, `src/ra/serialize.cc`,
`src/ra/heap_instances.cc`, `src/ra/heap_test.cc`, `src/ra/crate_test.cc`, `src/td/saveload_test.cc`.
Deleted: `src/tech/noinit.h`, `src/ra/heap_layout_test.cc`, `src/td/heap_layout_test.cc`, `src/td/session.h`, `src/td/wwfile.h`.
Modified (core): `src/ra/heap.h/.cc`, `ra/ioobj.cc`, `ra/iomap.cc`, `ra/saveload.h/.cc`, `ra/ccptr.h`, `ra/target.h/.cc`,
`tech/fixed.h`, `tech/ftimer.h`, the 17 RA heap classes + bases/mixins, the Map chain headers, `ra/scenario.h`,
`score.h`, `carry.h`, `special.h`, `options.h`, `vortex.h/.cc`, `session.h/.cc`, `base.h/.cc`, `init.cc`,
`td/saveload.cc`, `td/ioobj.cc`, `td/iomap.cc`, `td/heap.cc`, TD class headers, `.clang-tidy`,
`docs/CLANG_TIDY_PRIORITIES.md`, `ra/CMakeLists.txt`, `td/CMakeLists.txt`, `tech/CMakeLists.txt`.

## Verification

- Unit: `archive_test`, `fixed_test`, `ra_crate_test` (default state, timer preservation, invalid cells, truncated timer), timer tests, `ra_heap_test` (sparse indices, `ID != idx` rejected, count mismatch
  rejected), `CCPtr` range check, small mixins (`StageClass`, `TDEventClass`, `RegionClass`, `HouseStaticClass`,
  `FlasherClass`), `td_saveload_test`. Game objects and `CellClass` need the full `rasdl` link and are not unit-testable.
- In-game smoke after every flip commit: build `rasdl`, run with real SDL and the Steam MIX files (dummy video driver
  never loads palettes), skirmish vs AI: build, move, fire, save slot N, quit, load N, play 30 s, save N+1; then a
  campaign mission with triggers/reinforcements. With `RA_SAVE_DUMP` set, `cmp` the two plaintext dumps: the first
  differing offset names the class (Section tags); expected diffs only `Frame`, `Score.RealTime`, Help timer.
- Tidy sweeps (zsh: wrap in `bash -c`; `-p cmake-build-strict-ra-clang`; regex `\[[A-Za-z0-9._,-]+\]$`, strip `,-warnings-as-errors`):

```bash
JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2)); BUILD=cmake-build-strict-ra-clang
grep -c -- -Weverything $BUILD/compile_commands.json      # must be > 0
python3 -c "import json; d=json.load(open('$BUILD/compile_commands.json')); fs=sorted({e['file'] for e in d if '_deps' not in e['file'] and '/third_party/' not in e['file']}); open('/tmp/tidy_files.txt','w').write('\n'.join(fs)+'\n')"
# single check, per-site list
bash -c "xargs -a /tmp/tidy_files.txt -P $JOBS -I{} clang-tidy -p $BUILD --quiet --checks='-*,cppcoreguidelines-pro-type-member-init' --warnings-as-errors= {} 2>&1" \
  | grep -oE '^/home[^ ]+ (warning|error): .*\[[A-Za-z0-9._,-]+\]$' | sed -E 's/,-warnings-as-errors\]$/]/' | sort -u > /tmp/member_init_sites.txt
# full config + the new check (no '-*' => appended), must print nothing before the enable commit
bash -c "xargs -a /tmp/tidy_files.txt -P $JOBS -I{} clang-tidy -p $BUILD --quiet --checks='cppcoreguidelines-pro-type-member-init' --warnings-as-errors= {} 2>&1" \
  | grep -oE '^/home[^ ]+ (warning|error): .*\[[A-Za-z0-9._,-]+\]$' | sed -E 's/,-warnings-as-errors\]$/]/' | sed -E 's/^([^ ]+) .*\[([A-Za-z0-9._,-]+)\]$/\2\t\1/' | sort -u | cut -f1 | sort | uniq -c | sort -rn
cmake --build $BUILD --parallel $JOBS && ctest --test-dir $BUILD
```
  Enabled checks likely to fire on new code: `readability-redundant-member-init`, `modernize-use-nullptr`,
  `clang-diagnostic-missing-field-initializers` (partial brace lists), `bugprone-narrowing-conversions` (non-constant NSDMI).

## Risks

- `Frame` must be restored before the first `Timer::Serialize` read (commit 3 makes it first; test in commit 2 documents the contract).
- Mixed phase: invariant "no path calls `X::Code_Pointers()` outside `TFixedIHeapClass<T>::Code_Pointers`" (true today: only `taction.cc:361`, `house.cc:226` elsewhere).
- Inline TARGET resolution + `static_cast` is sound only with the kind check; keep kind tables in one place (`serialize.cc`).
- Explicit instantiation with unsatisfied `requires` (commit 3) — first strict build is the proof.
- Shell ctors: every base default ctor in the chain must be side-effect-free; `IsActive` NSDMI `true`.
- `HouseClass` (~120 members) is the largest field list; Section + `ID` checks and the dump diff find a missed field.
- `Options.GameSpeed`-only and the Display mode-flag resets are intended behaviour changes; state them in the commits.
- Dropping `Zones` from `Should_Save` relies on `Post_Load_Game` always running `Zone_Reset` (both branches, `scenario.cc:681`).
