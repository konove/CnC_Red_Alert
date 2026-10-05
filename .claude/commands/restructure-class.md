---
description: 'Give the classes a file declares the Google C++ class shape - a struct that behaves like a class becomes one, every data member private behind accessors, the declarations in the style guide''s Declaration Order (public/protected/private; types, constants, factories, constructors, destructor, functions, data), and the .cc definitions in the same order as the header. Use whenever the user asks to make a class''s members/variables/fields private, to "encapsulate" a class, to add accessors, to reorder or sort a class''s declarations or a .cc file''s functions, to "follow the declaration order", to turn a struct into a class (or says one "feels like a class"), or pastes the cppguide #Declaration_Order, #Access_Control or #Structs_vs._Classes link.'
---

Restructure the classes declared in: $ARGUMENTS

Two jobs, two commits per class, in this order: **make the data private**, then **put the
declarations and the definitions in order**. The first changes callers and adds accessors; the
second moves text and changes nothing. Keeping them apart is what makes each reviewable - the second
one's diff must be a pure move.

## 1. Scope

Every `class` and `struct` the named files declare, nested ones included, with the `.cc` that
defines their members. Callers anywhere in the scope of `/rename-google-style` section 1 follow (the
game's own tree for `src/ra` or `src/td`; all of `src/` for `src/game`, `src/engine`, `src/base`).
The other game's twin is its own pass.

Left alone:

- **Members inherited from a base declared elsewhere.** They are that header's pass. This class's
  own code keeps using them as it does now.
- **Types whose bytes are a format**: structs sent over the wire, read from a file, or copied with
  `memcpy`/`sizeof` (`/migrate-types` section 0). They stay passive structs and their field order
  does not move.

## 2. Class or struct

Decide this first; it settles which rule applies. The rule is the style guide's
<https://google.github.io/styleguide/cppguide.html#Structs_vs._Classes>: a struct is only for
passive objects that carry data; everything else is a class.

| The type is...                                                                                                                      | It becomes                                                                |
| ----------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------- |
| Passive data: fields a caller sets and reads freely, no invariant between them, at most a constructor, comparison and `Serialize()` | A `struct` with public fields named `snake_case` (no trailing underscore) |
| Anything else: it has behaviour, an invariant, virtual functions or a base class                                                    | A `class` with **every** data member private                              |

The keyword the type has today is not evidence: legacy code picked it by habit, and an earlier
stage's snake_case field names only record that nobody had asked yet. Judge by what the type
**does**. Any one of these makes it a class:

- A member function that acts rather than reports: it reads or changes the game, parses or formats
  itself, or carries out what the fields describe (`Perform()`, `Detach()`, `ReadIni()`,
  `ToIniEntry()`, `Draw()`).
- Fields that only make sense together: one says which of the others is live (a tag beside a union,
  a `need`/`kind` beside the pointers it selects), or a count mirrors a list.
- A field callers must not set freely: it is fixed up after load, cleared when its target dies, or
  kept in step with something outside the type.
- A base class or a virtual function.
- More functionality than helper methods that neither need nor enforce an invariant, or a type many
  files use and that is expected to grow.

Stateless types (traits, template metafunctions, some functors) may stay `struct`, as in the STL.

`ra::TriggerAction` (`ra/ai/trigger_action.h`) is the model case: declared `struct`, but it holds a
tagged union, detaches itself from dying types, reads and writes its INI form and performs the
action. It is a class. `Coordinate`-like value types with only a constructor, comparison and
`Serialize()` are structs.

A legacy `struct` that meets the test becomes a `class` - including one an earlier pass already gave
snake_case fields - and a legacy `class` that is only data becomes a `struct`. The keyword change is
its own commit, **before** section 3: `struct X {` -> `class X {` + `public:`, which changes no
access. Fix the forward declarations in the same commit
(`git grep -nE '(class|struct) <Name>;' -- src`); `-Wmismatched-tags` is an error in the strict
build. When in doubt it is a class.

## 3. Private data

In a class, no data member is public **or protected**: non-static, static and bit-field alike.
Derived classes go through accessors like everyone else. The only data that may sit in a public
section is a constant (`static constexpr kName`).

**Decide each member's accessors from how it is used outside the class**, not by stamping a
getter/setter pair on all of them. The uses are what the compiler reports once the member is private
(the method below), so this table is filled in while fixing them:

| Outside the class, the member is...                                                                      | Give it                                                                                                                                                                    |
| -------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Not touched                                                                                              | Nothing. Private, no accessor.                                                                                                                                             |
| Read                                                                                                     | `T name() const` for scalars, enums, pointers and bit-fields; `const T& name() const` for objects.                                                                         |
| Assigned                                                                                                 | `void set_name(T value)`.                                                                                                                                                  |
| Updated in place (`obj.count++`, `obj.flags \|= kX`)                                                     | `obj.set_count(obj.count() + 1)`. When two or more callers do the same update, a verb method that says what it means (`AddKill()`) - define it, and list it in the report. |
| An object used through its own methods (`obj.Timer.Start()`, `obj.Cargo.Attach(x)`)                      | `const T& name() const` plus `T& mutable_name()`; mutating callers write `obj.mutable_timer().Start()`.                                                                    |
| Passed by non-const reference or pointer, or bound to a span (an INI loader or dialog filling the field) | Rewrite the caller: a local, the call, then `set_name(local)`. Fall back to `mutable_name()` only when the callee keeps the reference.                                     |
| Used only by derived classes                                                                             | The same accessors, in the `protected` section.                                                                                                                            |

- Accessors are named like the variable (`CLAUDE.md` Naming): `quantity_` -> `quantity()`,
  `set_quantity()`, `mutable_quantity()`. A member still under its legacy name (`Quantity`), or
  under the plain `snake_case` it had as a field of a struct that section 2 made a class, is renamed
  to `snake_case_` in this same step, with the better name `/rename-google-style` section 3 would
  give it, so every caller is edited once.
- Getters are `const`. They and the setters are one-liners defined inline in the class, each with
  the member's comment moved up to the getter when the member's own comment would only repeat it.
- Inside the class's own member functions use the member (`quantity_`), not the accessor.
- Do not add `friend` declarations or `FRIEND_TEST` to avoid writing accessors, and review the
  friends already there: one that only reached data now uses the accessors and stops being a friend.
  Tests use the public interface.
- An accessor nobody calls is not added "for symmetry". A setter that would let a caller break an
  invariant the class maintains (a count that mirrors a list) is not added either: the writers get a
  method that keeps the invariant, or go on the report.
- Behaviour does not change. A place where a caller writes a member in a way that looks like a bug
  is reported, not fixed here.

**Method**, one class at a time, compiler-driven:

1. Move the data members to the private section and rename the legacy-named ones, leaving their
   relative order as it is (see section 4 on why the order matters).
2. `cmake --build build --parallel 22 -- -k 0 2>&1 | grep 'error:'`. Each
   `'x_' is a private member of 'X'` / `no member named 'Quantity'` is an exact `file:line:col`.
   Classify the use by the table, add the accessor it needs, patch the site, rebuild until clean.
   Over a few hundred sites, write the member -> accessor table to the scratchpad and split the
   files among fork agents.
3. A template in `src/game/` that reaches the member through a game-supplied type needs the other
   game's class to offer the same accessor. Add it there too (its own commit) or report it.
4. Code the compiler never sees (`#if 0`, `#ifdef WIN32`, excluded files): `git grep -nw` for each
   old member name over the scope.
5. Verify (section 6) and commit: "Make `<Class>`'s data members private".

## 4. Declaration order in the header

Per <https://google.github.io/styleguide/cppguide.html#Declaration_Order>. Each class has at most
one `public:`, one `protected:` and one `private:` section, in that order, empty ones omitted.
Inside each section:

1. Types and type aliases: `using`, `enum`, nested structs and classes, `friend` types
2. Static constants
3. Factory functions
4. Constructors and assignment operators
5. Destructor
6. All other functions: static and non-static member functions, friend functions
7. Data members, static and non-static

A struct may list its data members directly after its types instead.

- Legacy classes interleave several `public:`/`private:` blocks; merge them. A declaration's access
  never changes in this step - that was section 3.
- Within a group keep the existing relative order. It usually already groups related functions, and
  it keeps the diff readable. Section banners (`// Queries`) stay with their group when they still
  describe it and go when the merge made them wrong.
- **Data members keep their relative order.** It is the initialization order (`-Wreorder` is an
  error), default member initializers may read earlier members, and bit-fields pack by adjacency.
- Each declaration moves with its comment and any `#if` block around it.
- A method defined in the class body that is more than a trivial accessor or forwarding one-liner
  moves to the `.cc` (declaration stays, with the "what" comment), unless it is a template or the
  `.cc` does not exist. Do not add `inline` definitions to the header to keep it there.
- Free functions and constants the header declares outside a class are not reordered.

## 5. Definition order in the `.cc`

The `.cc` reads top to bottom like its header:

1. The includes, then file-local helpers: the anonymous namespace and `static` functions, constants
   and tables, in their existing order. A helper must precede its first use; one that cannot (it
   needs a class member defined below) keeps a forward declaration rather than breaking the order.
2. Static data member definitions, in the header's order.
3. For each class in the header's order: its member functions in the order the header declares
   them - so public before protected before private, and within each: factories, constructors,
   assignment, destructor, the rest.
4. The free functions the header declares, in the header's order.

- A function declared in some **other** header (a class whose members are spread over several `.cc`
  files, a hook) keeps its place relative to that header's order, after this file's own header's
  functions.
- Each definition moves with the comment above it and any `#if` block around it; nothing inside a
  body changes.
- The `_test.cc` is not reordered.

## 6. Verify

- The reorder is a pure move. For the header and the `.cc`, the sorted lines before and after must
  match apart from access specifiers, blank lines, and any body moved out of the class:
  `diff <(git show HEAD:<file> | sort) <(sort <file>)`. Then read
  `git diff --color-moved=zebra --color-moved-ws=ignore-all-space`: anything not coloured as moved
  is a change to explain or undo.
- `git clang-format -f -- <touched files>`, never `clang-format -i`.
- `cmake --build build --parallel 22 && ctest --test-dir build`.
- The strict checks: `tools/strict_tu.py <touched files>` when a multi-stage run has deferred the
  full pass, otherwise `cmake --build build-strict --parallel 14` in the foreground. This is where
  `-Wreorder`, `-Wmismatched-tags` and `readability-make-member-function-const` on a new accessor
  show up.
- For a class that is saved: `Serialize()` is field-wise, so neither step changes a save, but run
  the game's save/load smoke test (`tools/ra_saveload_smoke.sh` / `tools/td_saveload_smoke.sh`) when
  a serialized class's members moved.
- `git grep -nE '^\s*(public|protected|private):' -- <header>` shows each specifier at most once per
  class, in order.

Commit only when asked, through `/commit`: per class, "Make `<Class>`'s data members private", then
"Put `<Class>`'s declarations in the Google declaration order" (header and `.cc` together); a
`class`/`struct` keyword change ("Make `<Name>` a class") and a twin's accessor each on their own.

## 7. Report

- Per class: struct or class and why; the member -> accessor table, with the members that got none.
- The verb methods added in place of setters, and the `mutable_` accessors with why a setter did not
  do.
- Friends removed or kept, and why.
- What was left: inherited members from other headers, format structs, a template that needs the
  twin.
- Noticed but not changed: members nobody reads, callers that break an invariant, setters only one
  caller uses, a class that looks like two.
