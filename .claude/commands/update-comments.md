---
allowed-tools: Read, Grep, Glob, Edit, Write, Bash
description: 'Improve a file''s comments to be readable, concise and focused on "why", report the bugs found while tracing the code, and fix those bugs when asked. Use for "update/rework/clean up the comments in X", and also for the follow-up "fix the bugs it found" / "now fix those" after a comment pass, or when the request already says "update the comments and fix what you find".'
---

Review and improve comments in the specified file(s): $ARGUMENTS

## Philosophy

Comments should explain **why** code exists, not **what** it does. The code itself shows what
happens; comments should provide context that isn't obvious from reading the code.

**Preservation bias:** When in doubt, keep the comment. This is legacy 1990s game code where
original developer comments often carry valuable context, domain knowledge, or historical flavor
that cannot be reconstructed. A comment that seems redundant may be the only record of a design
decision. Only remove comments that are truly noise — prefer rewriting over removing.

## Coverage requirement (read this first)

The unit of work is **every comment in the file**, not every function. Comments inside function
bodies are the easiest to skip and are usually where the real knowledge is buried — a bare
`0x80000000`, a loop that looks infinite, a condition whose purpose is invisible. Docs on
declarations are the _visible_ half of the job, not the whole job.

Before editing, enumerate what you are responsible for:

```bash
# Every comment line, with line numbers. Both files if .h/.cc pair.
grep -n -E '^\s*(//|/\*|\*)' <file> | wc -l
# Function bodies are everything indented; scan those specifically.
grep -n -E '^\s+(//|/\*)' <file>
```

Work through that list in file order. Do not jump straight to the function banners. At the end,
re-run the enumeration and confirm you have a decision (keep / rewrite / remove / add-nearby) for
each in-body comment — not just the declaration-level ones.

**Also scan for code that has _no_ comment and needs one.** Grep the body for the usual suspects:

```bash
grep -n -E '0x[0-9A-Fa-f]{4,}|== *-?[0-9]{3,}|\[[0-9]{2,}\]|magic|HACK|XXX|kludge' <file>
```

**Convert every `/* ... */` block to `//`.** This is part of the pass, not an optional tidy-up, and
it is easy to skip because the legacy blocks read as "already commented". The original code is full
of

```c
/*
**	Some explanation.
*/
```

which must become

```c
// Some explanation.
```

Count them before and after so the conversion is verifiable:

```bash
grep -c -E '^\s*/\*' <file>     # block-comment openers; should reach 1 (the license header)
```

Two exceptions, both narrow:

- The **license header** at the top of the file stays a `/* */` block — it is the repo-wide legal
  notice and every other file matches it.
- **Inline** comments that cannot be `//` because code follows on the same line: unused-parameter
  names (`int /*timeout*/`) and commented-out sub-expressions (`/*a && */ b`). Leave these as they
  are.

When converting, drop the leading `**` from each line and re-wrap the prose. Strip the bare `//`
delimiter lines that the old `//` box style leaves at the start and end of a run — a comment should
not open or close with an empty line.

## Process

1. **Read the entire file** to understand its purpose and how the pieces fit together
2. **Trace the control flow** to understand what each function accomplishes
3. **Enumerate every comment** (see Coverage requirement) so none are silently skipped
4. **Identify the non-obvious parts**: Why was this approach chosen? What invariants must hold? What
   edge cases does this handle?

## For each comment, decide:

### Move to `.h` if:

- A `.cc` file has a "what" comment on a function definition that belongs on the declaration in the
  `.h` file
- The `.h` declaration lacks documentation but the `.cc` definition has it — move the comment, don't
  delete it
- This preserves documentation that would otherwise be lost when cleaning up `.cc` comments

### Remove only if:

- It's a pure tautology that adds zero information (`i++; // increment i`,
  `return true; // return true`)
- It's factually wrong and no longer matches the code (but first check: might the comment reveal a
  bug?)
- It's commented-out code with no explanatory note

Do **not** remove comments just because the "what" seems obvious from code. Comments that describe
what a block does in domain terms (e.g., "Scan for adjacent enemy units" above a loop) help readers
skim the code and preserve the original developer's intent. Rewrite these to be clearer rather than
deleting them.

### Rewrite if:

- It describes "what" but could also explain "why" — add the "why", but keep the "what" if it
  provides useful context
- It's verbose and can be made concise without losing meaning
- It uses unclear terminology or abbreviations that aren't game-domain terms
- It's grammatically awkward enough to impede understanding

### Add comments for:

- Non-obvious algorithms or formulas (explain the approach)
- Magic numbers that aren't self-documenting
- Workarounds for bugs or limitations (link to issues if possible)
- Invariants that must be maintained
- Performance-critical sections explaining optimization choices
- Integration points explaining how this code interacts with external systems

## In-body comments (do not skip this pass)

Walk each function body top to bottom. These are the recurring cases in this codebase:

- **Section markers inside long functions** — `/* ** Now do the thing */` blocks that split a
  200-line function into phases. Keep them; they are the only structure the function has. Rewrite
  them to say why the phase exists or what invariant holds when it ends, not merely what the next
  ten lines do.
- **Bare literals and sentinels** — `0x80000000`, `255`, `-1`, `0xFF`, a bare `24` or `60`. Say what
  the value _means_ and, when the same meaning appears in more than one place, say so. An unnamed
  sentinel with no comment is the single most common defect source here.
- **Loops whose bounds or termination are not obvious** — a `while (Count())` that terminates
  because the body removes an element, a scan that mutates its own limit variable.
- **Conditions guarding something invisible** — `if (Session.Type != GAME_NORMAL)`, platform and
  `#ifdef` branches, early returns. Say what breaks without the guard.
- **Blocks under `#if (0)` / `#if (1)` with an alternative** — keep both the code and the note
  explaining the choice.
- **Ordering that is load-bearing** — "this must run before X because Y". If you discover it while
  tracing, write it down; it is invisible to the next reader.

Follow the same keep / rewrite / remove rules above. Preservation bias applies here too — an awkward
in-body comment gets rewritten, not deleted.

## Report anything that looks like a bug

Tracing control flow closely is how comment passes find real defects. When a comment contradicts the
code, when a sentinel is compared against the wrong value or type, when a result is ignored that the
next line depends on, or when a condition is provably always true or false, **say so in the
summary** rather than quietly rewording the comment to match the code. Leave a `// TODO:` at the
site describing the defect, so the finding survives even if the summary scrolls away.

The user's next message after a comment pass is usually "fix the bugs it found", so write each
finding so that it can be acted on without re-tracing the code. Number them, and give each one:

- **Where**: `file:line` and the function.
- **What goes wrong**: the concrete path — which input or state, which line misbehaves, what the
  player or caller sees (crash, wrong frame, silent no-op). "Looks suspicious" is not a finding;
  trace it until you can say this, or say exactly what you could not determine.
- **Reachable?**: check the callers before claiming a bug. A null check that is wrong but whose
  callers never pass null is a latent defect, not a crash; say which it is, because it decides
  whether a fix is worth a test and a commit.
- **The other game**: `src/ra` and `src/td` carry near-copies of many files. Look at the twin and
  say whether it has the same defect.
- **Proposed fix**, in one or two sentences, and whether it changes game behaviour (see "Fixes that
  change the simulation" below).

Whether to fix in the same run depends on what was asked:

- Only a comment pass was asked for: report, leave the TODOs, change no code. A comment pass whose
  diff contains code changes cannot be reviewed as a comment pass.
- The request includes the fixes ("... and fix the bugs you find"), or the user follows up with "fix
  the bugs it found": finish the comment pass first, then continue with the next section.

## Fixing the bugs the pass found

Do this after the comment pass is complete and verified as comment-only (see "Verify before
reporting"), so the two kinds of change stay distinguishable. If this is a follow-up in the same
conversation, work from the numbered findings already reported; in a fresh conversation, recover
them with `grep -n 'TODO' <file>` and `git log -1 -p -- <file>`.

Fix every finding unless the user picked some. For each one:

1. **Confirm it against the callers.** Re-read the path with the fix in mind. A finding that turns
   out to be wrong is a good outcome: delete the TODO, put a comment there explaining why the code
   is right after all, and say so in the summary. Do not "fix" code you could not show to be broken.
2. **Write the failing test first** when the code can be reached from a unit test (`add_gtest` in
   the directory's `CMakeLists.txt`; `src/sdllib/wsa_test.cc` grew its playback tests this way,
   commit `9405c688`). Run it against the unfixed code and watch it fail or crash — that is the
   proof the bug was real, and it goes in the commit message ("against the previous code the
   resident-write test crashes"). Code tangled into the game globals often cannot be unit-tested;
   then say so, and describe how the fix was checked instead (the headless save/load smoke scripts
   in `tools/`, or reasoning from the callers).
3. **Make the smallest fix that removes the defect.** No drive-by refactoring in the same lines; the
   diff should let a reader see exactly what the bug was. If the fix makes a nearby check or
   assignment redundant, removing it is part of the fix — mention it.
4. **Bring the comments along.** Remove the `// TODO:`, and rewrite any comment (including the `.h`
   declaration you may have just written to match the _old_ behaviour) so it describes what the code
   does now. A comment pass that documented a bug faithfully leaves a wrong comment behind the
   moment the bug is fixed.
5. **Apply it to the twin** in the other game when the finding said it has the same defect. Same
   fix, separate commit later.

### Fixes that change the simulation

Most findings are in error paths, bounds and resource handling, and fixing them is invisible to a
working game. Some are not: a wrong comparison in targeting, threat evaluation, movement, production
or anything else that runs inside the game frame _is the shipped behaviour_. Fixing it changes how
missions play, desyncs a multiplayer session against an unfixed peer, and can diverge the save/load
smoke tests. Do not fix these silently. Fix the crashes and out-of-bounds cases, and for a pure
logic change list it, explain what would play differently, and let the user decide.

### Verify the fixes

- Build both games when a shared directory (`sdllib`, `tech`, `port`, `base`, `winvq`) was touched:
  `cmake --build build --parallel 22`
- Run the tests: `ctest --test-dir build --output-on-failure` (or `-R <name>` for the new one while
  iterating).
- `build/` compiles with `-w`, so a clean build there says nothing about warnings or clang-tidy. New
  test code is what trips the strict checks most often; leave the strict build to `/commit`, but say
  in the summary that it has not been run.
- `grep -n 'TODO' <file>`: every TODO this pass left is either gone or belongs to a finding you
  deliberately did not fix.

Do not commit unless asked. When the user does ask, `/commit` has the rule: the fix is its own
commit when it touches different lines from the comment pass, and shares one ("Rework X comments and
fix Y", with a "Fixes found while tracing:" paragraph per bug) when the two are interleaved.

## Style Guidelines (Google C++ Style)

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Comments) for
comments:

### General

- Use `//` style comments, even for multi-line (each line starts with `//`). This applies to
  comments you _rewrite_ as well as ones you add: converting the legacy `/* ** */` blocks is
  required, see the Coverage requirement above
- Write in complete sentences with proper punctuation
- Be concise but not cryptic
- Don't use comments to disable code; delete dead code instead

### File Comments

- Every file should have a comment at the top describing its contents (after the license header)
- Keep it brief: what abstractions does this file contain?

### Class Comments

- Every non-obvious class declaration should have a comment describing what it's for and how to use
  it
- Document thread-safety, ownership semantics, and lifetime requirements

### Function Comments

- Document functions at their **declaration** (in the header), not the definition
- Describe what the function does, its inputs, outputs, and any side effects
- Don't repeat in the `.cpp` what's already documented in the `.h`
- Implementation comments in `.cpp` should explain **how**, not restate **what**
- **IMPORTANT:** If a `.cc` file has a useful comment on a function definition and the `.h`
  declaration has none, **move the comment to the `.h` file** rather than deleting it. Never lose
  documentation by simply removing it.

### Variable Comments

- Document non-obvious member variables, especially sentinel values (`-1 means unlimited`)
- Class data members should have a comment describing what they're used for
- **Local** variables get the same treatment when they hold a sentinel or an unobvious unit (ticks
  vs. frames, leptons vs. pixels, cells vs. coordinates)

### Implementation Comments

- Explain tricky or non-obvious code
- Use `// TODO(username): description` for future work
- Place comments on the line above the code, not at the end of the line (except for very brief
  clarifications)

## Verify before reporting

- Re-run the enumeration from the Coverage requirement and confirm the in-body comments were
  actually considered
- Confirm `grep -c -E '^\s*/\*' <file>` is down to the license header plus any genuinely inline
  comments
- Build the affected target so a mangled comment cannot hide an unterminated block comment
- Confirm the diff of the comment pass contains no code changes. Do this _before_ starting on any
  bug fixes, since afterwards the check can no longer tell the two apart:
  `git diff -U0 <file> | grep '^+' | grep -vE '^(\+\+\+|\+\s*(//|\*|/\*))'`

## Output

After making changes, provide a brief summary of:

- Comments removed (with reasoning)
- Comments rewritten (before/after comparison for significant changes)
- Comments added (explaining why they're valuable)
- **In-body coverage**: how many in-body comments you reviewed, and which function bodies gained
  comments
- **Block conversion**: how many `/* */` blocks became `//`, and what remains
- **Suspected bugs**, numbered, in the form given under "Report anything that looks like a bug". If
  there are none, say "no defects found while tracing" so the absence is a statement rather than an
  omission
- If fixes were made: per finding, what was wrong, the fix, the test that covers it (or why there is
  none), and whether the twin in the other game was fixed too. List separately any finding you did
  not fix — disproved, or left for the user because it changes the simulation — and what is still
  unverified (normally the strict build)
