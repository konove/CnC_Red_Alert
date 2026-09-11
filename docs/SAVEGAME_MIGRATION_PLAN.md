# Migration plan: field-wise save games, delete `NoInitClass`, enable `cppcoreguidelines-pro-type-member-init`

## Resume checkpoint (2026-09-10)

- Steps 0–34a are complete; step 33 is `67677f6f`; step 32 is `d9a92318`; step 31 is `bf27db84`; step 30 is `fbb36dd1`; step 29 is `2689ff4c`; step 28 is `96f23c5b`; step 27 is `b9991065`; step 26 is `1af8d7be`; step 25 is `77e818ab`; step 24 is `3e5db85d`, step 23 is `b24cad08`, step 22 is `f5d3cfb0`, step 21 is `401409ae`, step 20 is `6799acbe` (TD plumbing), step 19 is `1121d8e6`.
- Step 18 is complete. Save version is **16**.
  Scenario, score, Carryover, vortex, layers, selection, trigger lists, and multiplayer globals now use
  field-wise serialization. Carryover is a vector, and the top-level pointer-coding passes are removed.
- Session saves and recordings share their common field list. Only recordings serialize the player roster:
  network save loading must retain connected peers for `Reconcile_Players()`.
- Recordings use `RARC` plus the save-format version to reject incompatible data. Both playback entry
  points honor load failure. Options restore game speed only, preserving local controls/audio/display settings.
- Step 19 is complete. It removes RA's raw-image heap fallback, pointer-coding stubs, layout test, and
  remaining NoInit includes/constructors (including the now-unused shared `fixed` bridge). The NoInit
  header remains for TD. RA's old `SAVEGAME_VERSION` macro was already removed.
- `RA_SAVE_DUMP=<path>` writes the plaintext save body before LZO, starting with `FRAM`; it omits the
  header and digest and replaces the dump on each save. An unset/empty value disables it. Failed dump
  opens or short writes produce debug warnings without interrupting the normal save. Format stays **16**.
- Step 20 is complete: TD now wraps its raw file in FilePipe/FileStraw and passes archives through
  all active save/load routines. The raw header has explicit **TD version 1**; the body starts with
  `FRAM` and an `int64_t Frame`. Object images and pointer-coding passes remain until later steps.
  Sparse cell counts no longer seek, `ActionMovie` is required, and load failures propagate from
  Map, raw-size/index checks, and the trailing fields. An unbuffered checked pipe reports short writes,
  with pointer decoding on every body-save exit. The unused `session.h` stays for deletion in step 30.
- TD supports `-NEWGAME<scenario>`, `-LOADGAME<n>`, `-QUITFRAME<n>`, `-SAVESLOT<n>`, and `-NOMOVIES`.
  `tools/td_saveload_smoke.sh` runs in isolation using a temporary executable symlink because TD changes
  to the executable directory. The smoke run exposed and fixed a shutdown double-free: `Uninit_Game`
  now clears `Palette` after deleting it, before the SDL quit handler calls `Prog_End`.
- Step-20 validation: strict builds of both games and all **156 CTest tests** pass. TD SCG01EA and
  SCB01EA match **120** and **180** unit positions across save/load, respectively. A version-0 header,
  one-byte-truncated ActionMovie, and invalid raw map size are rejected without starting gameplay.
  RA SCG01EA still matches 240 positions.
- Step 21 is complete: field-wise serializers are ready for TD's countdown, facing, stage, flight,
  fuse, cargo, door, flasher, and crew values. The unused Fuse_Read/Write pair is deleted.
  NoInit constructors remain wherever raw owners still need them; no member initializers were added.
  TD has no `CloakingClass`: its cloak state is `CloakType` plus a `StageClass` member of TechnoClass.
- TD save version is now **2**, accounting for countdown storage changing from `long` to `int64_t`
  on platforms with 32-bit long. The new countdown serializer stores remaining time and active state,
  re-anchoring to restored Frame. Heap objects still use raw images until the next steps wire these in.
- `td/serialize.{h,cc}` adds checked object TARGET proxies (including cargo's FootClass pointer) and
  enum-based static type references for all 11 tables. Object reads resolve raw heap addresses without
  dereferencing not-yet-constructed slots. Production type-table bindings are explicitly instantiated.
- Step-21 validation: strict builds of both games and **168 CTest tests** pass. Twelve new tests cover
  timer width/re-anchoring/invalid data, facing, partial animation, flight accumulation, door progress,
  flashing, crew kills, fuse behavior, and type-pointer resolution/rejection. Debug-display and legacy
  flight pointer-coding hooks are test-only link stubs; cargo/object-slot integration awaits heap migration.
  TD GDI/Nod smoke checks match 120/180 positions; RA matches 240. Step 22 starts the TD heap migration.
- Step 22 migrates **FactoryClass and TriggerClass** to field-wise archives (TD version **3**).
  Their NoInit constructors and raw I/O/pointer-coding methods are removed; safe defaults include active
  heap membership. Factory ownership is a house enum so saving never dereferences a coded HouseClass.
  Object TARGET and team-type TARGET references resolve checked heap slots. Trigger counters use int64_t.
- TD heaps now dispatch between Serializable objects and the remaining NoInit/raw objects. Migrated
  objects skip pointer coding; loads preserve sparse slots and reject duplicate/out-of-range slots,
  bad raw sizes, and truncated fields. Allocator and template instantiations are separated for tests;
  vector template definitions live in `vector_impl.h` so the allocator tests need no game link.
- `-FACTORYTEST` starts Jeep production after a new scenario. The TD smoke script accepts `--factory`
  as its third argument and compares serialized factory/trigger fields as well as unit positions.
  Step 23 continues with TeamTypeClass and TeamClass.
- Step-22 validation: strict builds of both games and **173 CTest tests** pass. Five heap tests cover
  sparse slots, skipped coding for field objects, raw vtable restoration, invalid counts/indices,
  duplicate slots, wrong raw sizes, and truncated data. TD GDI with active Jeep production matches
  **660** unit/factory/trigger states across save/load; Nod matches **630**, and RA matches **240**
  vehicle/vessel positions. Real-display gameplay and live multiplayer checks remain outstanding.
- Step 23 migrates **TeamTypeClass and TeamClass** (TD version **4**), including abstract base
  values, team missions, flags, timers, member references, and mixed static type references. Only
  populated definition arrays are saved; unused tails are initialized on load. Their NoInit/raw I/O,
  pointer coding, and vtable-capture setup are deleted; shared base NoInit paths remain for raw subclasses.
- Team shells initialize all fields without registering a team. Loads rebuild `TeamClass::Number`
  from the loaded heap, preserving recruitment limits and transient-team cleanup. House references
  use checked heap indices so they never inspect coded house metadata; team reads require a loaded
  house and team type before dereferencing them. Mixed type references validate kind and table bounds.
- `-TEAMTEST` creates a populated team with guard/loop missions and an active suspension countdown;
  pass `--team` as the TD smoke script's third argument. Smoke logs now stream complete serialized
  fields without a fixed-size buffer and compare team/type state and per-type live counts.
  Step 24 continues with HouseClass.
- Step-23 validation: strict builds of both games and **176 CTest tests** pass. New unit coverage
  checks mission fields, invalid/truncated missions, and abstract coordinate/active-state handling.
  Headless TD save/load matches **1,291** logged states with the populated team fixture, **1,231**
  with active production, and **1,380** in Nod (including team/type fields and live counts).
  RA still matches **240** vehicle/vessel positions. Real-display/live-multiplayer checks remain pending.
- Step 24 migrates **HouseClass** (TD version **5**): all gameplay fields, superweapon state,
  region threats, and every countdown timer are field-wise. Scan masks are uint64_t; credit/storage
  values and region threats are int64_t. House and SuperClass NoInit paths are removed; raw descendants
  elsewhere retain their own NoInit paths. Remap pointers use checked table IDs, preserving RemapNone
  independently of the player color. Factory indices, enums, names, and superweapon values are checked.
- House load shells recreate all ten runtime-only UnitTracker counters, as planned, instead of restoring
  process addresses. Their totals restart at zero; scenario reset deletes old houses to release trackers.
  Construction leaves scenario globals alone. Houses no longer participate in pointer coding.
- Superweapon tests exercise partial charge, suspended/resumed charging, and rejected input using the
  real fixed-point helpers, extracted from `coord.cc` into `fixedmath.cc` for independent linking.
  Region tests preserve wide and negative threat values. Smoke logs now include every house field.
- GDI diagnostics exposed different infantry positions **before** saving: fresh runs were using
  wall-clock seeds. `-SEED<n>` now parses past the dash correctly and works outside cheat builds;
  the TD smoke script uses `-SEED1` for reproducible scenario creation. Infantry coordinates are also
  logged for diagnosis. This does not add RNG-state persistence (remaining globals work).
  Step 25 continues with the smaller world-object heaps.
- Step-24 validation: strict builds of both games and **180 CTest tests** pass. Fixed-seed TD smoke
  runs match **1,831** states with active production, **1,891** with the populated team, and **1,980**
  in Nod, including serialized house fields. RA matches **240** vehicle/vessel positions.
  Real-display and live-multiplayer verification remains outstanding.
- Step 25 migrates **TemplateClass, OverlayClass, SmudgeClass, AnimClass, TerrainClass, and
  BulletClass** (TD version **6**), including shared ObjectClass flags, links, trigger references,
  and strength. Animations/terrain preserve stage progress; projectiles preserve flight, fuse,
  facing, altitude, and targeting state. The six classes lose raw I/O, NoInit, and vtable capture.
  Their default constructors initialize active shells without modifying scenario globals.
- ObjectClass keeps its NoInit and pointer-coding paths for raw Techno subclasses. Compile-time
  guards require buildings, infantry, units, and aircraft to stay raw while these six heaps use
  their own serializers. Trigger references validate TARGET kind/bounds; generic object references
  now support template slots. Overlay's static ownership reset moves to scenario initialization.
- `-WORLDTEST` / smoke argument `--world` creates all six world-object kinds, including limbo
  placement objects, shared trigger links, forward heap references, terrain stage state, an attached
  delayed animation, a configured dormant projectile, and a slow live missile with a timed fuse.
  Smoke comparisons include complete serialized world-object fields.
- The expanded projectile check exposed unsaved libc RNG state: shots created after loading diverged
  at frame 95. Step 25 brings RNG persistence forward from step 29. TD gameplay draws now use the
  shared deterministic RandomClass; the new RNGS section saves its seed, the legacy byte-generator
  state, and the simulation-table index. Loading no longer reseeds these streams. This changes TD's
  seeded sequences and makes recordings from the old RNG implementation incompatible in behavior.
  Tests cover mixed-stream continuation, invalid indices, and truncated state.
- Step-25 validation: strict builds of both games and **182 CTest tests** pass. TD smoke matches
  **5,111** states with the world fixture, **4,631** with production, **4,691** with a populated team,
  and **3,480** in Nod. RA matches **240** vehicle/vessel positions. Real-display and live-multiplayer
  verification remains outstanding.
- Step 26 migrates **MissionClass, RadioClass, TechnoClass, and BuildingClass** (TD version **7**).
  Mission queues/status/timers, checked radio TARGETs, all six Techno bases (including crew kills),
  ownership, cloak/facing/weapon state, and every building field are serialized explicitly.
  Building factory links use checked int32 indices, resolved without inspecting the later-loaded heap.
  Building shells initialize active membership without changing house counts or scenario globals.
- BuildingClass loses raw I/O, NoInit, and vtable capture. Shared bases retain their NoInit and
  pointer-coding paths for aircraft, infantry, and units; compile-time guards keep those heaps raw.
  Building fields now participate in every TD smoke comparison.
- `-BUILDINGTEST` / `--building` creates linked building fixtures with production, radio contact,
  infantry cargo, crew kills, non-default flags, timers, cloaking, facing, and door/stage progress.
  The smoke also mutates the saved building payload and checks that the real loader rejects invalid
  factory indices, missing types, unknown animation/mission enums, and a radio reference to a bullet.
  Heap-load failures now log the archive error for diagnosis.
- Expanded GDI mission coverage exposed animation frame limits initialized only by gameplay
  constructors. AnimTypeClass now derives its frame/loop limits when assets load, so shell-loaded
  animations resume at the saved stage; the cached limits are mutable fields on the static types.
- Step-26 validation: strict builds of both games and **182 CTest tests** pass. TD smoke matches
  **4,911** states with linked buildings, **5,291** with world objects, **4,782** in GDI mission 2,
  and **4,260** in Nod. All **six** malformed building saves are rejected before gameplay.
  RA matches **240** vehicle/vessel positions. Real-display and live-multiplayer checks remain pending.
- Step 27 migrates **FootClass, DriveClass, TurretClass, TarComClass, InfantryClass, AircraftClass,
  and UnitClass** (TD version **8**). Saved state includes navigation targets and full path
  buffers, team/member links, retry/attack timers, vehicle tracks and harvest state, turret reload,
  infantry action/fear state, and aircraft flight/landing/sight state. Crew kills now survive loads
  for mobile objects as well as buildings.
- Full mobile comparisons exposed movement reusing path entries beyond the first FACING_NONE.
  Both Foot constructors now initialize every path slot, and saves preserve the entire buffer.
- All object heaps are field-wise. RawImage dispatch, raw-object heap sizes, object pointer-coding
  methods/passes, vtable capture, and the object hierarchy's NoInit constructors are removed.
  Heap counts and sparse slot indices use explicit int32 archive values. Raw-only heap tests are
  replaced by empty-heap and malformed-header coverage; sparse-slot tests check hole reuse.
- `-MOBILETEST` / `--mobile` adds non-default vehicle/cargo state and a flying aircraft alongside
  the populated team fixture. Every mobile field now participates in all TD smoke comparisons.
  Corrupted-save checks cover path lengths/directions, team indices, drive tracks, flag owners,
  infantry actions, and aircraft types.
- Map and score still call Read_Object/Write_Object, so those helpers remain until steps 28–29;
  unused Get_VTable/Set_VTable are deleted. Map/layer/global pointer-coding passes and leaf NoInit
  constructors needed by the remaining raw UI chain stay for their planned migrations.
- Step-27 validation: strict builds of both games and **182 CTest tests** pass. TD smoke matches
  **6,092** states with mobile/team fixtures, **5,991** with linked buildings, **6,251** with world
  objects, **8,484** in GDI mission 2, and **6,480** in Nod. All **seven** malformed mobile saves
  and **six** malformed building saves are rejected before gameplay. RA matches **240** positions.
  Real-display and live-multiplayer checks remain pending.
- Step 28 is complete: it migrates **Map/Cell and the saved display/radar/sidebar fields** (TD version **9**).
  MAPS stores theater first; theater assets and empty cells are initialized before member reads.
  Map dimensions, int64 TotalValue, populated tiberium scan lists/direction, placement state,
  radar state, sidebar buildables, and sparse cells use archives. MCEL retains ascending CELL
  indices and a checked count; duplicate/out-of-range indices are rejected.
- Cells save all flags, enum/data/ownership fields, occupier and overlapper TARGETs, occupancy bits,
  and land type. Trigger TARGETs explicitly reference the CellTriggers side table, including
  forward references to the later-loaded trigger heap. Should_Save covers every non-default field.
  Cell defaults/reset/predicate move to cellvalues.cc for independent regression tests.
- Map/UI raw-image loading, map/cell pointer-coding passes, map vtable capture, map-chain NoInit
  constructors, and the now-unused VectorClass/CreditClass NoInit constructors are removed.
  Runtime graphics, screen geometry, and input resources remain local; load resets temporary modes,
  redraw/scroll/tooltip/mouse state, and credits display. Pending placement fixups remain after heaps.
- `-MAPTEST` / `--map` covers 16 isolated sparse cells, shared triggers, gapped overlappers,
  a pending building, and a map value above 32 bits. Corrupted-map checks cover theater, dimensions,
  scan counts, cell counts/indices, overlapper counts, land types, and trigger kinds.
  Every TD smoke compares a hash of all saved map fields; `TD_MAP_TRACE=1` logs their hex bytes at
  frames 60/61 to diagnose mismatches.
- GDI mission 2 exposed stale overlap references to inactive infantry. ObjectPtr writes these as
  null, but uninterrupted overlap insertion treated them as occupied and chose different slots.
  Overlap_Down now reclaims inactive entries before choosing a free slot; live overlap ordering and
  gaps remain serialized exactly.
- Step-28 validation: strict builds of both games and **184 CTest tests** pass. TD smoke matches
  **5,831** states with the map fixture, **8,544** in GDI mission 2, **6,540** in Nod, **6,051** with
  linked buildings, and **6,152** with mobile/team fixtures. All **eight** malformed map saves,
  **six** malformed building saves, and **seven** malformed mobile saves are rejected before gameplay.
  RA matches **240** positions. Real-display and live-multiplayer checks remain pending.
- Step 29 is complete: **Score, Base, Layers, and miscellaneous globals** use field-wise archives
  (TD version **10**). SCOR saves every counter and an explicit int64 elapsed time; the presentation
  pointer resets locally. BASE saves its house and checked node count, then building types/coordinates
  without struct padding or a sizeof guard. LAYR preserves ordered TARGET lists after all heaps load.
- MISC saves the player's heap index, scenario and movie names, ordered selection, waypoints,
  direction/variant, carryover settings, build level, views, countdown, and briefing text. RNGS retains
  the two previously migrated RNG streams. String buffers are terminated on read; scenario enums,
  waypoints, views, and the allocated player house are checked. TD still has no Session/Special/Options
  save path; this checkpoint migrates the existing miscellaneous field list.
- Layer/selection readers reject negative/oversized counts, null, duplicate, wrong-kind, out-of-range,
  and unallocated object references. Heap allocation is checked before dereferencing a slot; the
  earlier forward-reference resolver remains available for map/object loads.
- All remaining Code_All/Decode_All_Pointers and Read_Object/Write_Object helpers are removed.
  Player/scenario naming and pending-placement fixups now run only at the load tail. Score, Base,
  BaseNode, and Layer raw-layout test rows are removed; the leftover layout test is deleted in step 30.
- `-GLOBALTEST` / `--globals` adds a wide elapsed time, score counters, two base nodes, reversed unit
  selection, waypoints, carryover values, a view, and a countdown. Every TD smoke now also hashes
  Score, Base, Layers, miscellaneous fields, and RNG state. Unit tests cover wide score round-trips,
  truncated scores, and allocation queries for live, freed, and out-of-range heap slots.
- Step-29 validation: strict builds of both games and **186 CTest tests** pass. TD smoke matches
  **5,742** states with globals, **5,891** with map fixtures, **8,604** in GDI mission 2, **6,600**
  in Nod, **6,212** with mobile/team fixtures, and **6,111** with buildings. All **14** malformed
  globals, **eight** malformed maps, **seven** malformed mobile saves, and **six** malformed building
  saves are rejected before gameplay. RA matches **240** positions. Real-display and live-multiplayer
  checks remain pending.
- Step 30 is complete. TD has no remaining NoInit constructors or includes. The unused legacy
  session.h and wwfile.h copies and the obsolete heap-layout test are deleted. Save format stays **10**;
  the shared tech/noinit.h remains for the explicit step-31 deletion and initialization-check sweep.
- A shared td_engine object target compiles the game implementation once for tdsdl and the new
  td_archive_roundtrip_test. Startup helpers are shared while TD_NO_ENTRY_POINT excludes the game
  entry point in the test executable. This separate target keeps real game globals apart from the
  existing td_saveload_test's standalone stubs; no game data files or SDL startup are needed.
- Six BufferPipe-to-BufferStraw tests cover Cell flags and gapped object/trigger references, Map
  members and private scan arrays from an independently authored field stream, oversized scan counts,
  House type/remap identity and wide economy/timer state, Unit inherited fields and path tails, and
  truncated Cell/House/Unit records. Fixtures release their allocated objects and buffers.
- Step-30 validation: strict builds of both games and **191 CTest tests** pass. TD matches **5,891**
  states with map fixtures and **5,742** with globals; all **eight** malformed maps and **14** malformed
  globals are rejected. A version-10 save produced by the pre-cleanup binary loads with **5,742**
  matching state records. RA matches **240** positions. Real-display and live-multiplayer checks remain
  pending.
- Step 31 is complete. The shared tech/noinit.h is deleted; no NoInit declarations or includes remain
  anywhere in src. The previously listed ra/jshell.h include was already gone. Save formats remain
  **RA 16 / TD 10**; the member-initialization checks stay disabled until step 36.
- The single-check sweep (clang-tidy **23.1.2**, **877** distinct translation units from the strict
  compilation database, including generated header checks) reports **237** remaining sites and **zero**
  compilation errors: **27 tech**, **14 sdllib**, **1 winvq**, **75 TD**, **120 RA**. These measured
  counts supersede the earlier estimate of roughly 245. Exact locations and diagnostics are recorded
  in [MEMBER_INIT_BASELINE.tsv](MEMBER_INIT_BASELINE.tsv) for steps 32–35.
- Comparing diagnostics with the pre-migration source identified one new constructor site:
  RA's HouseClass loading shell. Its initializer list now sets IsActive=true and zeroes ZoneInfo,
  IniName, and InitialName; its deserializer then restores the saved values. The moved TD Cell
  constructor and changed timer parameter type retain their existing legacy findings. An accidental
  word replacement in the TD house header's license comment is also corrected.
- Step-31 validation: strict builds of both games and **191 CTest tests** pass. TD globals smoke
  matches **5,742** state records and rejects all **14** malformed globals. RA smoke matches **240**
  positions after the loading-shell fix. No suppressions were added. Real-display and live-multiplayer
  checks remain pending.
- Step 32 is complete: all **27 tech initialization sites** are fixed. Codec buffers, Blowfish
  tables, integer registers/remainder tables, field IDs, SHA state, LZW dictionary/stack data, and
  mix-file lookup offsets/sizes now have explicit defaults. The CD search state is value-initialized.
  Compression pipe headers retain their **0xFFFF** pending-header sentinel; straw headers start at
  zero. Existing key setup, hash seeds, integer setup, and dictionary reset routines remain in use.
- Five codec regression tests exercise LZO/LZW in both Pipe-to-Straw and Straw-to-Pipe directions,
  byte-at-a-time input, fragmented reads, multiple blocks, and a one-byte tail. LCW's existing compressor
  is a stub, so its Pipe/Straw decoders are checked against independently authored literal/run blocks.
  Base64 checks canonical encodings and decoding for short final groups; SHA checks its known abc
  digest after resetting a partial stream.
- The Base64 regression exposed an existing short-group offset bug: decoding writes one or two bytes
  at the start of its scratch buffer, while Get drains them from the end. Get now aligns a partial
  decoded group with that drain position, fixing padded final groups without adding persistent state.
- The single-check sweep of **103 tech translation units**, including generated header checks and
  the new tests, reports **zero** initialization sites and **zero** compilation errors. The step-31
  baseline remains historical; **210 sites** remain in the other components. Save formats remain
  **RA 16 / TD 10**; check enablement is still scheduled for step 36.
- Step-32 validation: strict builds of both games and **196 CTest tests** pass. TD globals smoke
  matches **5,742** state records and rejects all **14** malformed globals; RA smoke matches **240**
  positions. No suppressions were added. Real-display and live-multiplayer checks remain pending.
- Step 33 is complete: all **14 sdllib and one winvq initialization sites** are fixed. Viewport
  geometry, backing pointers, and lock counts now start empty; keyboard storage, countdown delay,
  modem buffer state, inactive audio volume, and mouse cursor state have explicit defaults. The modem
  defaults to touch-tone dialing. File status, font headers, socket records, and select timeouts are
  value-initialized, as are the VQA loader, drawer, and flipper aggregates. Existing setup routines
  and assignments remain in place.
- The single-check sweep of **60 sdllib/winvq translation units**, including generated header checks,
  reports **zero** initialization sites and **zero** compilation errors. The historical step-31
  baseline now has **195 sites** left in TD/RA. Save formats remain **RA 16 / TD 10**.
- Step-33 validation: strict builds of both games and all **196 CTest tests** pass. TD globals smoke
  matches **5,742** state records and rejects all **14** malformed globals; RA smoke matches **240**
  positions. No suppressions were added. Real-display and live-multiplayer checks remain pending.
- Step 34a is complete: all ten non-default TD event constructors now delegate to the default
  constructor, clearing `IsExecuted`, `MPlayerID`, unused payload bytes, and padding before assigning
  command fields. Explicit member defaults satisfy the initialization check; the full byte clear stays
  because events are compared and transmitted as raw bytes. A regression test constructs every overload
  over storage filled with `0xff` and compares the full representation against the expected command.
- TD cell occupancy flags are value-initialized; the existing cell reset and sparse-save tests remain.
  The empty private CellClass and CCFileClass copy constructors are deleted. The sweep of **308 TD
  translation units** reports **zero compilation errors** and **61 remaining TD sites**, with no
  findings in the step-34a classes. This removes **14 baseline sites**, leaving **181** across TD/RA.
  Save formats remain **RA 16 / TD 10**; no suppressions were added.
- Step-34a validation: strict builds of both games and all **197 CTest tests** pass. TD globals smoke
  matches **5,742** state records and rejects all **14** malformed globals; RA smoke matches **240**
  positions. Real-display and live-multiplayer checks remain pending.
  Next checkpoint is **34b: remaining TD initialization sites**.
- Step-19 validation: strict build of both games and all **154 CTest tests** pass. Headless save/load checks pass
  for SCG01EA and SCU01EA (240 matching unit/vessel positions each) and SCG02EA (120).
  Step 19 also loads a pre-cleanup version-16 save with 240 matching positions. The SCU01EA smoke
  passed with `RA_SAVE_DUMP` enabled (plaintext `FRAM=60` and Section tags verified); SCG02EA passed
  with an invalid dump path. Three tee tests cover exact copying, short writes, and disabled copying.
  The step-18 headless SCG01EA recording/playback check matched all 240 positions, with the
  `RARC`/version-16 header.
- A real-display playthrough and live multiplayer end-to-end checks remain outstanding.

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
- `SessionClass`/`NodeNameType`: one shared session field list for the Pipe and CCFileClass variants; write `Type` in both.
  Keep the player roster recording-only: network loads retain connected peers for `Reconcile_Players()`.
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
21. Leaf value types: `TCountDownTimerClass`, `FacingClass`, `StageClass` (public), `FlyClass`, `FuseClass` (delete Fuse_Read/Write), `CargoClass`, `DoorClass`, `FlasherClass`, `CrewClass`; type-ref helper ⚠. TD cloak state is a `CloakType` plus `StageClass`, not a separate class.
22. FactoryClass + TriggerClass, mixed Serializable/raw heap dispatch, and heap tests.
23. TeamTypeClass + TeamClass.
24–27. Remaining heap hierarchy in the same base-first order as RA (Techno + Crew ⚠, Turret/TarCom ⚠, Building Factory index ⚠, Trigger ⚠, House ⚠). Delete `ioobj.cc` bodies, `Read/Write_Object`, `VTable` statics, `heap.cc` raw paths.
28. Map/Cell ⚠ (theater-first, `CellTriggers`, full `Should_Save`).
29. Globals: Score, Base, Layers, Misc. RNG streams already migrated in step 25.
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
`src/ra/heap_instances.cc`, `src/ra/heap_test.cc`, `src/ra/crate_test.cc`, `src/ra/saveglobals.cc`,
`src/ra/saveglobals_test.cc`, `src/td/saveload_test.cc`.
Deleted: `src/tech/noinit.h`, `src/ra/heap_layout_test.cc`, `src/td/heap_layout_test.cc`, `src/td/session.h`, `src/td/wwfile.h`.
Modified (core): `src/ra/heap.h/.cc`, `ra/ioobj.cc`, `ra/iomap.cc`, `ra/saveload.h/.cc`, `ra/ccptr.h`, `ra/target.h/.cc`,
`tech/fixed.h`, `tech/ftimer.h`, the 17 RA heap classes + bases/mixins, the Map chain headers, `ra/scenario.h`,
`score.h`, `carry.h`, `special.h`, `options.h`, `vortex.h/.cc`, `session.h/.cc`, `base.h/.cc`, `init.cc`,
`td/saveload.cc`, `td/ioobj.cc`, `td/iomap.cc`, `td/heap.cc`, TD class headers, `.clang-tidy`,
`docs/CLANG_TIDY_PRIORITIES.md`, `ra/CMakeLists.txt`, `td/CMakeLists.txt`, `tech/CMakeLists.txt`.

## Verification

- Unit: `ra_saveglobals_test` (all Special bitmasks, wide score counters, stopwatch state, truncated scores,
  player names/addresses/fields), `archive_test`, `fixed_test`, `ra_crate_test` (default state, timer preservation, invalid cells, truncated timer), timer tests, `ra_heap_test` (sparse indices, `ID != idx` rejected, count mismatch
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
bash -c "xargs -a /tmp/tidy_files.txt -P $JOBS -I{} clang-tidy -p $BUILD --quiet --checks='-*,cppcoreguidelines-pro-type-member-init' --warnings-as-errors=-* {} 2>&1" \
  | grep -oE '^/home[^ ]+ (warning|error): .*\[[A-Za-z0-9._,-]+\]$' | sed -E 's/,-warnings-as-errors\]$/]/' | sort -u > /tmp/member_init_sites.txt
# full config + the new check (no '-*' => appended), must print nothing before the enable commit
bash -c "xargs -a /tmp/tidy_files.txt -P $JOBS -I{} clang-tidy -p $BUILD --quiet --checks='cppcoreguidelines-pro-type-member-init' --warnings-as-errors=-* {} 2>&1" \
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
