# Plan: enable `portability-template-virtual-member-function`

Written 2026-09-26 against `.clang-tidy` and clang-tidy 23.1.2. Companion to the row in
[CLANG_TIDY_PRIORITIES.md](CLANG_TIDY_PRIORITIES.md).

**Status: complete (2026-09-26).** The check is enabled; the results are in the "Template virtual
member enablement" review in CLANG_TIDY_PRIORITIES.md.

## Context

The row was skipped on 2026-09-12 with 35 reports, on the grounds that silencing it needs either an
explicit instantiation of every specialization or non-virtual heap and vector members. Every other
excluded P1–P3 name is now either enabled or a recorded policy skip, so this is the next check with
a defect story that the code can actually satisfy.

The check reports a virtual member of a class template specialization that the translation unit does
not instantiate. Clang and GCC instantiate a virtual member only where they emit the vtable; other
compilers (MSVC among them) instantiate every virtual member of every specialization they see, so a
member that does not compile for some `T` breaks only there.

An isolated sweep on 2026-09-26
(`clang-tidy --checks=-*,portability-template-virtual-member-function` over the 1,080 project files
in `cmake-build-strict-ra-clang/compile_commands.json`, 2 min 17 s at `-P 16`) reports **40 unique
locations in 7 headers**:

| Header                             | Reports | Specializations reported                                               |
| ---------------------------------- | ------- | ---------------------------------------------------------------------- |
| `td/vector.h`                      | 10      | Vectors named by the `extern template` lines at the end                |
| `ra/heap.h`                        | 9       | `TFixedIHeapClass<T>` members of `ObjectHeaps`, `TypeHeaps`            |
| `td/heap.h`                        | 6       | `TFixedIHeapClass<T>` members of TD's `ObjectHeaps`                    |
| `ra/vector.h`                      | 6       | `VectorClass<CellClass>` in `MapClass`, `DynamicVectorClass`'s base    |
| `ra/vector_dynamic.h`              | 4       | Dynamic vectors held by `BaseClass`, `LayerClass`, `SessionClass`, ... |
| `engine/stream/transform_sink.h`   | 3       | `TransformSink<BlowfishCodec>`, `TransformSink<Sha1Codec>`             |
| `engine/stream/transform_source.h` | 2       | `TransformSource<BlowfishCodec>`, `TransformSource<Sha1Codec>`         |

## Findings

**Explicit instantiation does not help.** Small probes show that an `extern template class`
declaration _causes_ the report rather than silencing it: the declaration suppresses the vtable, so
no virtual member is marked used, and every unused one is reported. That is why TD's vectors, which
carry `extern template` lines, report while most of RA's, which do not, stay quiet. Only an explicit
instantiation _definition_ in the same translation unit silences it, which is one TU out of
hundreds.

**The heap and vector virtuals are never dispatched dynamically.**

- `TFixedIHeapClass<T>` introduces `ID(const T*)`, `Logical_ID(const T*)`, `Alloc`, `Free(T*)`,
  `Ptr` and `Raw_Ptr` as virtual, and nothing derives from it, so no call can reach an override. Its
  overrides `Logical_ID(int)` and `Free(void*)` only forward to the base. `Validate()` is the one
  override with behavior (RA only: the stale-ID check for objects with an `ID` field), and every
  caller (`ObjectHeaps::Validate`, the heap tests) calls it on the typed heap.
- `VectorClass<T>` declares its destructor, `operator==`, `Resize`, `Clear` and both `ID`s virtual;
  `DynamicVectorClass<T>` overrides all but `operator==`. No code holds a `DynamicVectorClass`
  through a `VectorClass` pointer or reference (the only `const VectorClass<int>&` are two tests
  reading `at()`), and none deletes one through the base. The only virtual calls that can land in
  the derived class are `Clear()` from `VectorClass::operator=` and TD's `VectorClass::Resize(0)`;
  in both, `DynamicVectorClass` then overwrites or clamps `ActiveCount` itself (the implicit copy
  assignment copies it, `DynamicVectorClass::Resize` clamps it to the new length), so the base
  `Clear()` leaves the same state.
- The classes deriving from the vectors — both games' `LayerClass` and TD's `GScreenClass` — add
  their own virtuals but override none of the vector's. Save games are field-wise `Serialize()`, so
  dropping the vector's vptr changes no file format.

**The transform adapters must stay virtual.** `TransformSink<C>` and `TransformSource<C>` implement
the `ByteSink`/`ByteSource` interfaces, which is their whole purpose. They are reported only where
`BlowfishSink`, `BlowfishSource`, `Sha1Sink` and `Sha1Source` derive from them in a header, so every
TU that includes one instantiates the base without its vtable. Here the hazard the check guards
against cannot occur: `C` is constrained by the `ByteCodec` concept, and the overrides use only the
operations the concept requires, so any `C` that satisfies the constraint instantiates every one of
them. Removing the virtuals would mean turning the concept into an abstract codec class and
type-erasing the adapters, a redesign of the stream layer for a check the constraint already
satisfies.

## Plan

1. **Heaps (both games).** Make `TFixedIHeapClass<T>`'s own members non-virtual. Replace the
   forwarding `Free(void*)` override with `using FixedIHeapClass::Free;` and delete the forwarding
   `Logical_ID(int)` override (RA's `using FixedIHeapClass::Logical_ID;` already exposes it). Move
   RA's stale-ID check into `FixedIHeapClass::Validate()`, which becomes non-virtual: the typed heap
   hands its base a function that reads a slot's `ID` field (null when `T` has none, per
   `HasHeapId<T>`), so one `Validate()` in `heap.cc` covers both checks.
2. **Vectors (both games).** Drop `virtual` from `VectorClass<T>` and `override` from
   `DynamicVectorClass<T>`; the derived class's `Resize`, `Clear` and `ID` hide the base's. Give
   `LayerClass` (both games) a plain virtual destructor instead of `override`. Update TD's
   `Copy_From` comment, which explains itself by `Clear()` being virtual.
3. **Transform adapters.** Suppress the five reports in `transform_sink.h` and `transform_source.h`
   with one reasoned comment: the `ByteCodec` constraint makes every override instantiable for every
   admitted codec.
4. **Enable.** Remove `-portability-template-virtual-member-function` from `.clang-tidy`, re-sweep
   to zero, run the full strict build and the tests, and record the result in
   CLANG_TIDY_PRIORITIES.md (row and a review section).

## Verification

- The isolated sweep above reports zero, and no `clang-diagnostic-error` in any TU.
- Full strict build (`cmake --build build-strict --parallel 14`) and the GCC build (`build/`)
  succeed; `ctest` passes, including the heap, vector, layer and stream tests.
- `tools/ra_saveload_smoke.sh` and `tools/td_saveload_smoke.sh` still match, since the heaps and
  layers they round-trip are what changed.
