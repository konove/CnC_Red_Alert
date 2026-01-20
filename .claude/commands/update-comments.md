---
allowed-tools: Read, Grep, Glob, Edit
description: Improve code comments to be readable, concise, and focused on "why"
---

Review and improve comments in the specified file(s): $ARGUMENTS

## Philosophy

Comments should explain **why** code exists, not **what** it does. The code itself shows what happens; comments should
provide context that isn't obvious from reading the code.

## Process

1. **Read the entire file** to understand its purpose and how the pieces fit together
2. **Trace the control flow** to understand what each function accomplishes
3. **Identify the non-obvious parts**: Why was this approach chosen? What invariants must hold? What edge cases does
   this handle?

## For each comment, decide:

### Remove if:

- It restates what the code obviously does (`i++; // increment i`)
- It describes the "what" that's clear from well-named functions/variables
- It's outdated or no longer matches the code
- It adds noise without insight

### Rewrite if:

- It describes "what" but should explain "why"
- It's verbose and can be made concise
- It uses unclear terminology or abbreviations
- It's grammatically awkward

### Add comments for:

- Non-obvious algorithms or formulas (explain the approach)
- Magic numbers that aren't self-documenting
- Workarounds for bugs or limitations (link to issues if possible)
- Invariants that must be maintained
- Performance-critical sections explaining optimization choices
- Integration points explaining how this code interacts with external systems

## Style Guidelines (Google C++ Style)

Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html#Comments) for comments:

### General

- Use `//` style comments, even for multi-line (each line starts with `//`)
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

### Variable Comments

- Document non-obvious member variables, especially sentinel values (`-1 means unlimited`)
- Class data members should have a comment describing what they're used for

### Implementation Comments

- Explain tricky or non-obvious code
- Use `// TODO(username): description` for future work
- Place comments on the line above the code, not at the end of the line (except for very brief clarifications)

## Output

After making changes, provide a brief summary of:

- Comments removed (with reasoning)
- Comments rewritten (before/after comparison for significant changes)
- Comments added (explaining why they're valuable)
