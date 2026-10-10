# Plan: composition in the game object hierarchy

Written 2026-10-10.

**Status: done** (2026-10-10, 8c93acff..HEAD of the `object-composition` branch). Every step below
is implemented in both games; the object hierarchy is the one under [Target](#target). What the
steps settled where the plan left a choice:

- Step 1: the fuse's target accessor is `ImpactPoint()`, because `Target` is a type name.
- Step 3: `FactoryClass` and Tiberian Dawn's `AnimClass`, which inherited privately, keep the
  counter private with no accessor.
- Step 5: `RadioPeer` has one function, `Receive(RadioPeer* from, ...)`, which `TechnoClass`
  implements once by forwarding to the `Receive_Message(TechnoClass* from, ...)` chain; every
  override already used the sender as a techno. `ObjectClass` no longer declares
  `Receive_Message()`, and `Transmit_Message()` stays on the techno, no longer virtual.
- Step 6: the member is `mission_state_` behind `mission_state()`, since `mission` is the usual name
  of a `MissionType` parameter. Its fields are `current()`, `queued()`, `suspended()` and `step()`
  (the former `Status`). The override and restore logic stays in `TechnoClass`, because it calls the
  virtual `Assign_Mission()`.
- Step 7: the two levels are folded into `UnitClass`, as Red Alert did.

The steps are kept as written, in the future tense, as the record of what was planned.

## Context

The object classes of both games inherit for two different reasons, and only one of them is an
"is-a" relation that code dispatches on:

- **Real levels.** `AbstractClass → ObjectClass → TechnoClass → FootClass → DriveClass`, with the
  leaf classes below them. Code asks `Is_Techno()` and `Is_Foot()`, holds `TechnoClass*` and
  `FootClass*`, and overrides the virtuals each level declares.
- **Slices of one class.** A base that exists to hold a few fields and the functions on them, which
  nothing dispatches on. Its names read as operations on the object itself (`How_Many()`,
  `Set_Rate()`), any subclass can reach its state, and each one with a virtual destructor adds a
  vtable pointer to every object.

The second kind is what this plan replaces with members. What was left of it when the plan was
written:

| Base                                         | Inherited by                                                      | Holds                                                | Used polymorphically                                                |
| -------------------------------------------- | ----------------------------------------------------------------- | ---------------------------------------------------- | ------------------------------------------------------------------- |
| `FuseClass`                                  | `BulletClass`                                                     | timer, arming delay, target, proximity               | no                                                                  |
| `FlyClass`                                   | `AircraftClass`, `BulletClass`                                    | speed accumulator and speed                          | no                                                                  |
| `StageClass`                                 | `AnimClass`, `TerrainClass`, `FactoryClass`, sidebar `StripClass` | stage, timer, rate                                   | no                                                                  |
| `RadioClass`                                 | `TechnoClass` only                                                | the contact and the last three messages              | `Receive_Message(RadioClass* from, ...)`, declared on `ObjectClass` |
| `MissionClass`                               | `RadioClass` only                                                 | mission, suspended and queued mission, status, timer | the 18 `Mission_*()` virtuals, overridden in the techno subclasses  |
| `TurretClass`, `TarComClass` (Tiberian Dawn) | one child each, ending in `UnitClass`                             | turret facing and targeting                          | to be checked in step 7                                             |

`ObjectClass → MissionClass → RadioClass → TechnoClass` is a single chain: nothing but `TechnoClass`
derives from the two in the middle, so they are layers of `TechnoClass`, not kinds of object.

## Target

```
AbstractClass            identity: RTTI, id, coordinate, active flag
└─ ObjectClass           on the map: type, strength, limbo, draw, select
   ├─ AnimClass, BulletClass, TerrainClass, OverlayClass, SmudgeClass, TemplateClass
   └─ TechnoClass        owned, armed, commandable
      ├─ BuildingClass
      └─ FootClass       moves
         ├─ InfantryClass
         ├─ AircraftClass
         └─ DriveClass → UnitClass, VesselClass (Red Alert)
```

Single inheritance throughout. Everything else is a private member with an accessor, as the four
members of `TechnoClass` are now, plus two small pure-virtual interfaces at the two places where a
slice calls back into its owner:

- `RadioPeer`: what a radio contact has to be able to do, which is receive a message. `TechnoClass`
  implements it; the radio member points at a `RadioPeer` and does not know about technos.
- `MissionHandler`: the `Mission_*()` functions. `TechnoClass` implements it; the mission member
  owns the state and the timer and calls the handler for the mission that is due.

No interface goes on top of the whole hierarchy. `ObjectClass` has dozens of virtuals with real
default bodies, and an `IObject` above it would only repeat their declarations.

## Rules for every step

- **Both games, one commit per step** when the step renames something they share, otherwise one
  commit per game.
- **Saved games do not change.** Every class here serializes field by field; the owner's
  `Serialize()` calls `member_.Serialize(ar)` where it called `Base::Serialize(ar)`, in the same
  place. No `kSaveGameVersion` bump.
- **Network play does not change.** No step alters what the simulation computes.
- **The member is private, the accessor is named for what it is** (`cargo()`, `door()`,
  `animation()`), returns a reference and is marked `ABSL_ATTRIBUTE_LIFETIME_BOUND`, in a const and
  a non-const overload.
- **Fix callers from compiler errors, not by text replace,** when the method names are shared with
  another class. `StageClass`'s are: the stage step in 9e23f36c removed the base, built with `-k 0`
  and prefixed the reported sites. A text replace is fine when the names are unique in the game
  (`CargoClass`'s were).
- **A method name that only made sense on the owner loses the redundant word** once it is called
  through the accessor (`door().IsOpen()`, not `door().IsDoorOpen()`). Legacy names that read well
  enough through the accessor stay until the class is modernized.
- **Verify each step** with the plain build and `ctest`, the full strict build, cpplint on the
  touched files, `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh`, and, for steps 5 and
  6, `tools/net_check.py`.

## Steps

Sizes are call sites counted with `git grep` on 2026-10-10, Red Alert / Tiberian Dawn.

### 1. Fuse: a member of `BulletClass`

`FuseClass` has one inheritor and three functions (`Arm_Fuse()`, `Fuse_Checkup()`, `Fuse_Target()`)
plus a public `Timer` field that the bullet code reads. Make it `fuse_` behind `fuse()`. Through the
accessor the functions can become `Arm()`, `Checkup()` and `Target()`. Small: under 30 call sites a
game together with step 2.

### 2. Fly: a member of `AircraftClass` and `BulletClass`

`FlyClass` has `Fly_Speed()`, `Physics()` and `Get_Speed()`. Make it `flight_` behind `flight()` in
both inheritors. Tiberian Dawn's has a virtual destructor that goes in step 4.

### 3. Stage: a member of its four remaining inheritors

`AnimClass` (10 / 10 sites), `TerrainClass` (6 / 14), `FactoryClass` (23 / 26) and the sidebar's
`StripClass` (4 / 4) inherit `StageClass`; `FactoryClass` and Tiberian Dawn's `AnimClass` do so
privately, so nothing outside them is affected. Use the accessor name `animation()` where the
counter drives a drawn animation, as on `TechnoClass`; for `FactoryClass` it counts production
steps, so name it for that (`progress()`). Fix from compiler errors.

### 4. Make the member classes plain values

`game::FlasherClass`, `CargoClass`, Tiberian Dawn's `StageClass` and `FlyClass` still carry what a
base needed: a virtual destructor, deleted copy and move operations, an empty `AI()`, empty
`Code_Pointers()` / `Decode_Pointers()`. Once nothing inherits them (after steps 1 to 3):

- remove the virtual destructors and their out-of-line definitions, and with them the note in
  `src/td/CMakeLists.txt` about where the `FlyClass` and `StageClass` vtables live;
- remove the deleted copy and move operations where the owner already forbids copying;
- remove the empty `AI()` and pointer-coding functions and their calls;
- make `FlasherClass`'s `FlashCount` and `IsBlushing` private behind functions that say what the
  callers do (start flashing, ask whether to draw brightened).

This shrinks every techno by the vtable pointers of its former bases. `FlasherClass` has no test
file of its own; add one here.

### 5. Radio: a member of `TechnoClass` and the `RadioPeer` interface

`RadioClass` holds the contact (`RadioClass* Radio`) and the last three messages, and has
`In_Radio_Contact()`, `Radio_Off()`, `Contact_With_Whom()` and two `Transmit_Message()` overloads:
292 / 226 call sites. `Receive_Message(RadioClass* from, RadioMessageType, int32_t&)` is declared on
`ObjectClass` and overridden down the techno classes (8 declarations a game), although only a techno
can be a contact: `Contact_With_Whom()` already returns `TechnoClass*`.

- Add `RadioPeer`, a pure-virtual class with `Receive_Message()`, next to the radio code.
  `TechnoClass` implements it; the `from` parameter becomes `RadioPeer*`. Decide while doing it
  whether `ObjectClass` keeps the declaration (a non-techno never receives one).
- Turn `RadioClass` into the member: it keeps the contact as a `RadioPeer*`, the message history and
  the transmit logic, and `TechnoClass` holds it as `radio_` behind `radio()`.
- `Contact_With_Whom()` stays on `TechnoClass`, where the downcast to `TechnoClass*` belongs.
  `Limbo()`, which `RadioClass` overrides to break contact, moves into `TechnoClass::Limbo()`.
- `Transmit_Message()` is virtual and so is the receive side; keep the call sites reading
  `Transmit_Message(...)` on the techno if the member's version needs the sender, since it passes
  `this` as `from`.

With a `RadioPeer` test double the radio handshake can be unit-tested without building technos. Run
`tools/net_check.py` after it.

### 6. Mission: a member of `TechnoClass` and the `MissionHandler` interface

The largest step. `MissionClass` has public fields (`Mission`, `SuspendedMission`, `MissionQueue`,
`Status`) that the object code reads and writes directly, about 420 / 306 times, and 18 virtual
`Mission_*()` functions that `MissionClass::AI()` picks from when the mission timer runs out.

- **6a. Accessors first.** Make the four fields private behind accessors while `MissionClass` is
  still a base, one commit a game. This is the bulk of the call sites and changes no structure.
  `Status` is the per-mission state-machine step; name it for that.
- **6b. The interface.** Add `MissionHandler`, pure virtual, with the `Mission_*()` functions.
  `TechnoClass` implements it, with the default bodies `MissionClass` has today; the overrides in
  the building, foot, unit, infantry, aircraft and vessel code do not move.
- **6c. The member.** What is left of `MissionClass` (the state, the timer, `Assign_Mission()`,
  `Set_Mission()`, `Override_Mission()`, `Restore_Mission()`, the dispatch in `AI()`) becomes the
  member `mission_`, whose `AI()` takes the handler. The virtuals among those that subclasses
  override (`Assign_Mission()`, `Override_Mission()`, `Restore_Mission()`) stay virtual on
  `TechnoClass` and call the member.

After 6c `TechnoClass` derives from `ObjectClass` directly. Run the save/load smoke scripts and
`tools/net_check.py`; the mission state machine is where a reordering would desync.

### 7. Tiberian Dawn's turret chain

Tiberian Dawn has `DriveClass → TurretClass → TarComClass → UnitClass`, two more levels with one
child each, which Red Alert already folded away. Read what they hold and decide whether to fold them
into `UnitClass` the way Red Alert did or to make them members. Not started; needs a look before it
can be sized.

## Not in this plan

- `DriveClass` stays a level. It exists to share track-following between units and vessels, which is
  a real "is-a" for the code that follows tracks.
- `AbstractClass` and `ObjectClass` stay separate levels, because each has other children.
  `AbstractClass` (RTTI, id, coordinate, `As_Target()`, distance and direction) is also the base of
  `TeamClass`, which is targetable and has a position but is not on the map. `ObjectClass` (type,
  strength, limbo, draw, select) is also the base of anims, bullets, terrain, overlays, smudges and
  templates, which are on the map but are not owned or commanded.
- `AbstractClass`, `ObjectClass`, `TechnoClass` and `FootClass` keep their virtuals. Thinning those
  (for example, moving drawing out of `ObjectClass`) is a different piece of work.
- The type classes (`TechnoTypeClass` and below) are a parallel hierarchy with no mix-in bases.
- Renaming the legacy classes and their methods to the Google scheme. Each class gets that when it
  is modernized (`/modernize-file`); this plan only drops words the accessor makes redundant.
