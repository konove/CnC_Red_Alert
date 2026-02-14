---
allowed-tools: Read, Grep, Glob, Edit
description: Remove dead code (commented-out code, #if 0 blocks, unreachable statements)
---

Remove dead code from the specified file(s): $ARGUMENTS

## What Counts as Dead Code

### Commented-out code
Lines that are clearly disabled **code**, not documentation or explanatory comments. Look for:
- Commented-out function calls, variable declarations, assignments, control flow
- Blocks of code with `//` or `/* */` that contain recognizable C++ syntax (semicolons, braces, operators)
- Do **NOT** remove: documentation comments, TODOs, explanatory notes, license headers, section dividers

### `#if 0` / `#if false` blocks
Preprocessor-disabled code sections. Remove the entire block including the `#if 0`, `#else` (if present), and `#endif`.
If there is an `#else` branch with live code, keep that code and remove only the dead `#if 0` branch and its
preprocessor directives.

### Unreachable code
Statements that can never execute:
- Code after an unconditional `return`, `break`, `continue`, or `goto` within the same block
- Do **NOT** remove code after a conditional return (e.g., `if (x) return;` — the code after is reachable)
- Be careful with switch/case: `break` ends a case, but code in the next `case:` label is reachable

### Dead conditional branches
- `if (false) { ... }` or `if (0) { ... }` — remove the entire if block
- `while (false) { ... }` or `while (0) { ... }` — remove the entire while block
- `if (true) { ... } else { ... }` — keep the if-body, remove the else branch and the condition
- For `if (true)`, unwrap the body (remove the `if` and braces, keep the contents) only if it doesn't
  introduce scoping issues

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
   - Code that is merely unused but still reachable (unused variables, uncalled functions) — that requires
     broader analysis beyond a single file
   - Preprocessor guards for platform portability (`#ifdef __linux__`, `#ifdef __APPLE__`, etc.) unless
     the user explicitly asks
   - Comments that document behavior, even if they reference removed code

## Output

After making changes, provide a summary:
- Number and type of dead code blocks removed (e.g., "2 commented-out code blocks, 1 `#if 0` section")
- Brief description of what each removed block contained
- Any items you skipped and why (e.g., "kept commented example in doc comment")
