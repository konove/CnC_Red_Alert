---
description: 'Move engine code that sits in the global namespace into its library''s namespace (gfx::, stream::, codec::, window::, ...) and qualify every caller, following the Google C++ style guide''s Namespaces and Internal Linkage rules. Use whenever the user asks to put a file or an engine folder "in a namespace", to "namespace" the streams / PixelBuffer / Display, to follow the styleguide''s namespace rules, or pastes the cppguide #Namespaces or #Internal_Linkage link.'
---

Move into its namespace: $ARGUMENTS

The rules of the style guide are short: put code in a namespace, never `using namespace`, no inline
namespaces, nothing in `std`, internal linkage for `.cc`-local definitions, no unnamed namespaces or
`static` definitions in headers. The tree already obeys all of them except the first, and clang-tidy
enforces the rest. So this command is almost entirely about the first rule, and the work is not the
`namespace gfx {` line - it is everything outside the files that silently stops meaning the same
thing once the names move.

## 0. The project's policy

These are decided; apply them, do not reopen them.

| Question                    | Answer                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| --------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Which code                  | `src/engine/` only. `src/ra` and `src/td` stay in the global namespace; namespacing the games is a separate plan. Their small `config`, `dib` and `td_save_detail` namespaces stay as they are.                                                                                                                                                                                                                                                                |
| Which namespace             | The file's **library**, per `tools/engine_layout.py`: the library name without `engine_`. A subfolder shares its library's (`base/strings` -> `base`, `platform/win32` -> `platform`, `net/serial` -> `net`); `video/vqa` is its own library, so `vqa`. No project prefix: `945b3f29` named them after the folders and nothing else, so no `cnc::`.                                                                                                            |
| How callers name it         | Fully qualified at every use: `gfx::PixelBuffer`, as the tree already writes `base::ssize` and `base::SafeCopy`. No using-declarations, no namespace aliases - the tree has none. This holds however often a file uses a type or function (`ra/saveload.cc` writes `stream::FourCC` 56 times); the only exception is the enumerator row below. Inside the namespace, including other files of the same library, no qualifier.                                  |
| Enumerators                 | The enum's own `using enum X;` hoist (`engine/codec/aud_decoder.h:47` and 160 like it) moves inside the namespace with the enum. Outside it, headers qualify (`window::KN_UP`); a `.cc` that uses the enum's enumerators three or more times writes `using enum window::KeyNumType;` once, after its includes, and keeps its uses bare; one with fewer qualifies them. Never a `using enum` at namespace scope in a header (`google-global-names-in-headers`). |
| Internal linkage in a `.cc` | Unchanged: functions and variables keep `static`, `.cc`-local types go in one `namespace { ... }  // namespace` block (see `.clang-tidy`'s `misc-use-internal-linkage` comment). Both now sit inside the named namespace. Never `static` inside `namespace {}` (`readability-static-definition-in-anonymous-namespace`).                                                                                                                                       |
| Helpers a header must show  | Declarations that exist only so a template or inline function in the header can use them go in `<ns>::internal` (`namespace gfx::internal {`), which says "not API" the way the style guide asks.                                                                                                                                                                                                                                                              |
| Layout                      | `namespace gfx {` after the includes and after any forward declarations of other namespaces' types, no indentation, `}  // namespace gfx` before the include guard's `#endif`. Nested: one line, `namespace gfx::internal {`.                                                                                                                                                                                                                                  |

## 1. Scope

The unit is a **library**, or the file pair the user named (a header-only file, such as
`engine/stream/archive.h`, has no `.cc`; its `_test.cc` is still in scope). A whole library at once
is cheaper: moving one file of `gfx` while its siblings stay global means those siblings write
`gfx::` now and drop it again when they move. When the user names one file, do that file and say in
the report which siblings will need their qualifiers removed later.

A library whose files are partly namespaced already (`crypto`, `file`, `net`, `platform`) keeps the
namespace it has; this pass brings the rest of its files in.

## 2. Inventory, before editing

Write a table to the scratchpad of every name the files declare at namespace scope - classes,
structs, enums, type aliases, templates, free functions, operators, global variables and function
pointers, constants - with where it goes and its fan-out (`git grep -nw '<name>' -- src | wc -l`).
Most rows are "into the namespace". These are not, or need more than the wrapper:

**Hooks the games define.** An engine header can declare a function each game defines: `Prog_End()`
in `engine/window/misc.h` ("Each game defines it in its startup.cc"). Find them by locating every
declared function's definition: one under `src/ra` or `src/td` is a hook. Moved into the namespace,
the declaration becomes `window::Prog_End` while both games still define `::Prog_End`, and the link
fails. Keep hooks global: declare them after the namespace block under a comment saying the game
defines them. Moving a hook in (and making each game define it inside `namespace window { }`) is a
design change; list it for the user instead.

**Test stubs.** The opposite case: `engine/window/keyboard_test.cc` compiles `keyboard.cc` alone and
defines `SDL_Event_Loop()` and `Update_Mouse_Pos()` itself at global scope, standing in for the real
ones in `ww_win.cc` and `ww_mouse.cc`. Those are library functions, so they move into `window`, and
`keyboard.cc`'s calls then mean `window::SDL_Event_Loop` - the global stubs no longer satisfy them.
A stub goes in the namespace of the function it replaces. `git grep -n 'stub' -- 'src/engine/**'`
and the library's `CMakeLists.txt` (`add_gtest` entries that list a `.cc` from the library in
`SOURCES`) find them.

**Things that belong to another namespace.** A forward declaration of someone else's type inside the
file (`union SDL_Event;` in `engine/window/keyboard.h`, a `class` from another engine library) stays
outside the block; inside, it would declare a new `window::SDL_Event` that nothing defines. Macros
ignore namespaces: leave them where they are. Specializations of `std::hash`,
`magic_enum::customize::enum_range`, `fmt`/absl traits and the like must be written at their own
namespace's scope, after this namespace closes, naming `gfx::X`. `extern "C"` functions and anything
passed to SDL as a C callback keep their linkage; leave them where they are.

**Functions found by ADL.** Free `operator==`, `operator<<`, `AbslHashValue`, `AbslStringify`,
`PrintTo`, `swap` move with their type into the same namespace. Left global, they are found only
from global-namespace code: gtest and absl look in the type's namespace, so `EXPECT_EQ` starts
printing bytes or stops compiling, and any same-named operator declared inside `gfx` hides the
global one from `gfx`'s own code.

**Forward declarations elsewhere.** `class ArchiveReader;` appears in 72 game headers. Once the real
class is `stream::ArchiveReader`, each of those declares a second, unrelated `::ArchiveReader`, and
the errors surface far away as incomplete types or failed overload resolution
(`bugprone-forward-declaration-namespace` catches them in the strict build). Rewrite each as one
block, `namespace stream {` / `class ArchiveReader;` / `class ArchiveWriter;` /
`}  // namespace stream`. Find them with
`git grep -nE '^\s*(class|struct|enum class) <Name>\b[^{]*;' -- src`. The qualification script
rewrites them whatever the order - even inside a block you already wrote - into the invalid
`class stream::ArchiveReader;`, so fix them **after** it runs (section 3, step 7), and
`git grep -nE '(class|struct) <ns>::' -- src` must come back empty.

**Redeclarations.** An `extern gfx_type Name;` or a function prototype repeated in a game header
(`ra/externs.h`, `function.h`) instead of including the engine header declares a different global
and fails at link time. `git grep -nw '<name>' -- src/ra src/td | grep -E 'extern|\(.*\);'`.

**Friends.** `friend void Foo(Bar&);` in a class that moves into `gfx` now befriends `gfx::Foo`. If
`Foo` moves too, nothing changes; if it stays global, write `friend void ::Foo(Bar&);` with `Foo`
declared before the class.

**Name clashes.**

- The namespace name must not already name a global type, function or variable
  (`git grep -nwE '(class|struct|enum|using) <ns>\b'`). Local variables called `file` or `stream` do
  not clash: the name before `::` is looked up among namespaces and types only.
- Inside the namespace, a declared name hides every global of the same name for unqualified lookup,
  and it can do so silently when the overloads are compatible. For each function the files declare,
  `git grep -nwE '<Name>\s*\(' -- src` for a same-named global elsewhere; if the moved code calls
  that global, write `::Name(...)`.
- The same name declared in the other games' own code (`td/` has twins of many engine names) is not
  a clash - it is a different function - but it does mean a scripted qualification must not touch
  the game's own declaration and uses. The compiler will say so.

**Configuration that names types.** `.clang-tidy`'s
`bugprone-throwing-static-initialization.AllowedTypes` lists engine types (`PixelBuffer`,
`PixelView`, `GameFile`). Whether an unqualified entry still matches `gfx::PixelBuffer` depends on
the check, so do not assume: `git grep -nw '<name>' -- .clang-tidy .iwyu_mappings cmake` for every
type, and if the strict build newly flags the static objects those entries covered (`radar.cc`),
qualify the entries (`gfx::PixelBuffer`).

## 3. Apply

1. **Redeclarations** in game headers first (section 2): replace them with the engine header's
   include, or qualify the type.
2. **Headers:** open `namespace <ns> {` after the includes and forward declarations, close it with
   `}  // namespace <ns>` before `#endif`. Hooks and other-namespace specializations go after the
   close. Do not re-indent.
3. **Sources:** the same wrapper around everything after the includes. Define members inside the
   namespace (`PixelBuffer::Clear()`, not `gfx::PixelBuffer::Clear()` at global scope). The existing
   `static` functions and the `namespace {` block move inside unchanged. A `.cc` that defines a hook
   defines it outside the block.
4. **Tests** go in the namespace with their own unnamed namespace inside, as
   `engine/base/numeric_test.cc` does: `namespace gfx {` / `namespace {` ... `}  // namespace` /
   `}  // namespace gfx`. A test stub for a hook stays global; a stub for a moved function moves
   too.
5. **Enumerators, in `.cc` files first.** Count each caller `.cc`'s uses of the enum's enumerators;
   where there are three or more, add `using enum <ns>::<Enum>;` after the includes and leave the
   file out of the enumerator mapping in the next step. Everything else - headers and the light
   users - gets its enumerators qualified with the other names.
6. **Callers.** Script the qualification with `tools/rename_globals.py`, which already skips
   comments, literals and uses preceded by `.`, `->` or `::`: a spec with `"root": "src"`,
   `"mapping": {"PixelBuffer": "gfx::PixelBuffer", ...}`, the library's own files under `"skip"`,
   and any file where the name is also a class member under `"exclude"` (then do those by hand). Its
   quote skipper is naive: a digit separator (`1'000`, as in `ra/saveglobals_test.cc`) or an English
   apostrophe in a comment leaves it "inside a literal" for the rest of the file - the two misses in
   `945b3f29` - so finish with a grep for unqualified uses, not with trust. For names with a handful
   of callers, let the compiler find them instead:
   `cmake --build build --parallel 22 -- -k 0 2>&1 | grep -E 'error:.*(not declared|did you mean)'`.
   Neither the build nor the compiler-driven loop sees branches this platform does not compile -
   `#ifdef _WIN32`, WOLAPI, `DEMO`, TD's dead `FRENCH`/`GERMAN` blocks - so the closing grep is what
   covers them. Other engine libraries that use the names are callers like any other.
7. **Forward declarations**, now that the script has turned them into
   `class stream::ArchiveReader;`: rewrite each file's run of them into one
   `namespace stream { ... }  // namespace stream` block at the same place (a short script over
   `git grep -lE '(class|struct) <ns>::' -- src`), until that grep is empty.
8. **Code examples in comments** (`// Example:` blocks, `CLAUDE.md` snippets) qualify the names so
   they still compile as written. Prose that mentions `PixelBuffer` in passing stays as it is, and
   so do finished `docs/*_PLAN.md` files, which record the tree as it was.
9. `git clang-format -f -- <every touched file>` - never `clang-format -i`, which reflows the
   untouched legacy code around the edits.

The diff is the wrapper plus qualifiers, nothing else. Names that should change, dead declarations
and the bugs found while reading go in the report for `/rename-google-style` and
`/remove-dead-code`.

## 4. Verify

- `cmake --build build --parallel 22 && ctest --test-dir build --output-on-failure`. Both games and
  every test link: a hook left inside the namespace shows up here as `undefined reference`.
- Nothing is left global:
  `nm -C --defined-only build/src/engine/<folder>/libengine_<ns>.a | grep -E ' [TDBRWV] ' | grep -vE ' (<ns>|std|absl)::'`
  lists the library's external symbols outside its namespace (uppercase letters only, so `static`
  functions drop out). It should show only the hooks, `extern "C"` functions and other namespaces'
  specializations the report names. Run it before starting too: on a partly namespaced library it is
  the inventory of what is left (`libengine_base.a` still shows `Buffer::Buffer(long)`). A
  header-only file emits no symbols of its own, so for it this check proves nothing; the grep below
  carries the weight.
- No unqualified use survives outside the namespace:
  `git grep -nwE '<Name>' -- src/ra src/td src/engine | grep -v '<ns>::<Name>'` over the table, read
  by hand (member names and the other game's twins will match and are fine).
- The strict checks on every touched file: `tools/strict_tu.py <files>` during a multi-stage run,
  otherwise `cmake --build build-strict --parallel 14` in the foreground. This is where
  `bugprone-forward-declaration-namespace`, `google-readability-namespace-comments`,
  `misc-anonymous-namespace-in-header` and `google-build-using-namespace` speak. A header with a
  wide fan-out (`gfx`, `stream`) re-analyzes hundreds of units; that is expected.
- `git diff -U0 | grep '^[-+]' | grep '"'`: nothing qualified inside a string literal.

Commit only when asked, through `/commit`: one commit per library, since the wrapper and the
qualified callers cannot build apart. The message lists the hooks kept global and any clash found.

## 5. Report

- The namespace, and the counts: names moved, call sites qualified, forward declarations rewritten.
- What stayed global and why - each hook with the game files that define it, specializations,
  `extern "C"`.
- Clashes and hidden-name cases found, and how each was resolved.
- For a single-file pass, the sibling files that now qualify names they will drop when they move.
- Noticed but not changed: bad names, dead declarations, hooks that could become a design change.
