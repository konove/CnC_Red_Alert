# Plan: enable `cppcoreguidelines-use-enum-class`

Written 2026-09-15 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-15).** The check is enabled; the results are in the "Enum class
enablement" review in CLANG_TIDY_PRIORITIES.md.

## Context

The row was skipped on 2026-09-12 with 517 reports: 178 unnamed enums used as integer constants, 46
per-dialog `RedrawType` levels compared as ordered values, and 137 core type enums in each game's
`defines.h` that index arrays and loops. A fresh isolated sweep over all 936 compile commands on
2026-09-15 reports **515 unscoped enums** in 118 files:

| Group                                                                   | Count |
| ----------------------------------------------------------------------- | ----- |
| Named enums in `ra/defines.h` and `td/defines.h`                        | 137   |
| Other named enums at namespace scope (`ra` 39, `td` 13, `sdllib` 17)    | 73    |
| Named enums nested in a class or function (46 of them `RedrawType`)     | 128   |
| Unnamed enums (171 nested in a class or function, 6 at namespace scope) | 177   |

The check wants every one of them scoped. What the project gets from that is type safety on the
game's vocabulary: today a `StructType` converts silently to `int` and, through `int`, meets
`UnitType`, `HousesType` and array indices of the wrong table; the `ScanBit` defect found by the
signed-bitwise pass was exactly a type index used as a shift count. A scoped enum makes every such
conversion explicit, and the compiler rejects the mixed ones.

Scoping every named enum with a script (`enum class` plus `using enum` so the enumerator names keep
resolving) and syntax-checking the whole tree measures the cost: **7,900 compile errors** before any
fix, of which about 2,000 are one cascade (the `GadgetClass` constructor taking its own scoped
`FlagEnum` as `unsigned`), about 1,100 are `KeyNumType` alone, and 977 are array subscripts.
Classified by what the site needs:

| Site                                                              | Sites |
| ----------------------------------------------------------------- | ----- |
| `KeyNumType` and `KeyASCIIType` used as integers (keyboard codes) | 1,171 |
| Array subscript with an enum index (`ColorRemaps[color]`)         | 977   |
| `int` parameter given an enum (`Bit<T>(MZONE_X)`, `Ptr(house)`)   | ~900  |
| `ColorType` (`TBLACK`, `WHITE`, ...) given to an `int` parameter  | 361   |
| Arithmetic or comparison of an enum with `int` (`type + 1`)       | ~600  |
| Flag tests in a bool context (`if (flags & SHAPE_GHOST)`)         | 162   |
| Named enums used only as integer constants (`MAX_STEPS`, IDs)     | ~500  |

## Design

Every enum takes the first rule that applies. The goal is that a site changes only where a real
conversion happens, and that the conversion is spelled once, in a type, rather than at every use.

**1. An enum whose enumerators are only integer constants becomes `constexpr` constants.** All 177
unnamed enums are this (`enum { BUTTON_OK = 100, BUTTON_CANCEL }`, dialog layout `D_DIALOG_W`,
mission states `enum { INITIAL, DURING }` stored in an `int` field), and so is every named enum
whose name is never used as a type: `StepCountEnum`, `PowerEnums`, `GameOptionsButtonEnum`,
`CD_VOLUME`, `ConcreteEnum`, sdllib's `ColorType`. They become `constexpr int` (`static constexpr`
in a class, `inline constexpr` in a header at namespace scope), named `kPascalCase` as `CLAUDE.md`
asks of modernized code (`BUTTON_OK` → `kButtonOk`, `D_DIALOG_W` → `kDialogW`, `INITIAL` →
`kInitial`, `TBLACK` → `kTBlack`). A named constant set keeps its enumerator order and values.

**2. Every other named enum becomes `enum class`, with `using enum` in the same scope.** The
enumerator names stay: `STRUCT_WEAP`, `HOUSE_SPAIN`, `MISSION_ATTACK` and the rest are the data
vocabulary in tens of thousands of table rows, INI keys and scenario values, and the type safety the
check is after comes from the scope, not the spelling. `using enum Name;` directly after the
definition keeps unqualified uses resolving, including `Class::ENUMERATOR` for a nested enum. A
`typedef enum X {...} Y;` becomes `enum class Y {...};` under the name the code uses. A fixed
underlying type stays (`enum class HousesType : int8_t`); `CNC_FLAG_ENUM` stays and moves after
`enum class`.

**3. Tables indexed by an enum become `base::EnumArray<E, T>`.** A new aggregate in
`base/enum_array.h` wraps `T[magic_enum::enum_count<E>()]`, takes the enum in `operator[]`, and
iterates like `std::array`. `RemapControlType ColorRemaps[enum_count<PlayerColorType>()]` becomes
`base::EnumArray<PlayerColorType, RemapControlType> ColorRemaps` and every `ColorRemaps[color]` is
unchanged; a numeric index that was legal by accident is now a compile error. The 94 arrays already
sized by `enum_count` and the tables sized by a literal count (`Layer[LAYER_COUNT]`-style, the
`Verses` warhead matrix, per-type counters such as `BQuantity`) take this form; two-dimensional
tables nest it.

**4. A remaining enum-to-integer conversion is `static_cast<int>(e)`** (`static_cast<uint32_t>` for
a fixed unsigned underlying type), and an integer-to-enum conversion is `static_cast<E>(i)` as it
already was. Before casting, prefer the typed accessor that exists: `HouseClass::As_Pointer(house)`
over `Houses.Ptr(static_cast<int>(house))`, `base::Bit<T>(E)` (new overload) over
`Bit<T>(static_cast<int>(e))`. A function whose `int` parameter only ever receives one enum takes
that enum instead. A bitfield that stores an enum casts on store and load.

**5. Flag enums keep their operators and test with `base::Any`.** The games' `jshell.h` templates
and sdllib's per-type `operator|`, `&`, `~` already cast through `uint32_t` and work on scoped
enums. The one thing a scoped flag set loses is truth testing: `if (flags & SHAPE_GHOST)` becomes
`if (base::Any(flags & SHAPE_GHOST))`, where `Any(E)` (in `base/numeric.h`) is `!= 0` on the
underlying type. `int` masks such as `flags & 0xFF` cast through rule 4.

**6. `KeyNumType` and `KeyASCIIType` stay unscoped under a reasoned `NOLINT`.** A key number is a
16-bit pattern: the key code in the low byte, modifier bits above it, and `KN_BUTTON | id` for
gadget events, stored in the keyboard buffer as an integer and masked against `0xFF`, `char` values
and gadget IDs at 1,171 sites in the keyboard driver and every dialog. Scoping it adds a cast at
each of them and no safety; the composites are already built through `Build_Key_Number` and the
typed operators in `sdllib/keyboard.h`.

## Work split

The parent first lands the type-level work in one commit: `base/enum_array.h` with its test, the
`Bit` enum overload and `Any`, the scripted `enum class` conversion of every named enum, the typedef
renames, the `NOLINT` on the two key enums, the shared-header fixes (`defines.h`, `externs.h`,
`const.h`, `gadget.h`, `type.h`, the table declarations) and the constants for the unnamed enums
declared in headers, so that what remains is per-file. Fork agents then own disjoint `.cc` groups
split by error count (about 700 sites each): shared libraries, RA game objects, RA UI and network,
TD game objects, TD UI and network, both map editors. Each agent applies rules 1, 4 and 5 in its
files, verifies with
`clang-tidy -p cmake-build-strict-ra-clang --checks=cppcoreguidelines-use-enum-class <file>` (the
full enforced set plus the target), formats with `git clang-format -f -- <own files>`, and reports
any fix that needs a header outside its group for the parent to apply.

## Verification

The isolated sweep over all 936 compile commands must report nothing for the check; then the full
strict build with the check enabled, CTest, and both save/load smoke tests
(`tools/ra_saveload_smoke.sh`, `tools/td_saveload_smoke.sh` with every fixture) must pass. Enums
serialize through `static_cast<int32_t>` in `tech/archive.h`, so the save format is unchanged.
Scoping changes an enum's default underlying type from `unsigned int` to `int`; no enumerator is
above `INT_MAX` and the value is promoted to `int` in arithmetic either way, so no behavior changes.

## Risks

The volume: about 5,700 sites after the cascades, more than the signed-bitwise pass. The script does
the declarations and `EnumArray` absorbs the subscripts, so the hand work is the casts and the
constants. The `jshell.h` bitwise operator templates are unconstrained and match any type; they are
left as they are, and constraining them to enums is a follow-up. `using enum` at namespace scope
means two scoped enums in one scope still cannot share an enumerator name, exactly as before.
