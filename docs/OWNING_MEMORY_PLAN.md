# Plan: enable cppcoreguidelines-owning-memory

Written 2026-09-26 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the
`cppcoreguidelines-owning-memory` row in [CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-26).** The check is enforced; the results are in the "Owning memory
enablement" review in CLANG_TIDY_PRIORITIES.md. Step 1 is 3301f035, step 2 b5fac972 (1,044 creations
and 169 deletions of pooled objects, far more than the 180 reported `new` sites because the check
does not report a `new` whose result is discarded), steps 3 and 4 6bdd2b15 (528 findings in 81
files, by seven parallel agents on disjoint file groups).

## Context

No row in CLANG_TIDY_PRIORITIES.md is Pending, so the next work is re-measuring the skipped ones. An
isolated sweep on 2026-09-26 (every `src/` translation unit of
`cmake-build-strict-ra-clang/compile_commands.json`, header-verify units included, findings
deduplicated by location) gave:

| Check                                                 | 2026-09-12 | 2026-09-26 |
| ----------------------------------------------------- | ---------- | ---------- |
| `cppcoreguidelines-pro-bounds-array-to-pointer-decay` | 4,423      | 2,290      |
| `modernize-avoid-c-arrays`                            | 2,252      | 1,997      |
| `cppcoreguidelines-pro-type-union-access`             | 1,438      | 1,521      |
| `cppcoreguidelines-owning-memory`                     | 1,240      | 843        |
| `misc-use-anonymous-namespace`                        | 766        | 626        |
| `bugprone-easily-swappable-parameters`                | 426        | 378        |
| `cppcoreguidelines-avoid-non-const-global-variables`  | 1,399      | 298        |

`owning-memory` is the first skipped P3 row and the one with direct defect value: every finding is a
place where the type system does not say who frees an allocation, which is how leaks and double
frees happen (the type objects' `DimensionData`/`RadarIcon` leak in `ra/type_heaps.cc` is one). It
was skipped on 2026-09-12 because enabling it means smart pointers or `gsl::owner` on every raw
`new`/`delete`, which CLAUDE.md lists under changes to avoid unless requested. This plan is that
request; the project treats modernization as the goal (see the decision in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md#legacy-code-policy-exclusions-2026-09-12)).

## Findings

843 findings in 137 files: `base` 6 (4 files), `engine` 22 (5), `ra` 443 (72), `td` 372 (56); 29 are
in tests.

| Message                                                    | Count |
| ---------------------------------------------------------- | ----- |
| deleting a pointer through a type not marked owner         | 312   |
| assigning a newly created owner to a non-owner             | 239   |
| initializing a non-owner with a newly created owner        | 154   |
| passing a newly created owner as a non-owner argument      | 114   |
| returning a newly created owner from a non-owner function  | 19    |
| calling a legacy resource function (`realloc`, `fopen`, …) | 15    |

Probing the check (`std::unique_ptr<T> p(new T)` passes, `p.reset(new T)` does not,
`std::make_unique` and `std::unique_ptr<char[]>(new char[n])` pass) and reading the sites sorts them
into these patterns:

1. **Pool-owned game objects (180 `new` sites, most of the deletes).** `AnimClass`, `UnitClass`,
   `BulletClass`, `TeamClass`, the type classes and the rest of the classes with a class-specific
   `operator new` allocate from their `TFixedIHeapClass` in `ObjectHeaps`/`TypeHeaps`; that pool
   owns the object, iterates it (`TheObjectHeaps().anim().Ptr(i)`) and frees it through the matching
   `operator delete`. `operator new` is `noexcept` and returns `nullptr` when the pool is full, so
   callers test the result. The caller never owns these objects, which is exactly what the check
   cannot see. 86 `delete this` sites are not reported.
2. **Function-local lifetime.** Dialog gadgets (`TextButtonClass`, `ShapeButtonClass`,
   `StaticButtonClass`, `ToolTipClass`), score-screen print objects, temporaries in tests: created
   and deleted within one function.
3. **Owning members and subsystem fields.** `SerialPort`, `TheNetwork().modem_registry()`,
   `packet_transport()`, `wolapi()`, the IPX `Connection[]` array, `CommBufferClass Queue`,
   `EgoClass`/`PixelBuffer` slides, `g_tick_timer`, `Game* game`, the MIX archive handles in
   `installation.cc`/`conquer.cc`.
4. **Containers of owned pointers.** `TheSession().Players`/`Chat`/`PhoneBook`/`Scenarios`,
   `TheNetwork().games()`/`phone_book()`, the load dialog's `Files`, `ScreenList`, `ExtrasList`,
   `OutBuffers`, the WOL `User`/`Channel`/`Ladder` chains, `packet.cc`'s `FieldClass` list.
5. **Owned buffers.** `new char[n]` in `base/buff.cc`, `safe_string.cc`, `packet.cc`, event
   `Variable.Pointer` payloads, TD's intro/ending audio and palette buffers.
6. **C resources.** `realloc` in `engine/platform/mem.cc` (the allocator itself) and `fopen` debug
   dumps in `queue.cc`.

## Plan

Ownership rules, in order of preference (the fork recipe carries these verbatim):

- A local whose lifetime is the function becomes a stack object; if it must be a pointer
  (polymorphic, large, conditionally created), `std::unique_ptr` via `std::make_unique`.
- An owning member, global or subsystem field becomes `std::unique_ptr<T>` (or `std::optional<T>`
  when the type is concrete and never shared). Accessors that returned `T*&` for callers to
  `delete`/assign get `reset`-style mutators; readers take `.get()` or a reference.
- A container of owned pointers becomes `std::vector<std::unique_ptr<T>>`, or a container of values
  when nobody holds pointers to the elements.
- A function that takes ownership takes `std::unique_ptr<T>`; a factory returns one.
- A buffer becomes `std::vector<T>`, or `std::unique_ptr<T[]>` via `std::make_unique_for_overwrite`
  where it is filled immediately and never resized.
- Pool-owned objects go through `base::NewPooled<T>(args...)` and `base::DeletePooled(p)` (below).
- `NOLINT(cppcoreguidelines-owning-memory)` only where ownership truly lives outside C++ (a C or SDL
  API, the allocator itself), always with the reason on the same line.

Steps:

1. **Pool helper.** Add `base/pooled.h`: `NewPooled<T>(args...)` runs `new T(...)`, returning
   `nullptr` when `T::operator new` does, and `DeletePooled(T*)` runs `delete`; both require a
   class-specific allocation function or a virtual destructor, so they cannot be used on an ordinary
   heap type, and each carries the one documented suppression. Unit test with a small pooled type.
   Commit.
2. **Pooled sites, RA and TD.** Script `new <PooledClass>(` → `base::NewPooled<PooledClass>(` and
   fix the forwarding fallout (`0` passed for a pointer becomes `nullptr`); then re-sweep and turn
   the deletes of pooled objects into `DeletePooled`. One commit per game.
3. **Engine, base and tests.** Buffers, `packet.cc`'s field list, `mix_archive.cc`, the tick timer,
   `mem.cc`, the test fixtures. Commit.
4. **RA and TD by area, in parallel forks** on disjoint file groups (game logic, UI/dialogs, network
   and WOL, per game), each following the recipe and verifying each file with
   `clang-tidy -p cmake-build-strict-ra-clang --quiet --checks=cppcoreguidelines-owning-memory` (the
   full enforced set plus the target). Changes to a header another group owns are reported to the
   parent and applied centrally. One commit per game area.
5. **Enable.** Remove the name from `.clang-tidy`, re-sweep to zero with no
   `clang-diagnostic-error`, run the full strict build, the GCC build, CTest and both save/load
   smoke tests, and record the result: the row Enabled, a review section, and a note on the
   2026-09-12 legacy-code policy section that it is superseded for this check.

## Verification

- The isolated sweep of `cppcoreguidelines-owning-memory` over every TU reports nothing.
- `cmake --build cmake-build-strict-ra-clang --parallel 14` and `cmake --build build --parallel 22`
  succeed; `ctest` passes in both.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` pass against the strict binaries:
  object creation and destruction change spelling on every save/load path.
- `build-asan` (`-DENABLE_ASAN=ON`) runs CTest without new leak or use-after-free reports.
