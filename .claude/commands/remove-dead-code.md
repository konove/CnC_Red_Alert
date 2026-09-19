---
allowed-tools: Read, Grep, Glob, Edit, Bash
description: 'Remove dead code. Two modes - sweep a file for commented-out code, #if 0 blocks and unreachable statements; or remove a specific thing the user says is dead across the tree - a parameter nobody passes, one every caller passes the same value for ("always called with 1", "the default is never overridden"), a parameter the body ignores, or a function with no callers. Use whenever the user observes that something is unused or constant and wants it gone, even if they never say "dead code".'
---

Remove dead code: $ARGUMENTS

The argument is either **file(s)** to sweep (the patterns under "What Counts as Dead Code"), or a
**claim** about a parameter or function ("nobody passes `magic_color` to `Animate_Frame`",
"`Draw_Box` is always called with `true`"). A claim goes through "Dead parameters and functions"
below, which works across the tree rather than in one file.

## What Counts as Dead Code

### Commented-out code

Lines that are clearly disabled **code**, not documentation or explanatory comments. Look for:

- Commented-out function calls, variable declarations, assignments, control flow
- Blocks of code with `//` or `/* */` that contain recognizable C++ syntax (semicolons, braces,
  operators)
- Do **NOT** remove: documentation comments, TODOs, explanatory notes, license headers, section
  dividers

### `#if 0` / `#if false` blocks

Preprocessor-disabled code sections. Remove the entire block including the `#if 0`, `#else` (if
present), and `#endif`. If there is an `#else` branch with live code, keep that code and remove only
the dead `#if 0` branch and its preprocessor directives.

### Unreachable code

Statements that can never execute:

- Code after an unconditional `return`, `break`, `continue`, or `goto` within the same block
- Do **NOT** remove code after a conditional return (e.g., `if (x) return;` — the code after is
  reachable)
- Be careful with switch/case: `break` ends a case, but code in the next `case:` label is reachable

### Dead conditional branches

- `if (false) { ... }` or `if (0) { ... }` — remove the entire if block
- `while (false) { ... }` or `while (0) { ... }` — remove the entire while block
- `if (true) { ... } else { ... }` — keep the if-body, remove the else branch and the condition
- For `if (true)`, unwrap the body (remove the `if` and braces, keep the contents) only if it
  doesn't introduce scoping issues

## Process

1. **Read the entire file** to understand its structure and purpose
2. **Scan for each dead code pattern** listed above
3. **Verify each candidate** before removing:
   - Is the commented-out code actually dead, or is it an explanatory example in a comment?
   - Is the `#if 0` block truly dead, or is it a configuration option toggled by build flags?
   - Is the code after `return` truly unreachable, or is there a label that `goto` jumps to?
   - Could the dead branch be intentional (e.g., debug scaffolding with a `constexpr bool`)?
4. **Remove the dead code** using the Edit tool. Clean up:
   - Trailing blank lines left behind (collapse multiple blank lines to one)
   - Dangling `else` that no longer has an `if`
   - Empty blocks left after removal
5. **Do NOT remove or modify**:
   - Code that is merely unused but still reachable (uncalled functions, ignored parameters) during
     a file sweep — proving that takes the tree-wide search in the next section. List what you
     noticed as candidates in the summary instead, so the user can ask for them by name
   - Preprocessor guards for platform portability (`#ifdef __linux__`, `#ifdef __APPLE__`, etc.)
     unless the user explicitly asks
   - Comments that document behavior, even if they reference removed code

## Dead parameters and functions

Used when the user names the thing: a parameter nobody passes, one that always gets the same value,
one the body ignores, or a function nothing calls. The edit is trivial; the work is proving the
claim, because it is a statement about every caller in the tree and the user made it from the few
they were looking at. Treat it as a hypothesis. If it turns out false, stop and show the
counterexample — that answer is as useful as the removal.

### 1. Find every declaration

The signature lives in more places than the `.h`/`.cc` pair:

- **Virtual chains.** `grep -rn '\bName *(' src/` for the base and every override. The parameter
  goes only if _no_ override reads it and no caller through any base passes something else; then the
  whole chain changes together (`79642f79` changed `Draw_Me(int forced)` to `bool` this way). An
  override declared in a file you did not open still has to match, or it silently stops overriding —
  `override` turns that into a compile error, so add it where it is missing first.
- **RA and TD twins.** `src/ra` and `src/td` are separate targets with same-named, separately
  declared functions. Work in the game the user named; check the twin and report whether the same
  claim holds there, but change it only if asked. A function in `sdllib`, `tech`, `port`, `base` or
  `winvq` is shared, so its callers are in all of `src/`.
- **Overloads.** Make sure each call you count resolves to _this_ overload.

### 2. Find every caller, including the ones the build does not compile

`grep -rn` is the list; the compiler is only a check on it. Going the other way round — delete the
parameter and fix what breaks — misses two kinds of caller:

- Calls the default build never sees: `#ifdef WIN32`, the WOLAPI sources, `CHEAT_KEYS` /
  `config::k...` blocks, Emscripten branches. They rot silently if skipped.
- Calls that still compile and now mean something else. Remove the `int` from
  `F(obj, int frame, bool redraw = true)` and `F(obj, 1)` compiles happily with `redraw = 1`.
  Whenever a later parameter has a default, or is convertible from the removed one's type, check
  each call site by eye and count them before and after.

Also look for uses that are not calls: the address taken (`&Name`, a callback table, a gadget or
timer callback, a `std::function`), where the signature is dictated by the pointer type and cannot
change; and macros that expand to a call.

Write down the evidence as you go — it goes in the summary and the commit message: "14 callers, all
pass only handle, view and frame".

### 3. Look at what the arguments were

Deleting an argument deletes its evaluation. `Explosion_Damage(coord, Random_Pick(0, 3))` draws from
the synchronized random stream; drop the call and every later random number in the match shifts,
which desyncs multiplayer and changes replays. The same goes for `i++`, an allocation, or anything
that logs. When the argument expression has a side effect, keep the effect as its own statement and
say so — or stop and ask if it is unclear whether the effect was ever wanted.

After the edit, check each caller for a local that was computed only to be passed. It is dead now
too, and `build/` compiles with `-w`, so nothing will point it out.

### 4. Remove it, and follow the consequences inward

- **Ignored by the body** (unnamed, `/*name*/`, or simply never read): delete it from the
  declarations and every call.
- **Always the same value** (every caller passes `1`, or nobody overrides the default): replace the
  parameter with that value inside the body, then simplify with the rules under "Dead conditional
  branches" — the `if (flag)` keeps its body, the `else` goes. Do the substitution on paper first:
  `if (!clip || x < width)` with `clip == true` reduces to `x < width`, not to nothing.
- **A function nothing calls**: confirm as in step 2 (address-taken uses matter most here), then
  delete the declaration, the definition and its documentation. A virtual with no callers may still
  be called through the base; an entry in a save-game or network dispatch table is a caller.

Then look at what the removal orphaned, and take that too: an enum or flag type that existed only
for this parameter (`WSAType` went with `Animate_Frame`'s flags), a helper only the dead branch
called, a constant, an `#include`. Each of these is its own claim — give it the same grep before
deleting. Stop at anything serialized: a data member that is written by a `Serialize()` is part of
the save format even if nothing reads it.

Update the words as well: the declaration comment's description of the parameter, the mention of it
in other comments, and `/*name=*/` argument comments at call sites that now label the wrong
position.

### 5. Verify

```bash
cmake --build build --parallel 22          # both games; a shared signature has callers in each
ctest --test-dir build --output-on-failure
grep -rn '\bName *(' src/ | wc -l          # same count as before, unless calls were meant to go
```

`build/` is `STRICT_CHECKS=OFF`, so it proves the GCC build only; the strict build is left to
`/commit`. Callers under `#ifdef`s that this machine cannot compile were edited blind — name them in
the summary.

## Output

After making changes, provide a summary:

- Number and type of dead code blocks removed (e.g., "2 commented-out code blocks, 1 `#if 0`
  section")
- Brief description of what each removed block contained
- Any items you skipped and why (e.g., "kept commented example in doc comment")
- In a file sweep: candidates for tree-wide removal that you noticed but did not touch (ignored
  parameters, functions that look uncalled)

For a parameter or function removal, report instead:

- The claim and the evidence: how many declarations and callers, what each passed, where you looked
- If the claim was false: the counterexample (`file:line`), and nothing changed
- What went: the parameter, the branches it controlled, and anything orphaned by it
- Side effects preserved from removed arguments, callers edited without being compiled, and whether
  the twin in the other game has the same dead parameter
