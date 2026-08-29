---
allowed-tools: Read, Grep, Glob, Edit, Bash
description: Improve code comments to be readable, concise, and focused on "why"
---

Review and improve comments in the specified file(s): $ARGUMENTS

## Philosophy

Comments should explain **why** code exists, not **what** it does. The code itself shows what happens; comments should
provide context that isn't obvious from reading the code.

**Preservation bias:** When in doubt, keep the comment. This is legacy 1990s game code where original developer comments
often carry valuable context, domain knowledge, or historical flavor that cannot be reconstructed. A comment that seems
redundant may be the only record of a design decision. Only remove comments that are truly noise — prefer rewriting over
removing.

## Coverage requirement (read this first)

The unit of work is **every comment in the file**, not every function. Comments inside function bodies are the easiest
to skip and are usually where the real knowledge is buried — a bare `0x80000000`, a loop that looks infinite, a
condition whose purpose is invisible. Docs on declarations are the *visible* half of the job, not the whole job.

Before editing, enumerate what you are responsible for:

```bash
# Every comment line, with line numbers. Both files if .h/.cc pair.
grep -n -E '^\s*(//|/\*|\*)' <file> | wc -l
# Function bodies are everything indented; scan those specifically.
grep -n -E '^\s+(//|/\*)' <file>
```

Work through that list in file order. Do not jump straight to the function banners. At the end, re-run the enumeration
and confirm you have a decision (keep / rewrite / remove / add-nearby) for each in-body comment — not just the
declaration-level ones.

**Also scan for code that has *no* comment and needs one.** Grep the body for the usual suspects:

```bash
grep -n -E '0x[0-9A-Fa-f]{4,}|== *-?[0-9]{3,}|\[[0-9]{2,}\]|magic|HACK|XXX|kludge' <file>
```

**Convert every `/* ... */` block to `//`.** This is part of the pass, not an optional tidy-up, and it is easy to skip
because the legacy blocks read as "already commented". The original code is full of

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

- The **license header** at the top of the file stays a `/* */` block — it is the repo-wide legal notice and every
  other file matches it.
- **Inline** comments that cannot be `//` because code follows on the same line: unused-parameter names
  (`int /*timeout*/`) and commented-out sub-expressions (`/*a && */ b`). Leave these as they are.

When converting, drop the leading `**` from each line and re-wrap the prose. Strip the bare `//` delimiter lines that
the old `//` box style leaves at the start and end of a run — a comment should not open or close with an empty line.

## Process

1. **Read the entire file** to understand its purpose and how the pieces fit together
2. **Trace the control flow** to understand what each function accomplishes
3. **Enumerate every comment** (see Coverage requirement) so none are silently skipped
4. **Identify the non-obvious parts**: Why was this approach chosen? What invariants must hold? What edge cases does
   this handle?

## For each comment, decide:

### Move to `.h` if:

- A `.cc` file has a "what" comment on a function definition that belongs on the declaration in the `.h` file
- The `.h` declaration lacks documentation but the `.cc` definition has it — move the comment, don't delete it
- This preserves documentation that would otherwise be lost when cleaning up `.cc` comments

### Remove only if:

- It's a pure tautology that adds zero information (`i++; // increment i`, `return true; // return true`)
- It's factually wrong and no longer matches the code (but first check: might the comment reveal a bug?)
- It's commented-out code with no explanatory note

Do **not** remove comments just because the "what" seems obvious from code. Comments that describe what a block does
in domain terms (e.g., "Scan for adjacent enemy units" above a loop) help readers skim the code and preserve the
original developer's intent. Rewrite these to be clearer rather than deleting them.

### Rewrite if:

- It describes "what" but could also explain "why" — add the "why", but keep the "what" if it provides useful context
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

- **Section markers inside long functions** — `/* ** Now do the thing */` blocks that split a 200-line function into
  phases. Keep them; they are the only structure the function has. Rewrite them to say why the phase exists or what
  invariant holds when it ends, not merely what the next ten lines do.
- **Bare literals and sentinels** — `0x80000000`, `255`, `-1`, `0xFF`, a bare `24` or `60`. Say what the value *means*
  and, when the same meaning appears in more than one place, say so. An unnamed sentinel with no comment is the single
  most common defect source here.
- **Loops whose bounds or termination are not obvious** — a `while (Count())` that terminates because the body removes
  an element, a scan that mutates its own limit variable.
- **Conditions guarding something invisible** — `if (Session.Type != GAME_NORMAL)`, platform and `#ifdef` branches,
  early returns. Say what breaks without the guard.
- **Blocks under `#if (0)` / `#if (1)` with an alternative** — keep both the code and the note explaining the choice.
- **Ordering that is load-bearing** — "this must run before X because Y". If you discover it while tracing, write it
  down; it is invisible to the next reader.

Follow the same keep / rewrite / remove rules above. Preservation bias applies here too — an awkward in-body comment
gets rewritten, not deleted.

## Report anything that looks like a bug

Tracing control flow closely is how comment passes find real defects. When a comment contradicts the code, when a
sentinel is compared against the wrong value or type, or when a condition is provably always true or false, **say so in
the summary** rather than quietly rewording the comment to match the code. Leave a `// TODO:` at the site describing
the defect. Do not fix the code as part of a comment pass unless asked — but never let the finding disappear.

## Style Guidelines (Google C++ Style)

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Comments) for comments:

### General

- Use `//` style comments, even for multi-line (each line starts with `//`). This applies to comments you *rewrite*
  as well as ones you add: converting the legacy `/* ** */` blocks is required, see the Coverage requirement above
- Write in complete sentences with proper punctuation
- Be concise but not cryptic
- Don't use comments to disable code; delete dead code instead

### File Comments

- Every file should have a comment at the top describing its contents (after the license header)
- Keep it brief: what abstractions does this file contain?

### Class Comments

- Every non-obvious class declaration should have a comment describing what it's for and how to use it
- Document thread-safety, ownership semantics, and lifetime requirements

### Function Comments

- Document functions at their **declaration** (in the header), not the definition
- Describe what the function does, its inputs, outputs, and any side effects
- Don't repeat in the `.cpp` what's already documented in the `.h`
- Implementation comments in `.cpp` should explain **how**, not restate **what**
- **IMPORTANT:** If a `.cc` file has a useful comment on a function definition and the `.h` declaration has none, **move
  the comment to the `.h` file** rather than deleting it. Never lose documentation by simply removing it.

### Variable Comments

- Document non-obvious member variables, especially sentinel values (`-1 means unlimited`)
- Class data members should have a comment describing what they're used for
- **Local** variables get the same treatment when they hold a sentinel or an unobvious unit (ticks vs. frames,
  leptons vs. pixels, cells vs. coordinates)

### Implementation Comments

- Explain tricky or non-obvious code
- Use `// TODO(username): description` for future work
- Place comments on the line above the code, not at the end of the line (except for very brief clarifications)

## Verify before reporting

- Re-run the enumeration from the Coverage requirement and confirm the in-body comments were actually considered
- Confirm `grep -c -E '^\s*/\*' <file>` is down to the license header plus any genuinely inline comments
- Build the affected target so a mangled comment cannot hide an unterminated block comment
- If the file was reformatted, confirm the diff contains no code changes:
  `git diff -U0 <file> | grep '^+' | grep -vE '^(\+\+\+|\+\s*(//|\*|/\*))'`

## Output

After making changes, provide a brief summary of:

- Comments removed (with reasoning)
- Comments rewritten (before/after comparison for significant changes)
- Comments added (explaining why they're valuable)
- **In-body coverage**: how many in-body comments you reviewed, and which function bodies gained comments
- **Block conversion**: how many `/* */` blocks became `//`, and what remains
- Any suspected bugs found while tracing (see "Report anything that looks like a bug")
