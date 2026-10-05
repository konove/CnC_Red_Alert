# Plan: one class per trigger action

Written 2026-10-04 against `main` at c25198d5.

**Status: done 2026-10-04,** merged to `main` (ae84a09d..062b07d5), apart from the by-hand check of
the editor dialog. See Outcome at the end for what differs from the design.

## Context

`ra::TriggerAction` (`src/ra/ai/trigger_action.h`) is one class for all 37 kinds of trigger action.
It carries a `TriggerActionType`, a `TeamTypeClass*`, a `TriggerType*` and a nine-member union, and
`Perform()` is a 330-line switch over the type. TODO.md proposed replacing the union with an
`int value_`. This plan replaces that entry: the union is a symptom, and the cause is that 37
different things share one class.

What the single class costs today:

- **The union is read through inactive members.** The INI reader and the save game write `value`;
  `Perform()` reads `movie`, `special`, `house`, `speech` (one byte each) and `flag`. The shipped
  scenarios depend on it: SCG12EA's nuke trigger stores -255 and SCG10EA's `lch3` stores movie -170,
  and both only mean something as their low byte.
- **Every action carries fields it does not use.** A `kPlaySound` action has a team pointer and a
  trigger pointer; a `kCreateTeam` action has a value. Nothing says which field an action reads
  except `ActionNeed()` and the body of the switch.
- **`trigger_action.cc` includes 50 headers,** because one function reaches the map, the houses, the
  timer, the audio, the movie player, the sidebar and four object heaps.
- **Nine typed getter/setter pairs** exist so that the editor and the tests can reach one `int`.

## Findings

### Who uses an action, and how

| User                                                         | What it does                                                                                          |
| ------------------------------------------------------------ | ----------------------------------------------------------------------------------------------------- |
| `TriggerClass::Spring()` (`ra/ai/trigger.cc:338-354`)        | Calls `Perform(house, object, id, cell)` on `action1` and/or `action2`                                |
| `trigger.cc:208`, `scenario.cc:641`                          | Ask only `type() == kAllowWin`                                                                        |
| `TriggerType::ReadIniEntry()` / `ToIniEntry()`               | Read and write `action,team,trigger,value`                                                            |
| `TriggerType::ReadIni()` (`trigger_type.cc:200-220`)         | Resolves `pending_trigger_name` for INI formats 0 and 1, once every trigger type exists               |
| `DropUnfilledTypeReferences()` (`scenario.cc:1865`)          | Nulls a team or trigger pointer that names an unfilled heap slot                                      |
| `TriggerAction::Serialize()` (`saveload/ioobj.cc:190`)       | Saves `type, team slot, trigger slot, value`                                                          |
| `TriggerType::Detach()`                                      | Forwards to both actions                                                                              |
| The trigger type dialog (`ra/editor/trigger_type_dialog.cc`) | Reads every field into its widgets, and on OK calls `set_type()` then the setter `ActionNeed()` names |

Three of these (INI, save game, editor) treat an action as the same four-field record whatever its
type. Only `Perform()` cares what the action is. That is the seam: behaviour becomes polymorphic,
and the four-field record becomes an explicit struct that exists only at those three boundaries.

### The payloads

`ActionNeed()` already sorts the 37 types by payload. The widths matter for the low-byte rule:

| Payload         | Type                         | Width  | How the record's `value` becomes it                                       |
| --------------- | ---------------------------- | ------ | ------------------------------------------------------------------------- |
| House           | `HousesType : int8_t`        | 1 byte | low byte                                                                  |
| Special weapon  | `SpecialWeaponType : int8_t` | 1 byte | low byte (-255 is the nuke)                                               |
| Movie           | `VQType : int8_t`            | 1 byte | low byte (-170 is movie 86)                                               |
| Speech          | `VoxType : int8_t`           | 1 byte | low byte                                                                  |
| Flag            | `bool`                       | 1 byte | low byte is non-zero                                                      |
| Sound           | `VocType`                    | `int`  | the value, or its low byte when the value is no sound and the byte is one |
| Theme           | `ThemeType`                  | `int`  | the value                                                                 |
| Quarry          | `QuarryType`                 | `int`  | the value                                                                 |
| Waypoint/number | `int`                        | `int`  | the value                                                                 |
| Team            | `TeamTypeClass*`             |        | the record's team field                                                   |
| Trigger         | `TriggerType*`               |        | the record's trigger field                                                |

## Design

### The interface

```cpp
// What an action is told when its trigger springs.
struct TriggerActionContext {
  HousesType house;     // The trigger's owner.
  ObjectClass* object;  // What the trigger sprang on; nullptr for none.
  int trigger_id;       // The springing trigger's heap slot; -1 for none.
  Cell cell;            // The cell the trigger is attached to; cell 0 for none.
};

// An action as the scenario INI, a saved game and the editor see it.
struct TriggerActionRecord {
  TriggerActionType type = TriggerActionType::kNone;
  TeamTypeClass* team_type = nullptr;
  TriggerType* trigger_type = nullptr;
  int value = -1;
};

class TriggerAction {
 public:
  virtual ~TriggerAction() = default;

  [[nodiscard]] virtual TriggerActionType type() const = 0;

  // Carries the action out. Returns false when it could not be done, so the
  // trigger is not counted as used.
  virtual bool Perform(const TriggerActionContext& context) = 0;

  // Drops the action's reference to `target`, which is being destroyed.
  virtual void Detach(Target target);  // Does nothing by default.

  // Returns the action as a record; MakeTriggerAction() makes an equal action
  // from it.
  [[nodiscard]] virtual TriggerActionRecord ToRecord() const = 0;
};

// Returns the action `record` describes; never null. A type that takes no team,
// trigger or value ignores that field.
[[nodiscard]] std::unique_ptr<TriggerAction> MakeTriggerAction(
    const TriggerActionRecord& record);
```

`ActionName()` and `ActionNeed()` stay free functions of `TriggerActionType`: the editor asks them
about a type picked from a list, before any action exists.

### The concrete classes

29 classes for the 37 types. A class that covers more than one type stores which, and `type()`
reports it, so the INI and save numbers are unchanged.

| Payload        | Classes (types they cover)                                                                                                                                                 |
| -------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| None           | `NoAction` (kNone, kWinLose, kAllowWin), `RevealAllAction`, `TimerSwitchAction` (kStartTimer, kStopTimer), `CreepShadowAction`, `DestroyObjectAction`, `LaunchNukesAction` |
| House          | `DeclareWinnerAction` (kWin, kLose), `BeginProductionAction`, `AllHuntAction`, `FireSaleAction`, `AutocreateAction`                                                        |
| Team           | `CreateTeamAction`, `DestroyTeamAction`, `ReinforcementsAction`                                                                                                            |
| Trigger        | `DestroyTriggerAction`, `ForceTriggerAction`                                                                                                                               |
| Waypoint       | `DropZoneFlareAction`, `RevealAroundAction` (kRevealSome), `RevealZoneAction`                                                                                              |
| Number         | `TextAction` (kTextTrigger), `TimerAdjustAction` (kAddTimer, kSubTimer, kSetTimer), `GlobalAction` (kSetGlobal, kClearGlobal)                                              |
| Movie          | `PlayMovieAction`                                                                                                                                                          |
| Sound          | `PlaySoundAction`                                                                                                                                                          |
| Theme          | `PlayMusicAction`                                                                                                                                                          |
| Speech         | `PlaySpeechAction`                                                                                                                                                         |
| Flag           | `BaseBuildingAction`                                                                                                                                                       |
| Special weapon | `SpecialWeaponAction` (kOneSpecial, kFullSpecial)                                                                                                                          |
| Quarry         | `PreferredTargetAction`                                                                                                                                                    |

Each holds exactly its payload as a typed private member (`VQType movie_`,
`TeamTypeClass* team_type_`, nothing at all for `RevealAllAction`). Two small abstract bases,
`TeamAction` and `TriggerReferenceAction`, hold the pointer and implement `Detach()` for the three
and two classes that need it; no other intermediate bases.

The classes live in an anonymous namespace in `trigger_action.cc`. Nothing outside names one:
everything is built by `MakeTriggerAction()` and used through the interface, tests included. The
header exports only the enum, the two structs, the interface, the factory and the two free
functions.

### The low byte

The conversion from the record's `int` to a typed payload is written once, in the factory, per the
table above: `static_cast<E>(static_cast<int8_t>(value))` for the one-byte enums,
`(static_cast<uint32_t>(value) & 0xFFU) != 0` for the flag, and today's range rule for sounds. No
union, no read of an inactive member. The three low-byte tests in `trigger_action_test.cc`
(specials, movies, a house over -1) keep their inputs and expectations and go through the factory.

### Ownership

`TriggerType::action1` and `action2` become `std::unique_ptr<TriggerAction>`, never null: the
constructor makes two `NoAction`s. `TriggerType` stays a legacy class with public members; giving it
accessors is `/restructure-class` work, not this plan's.

- **Save game.** `TriggerType::Serialize()` saves `action->ToRecord()` and, loading, assigns
  `MakeTriggerAction(record)`. The record serializes the same four fields in the same encoding
  (`game::HeapSlotPtr` for the two pointers).
- **INI.** `TriggerType::ReadIniEntry()` parses each action into a record. The actions are built
  after every trigger type exists, in the pass that today resolves `pending_trigger_name`, so a
  trigger named before it is defined resolves before the action is made. `pending_trigger_name_`
  moves out of the action into that pass's bookkeeping, and the unfilled-slot check
  `DropUnfilledTypeReferences()` runs on actions moves there too.
- **Editor.** The dialog copies `ToRecord()` of each action into a local record, shows and edits the
  record, and on OK assigns `MakeTriggerAction(record)`. Its typed reads (`action1.special()`)
  become casts of `record.value`.

### What changes in the data

The formats do not change. The contents of fields an action does not use do:

- An action no longer remembers a team, trigger or value its type ignores. `ToRecord()` writes null,
  null and -1 for them. Today an editor-saved INI or a save game writes back whatever was read.
- A one-byte payload is written as its own value. SCG12EA's nuke special is saved as 1, not -255.
  Both read back as the nuke, in this binary and the previous one.

So saves and editor-written INIs load in both directions across this change, but are not
byte-identical to what the previous binary writes. Game behaviour is unchanged.

## Non-goals

- `TEventClass` has the same shape (one class, a type enum, a shared payload). It gets the same
  treatment later, as its own plan.
- Tiberian Dawn's triggers are a different design and stay as they are.
- `TriggerType`'s public data members.
- The `DestroyObjectsAttachedTo()` TODO about objects skipped when the heap compacts.
- Splitting `trigger_action.cc` into several files. Expected size is about 700 lines; revisit if it
  reads badly.

## Plan

Each step builds, passes the tests and is one commit.

0. Commit this document; replace the TODO.md entry with a pointer to it.
1. Add `TriggerActionRecord`, `ToRecord()` and a record constructor to the existing class, and route
   `ReadIni()`, `ToIniEntry()` and `Serialize()` through the record. No behaviour change.
2. Move the INI pending-name and unfilled-slot handling into `TriggerType::ReadIni()`'s second pass,
   working on records.
3. Port the editor dialog to edit a record. After this step nothing outside `trigger_action.cc` and
   its test calls a typed setter.
4. Introduce `TriggerActionContext`; `Perform()` takes it.
5. Make `TriggerAction` the interface. The existing class becomes a private `LegacyAction` that
   still holds the union and the switch; `TriggerType` holds `unique_ptr`s made by
   `MakeTriggerAction()`. Tests move to the factory.
6. Peel the classes out of `LegacyAction`, one payload group per commit (none, house, team, trigger,
   waypoint and number, media, the rest). Each commit deletes the switch cases and union members it
   replaces.
7. Delete `LegacyAction`, the union and the typed accessors. Update the comments in `defines.h` that
   say `TriggerAction` reads a byte of the union.

## Verification

- `trigger_action_test.cc`, ported: every existing behaviour test keeps its expectations. New:
  `MakeTriggerAction(action->ToRecord())` round-trips for all 37 types, and the factory ignores the
  fields a type does not use.
- A test that reads the `[Trigs]` section of each shipped scenario, writes it and reads it again,
  and gets equal records.
- `tools/ra_saveload_smoke.sh` on every fixture, and the checked-in save fixture still loads. A save
  written after the change loads in the pre-plan binary.
- Headless logic logs match the pre-plan binary on a mission whose triggers fire (reinforcements,
  timer, reveal, movie).
- `tools/coverage.py src/ra/ai/trigger_action.cc` stays at 100%.
- One full strict build before the last commit; `tools/strict_tu.py` during the steps.
- By hand: the editor's trigger dialog opens an existing trigger with the right widgets filled,
  switching the action type switches the value widget, and OK then reopening shows what was set.

## Outcome

All eight steps are done, one commit each and seven for step 6. `ra::TriggerAction` is the interface
above, `MakeTriggerAction()` makes one of 29 classes private to `trigger_action.cc`, and the union,
the nine typed accessor pairs and `pending_trigger_name_` are gone.

What differs from the design:

- **No pending-name bookkeeping.** `TriggerType::ReadIni()` creates and names every trigger type
  first and reads the entries second, so `ReadTriggerActionRecord()` resolves a trigger name
  (formats 0 and 1) or checks a heap slot (later formats) on the spot.
  `DropUnfilledTypeReferences()` lost its action half. `ReadIniEntry()` called on its own no longer
  resolves a name that is defined later; only `ReadIni()` calls it in the game.
- **The data change landed with the factory (step 5),** not class by class: a scaffolding
  `Normalized()` gave `LegacyAction` the record as the classes would keep it, so the low-byte tests
  went through `ToRecord()` at once and every step 6 commit was a pure refactor.
- **`LegacyAction` was deleted with the last group** rather than in step 7: once the factory stopped
  naming it, it was unused code, which the strict build rejects.
- **`TriggerActionContext` has defaults** (`HOUSE_NONE`, `nullptr`, -1, cell 0), so a test names
  only the fields it uses.
- **Trigger types are now destroyed, not just freed.** `TriggerType::DestroyAll()` called the heap's
  `FreeAll()`, which runs no destructors, so the two owned actions leaked on every scenario clear
  (AddressSanitizer showed it). It and `~ObjectHeaps()` now call the heap's `DestroyAll()`.
- **A type number that is no action** makes a `NoAction` that keeps the number.
- **`trigger_action.cc` is about 1,150 lines,** not 700. Each class carries its own `type()` and
  `ToRecord()`; the four one-line house actions are the obvious candidates for a shared base
  (TODO.md).

Verification, as run:

- `ra_trigger_action_test` (59 tests) and `ra_trigger_type_test`. The round trip over all 37 types
  and "keeps only the fields its type uses" are `EveryActionIsMadeAgainFromItsRecord` and
  `AnActionKeepsOnlyTheFieldsItsTypeUses`.
- `TriggerTypeTest.TheShippedTriggersAreReadBackAsWritten` passed on the 42 campaign scenarios of
  the Steam install. It needs `RA_SCENARIO_DIR` and is skipped without it.
- Saves written by the new binary and by the pre-plan binary each load in the other and log the same
  unit positions to frame 120 (SCG01EA, SCU05EA). `tools/ra_saveload_smoke.sh` passes.
- SCU01EA and SCU05EA log the same unit and vessel positions over 900 frames in both binaries,
  harvesters excluded: their logged destination differs between two runs of one binary.
- `tools/coverage.py src/ra/ai/trigger_action.cc`: 605 of 639 lines. It was 325 of 355 before, not
  100%; the uncovered lines are the same ones (audio, movie, tutorial text, the sidebar button, the
  drop zone animation).
- Not met: the checked-in save fixture (`--load-fixture`) does not load, and did not before the plan
  either: it is not a version 18 save.
- Open: the by-hand check of the trigger dialog.
