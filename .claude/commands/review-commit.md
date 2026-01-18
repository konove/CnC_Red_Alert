---
allowed-tools: Bash(git diff:*), Bash(git status:*), Bash(git log:*), Read, Grep, Glob
description: Review pending local changes for bugs, style issues, and CLAUDE.md compliance
---

Review the pending changes on the current branch.

Steps:

1. Run `git status` to see what files have changed
2. Run `git diff` to see the actual changes (both staged and unstaged)
3. Read CLAUDE.md to understand project style guidelines
4. For each changed file, review the diff for:
    - Bugs and logic errors
    - Memory safety issues (buffer overflows, leaks)
    - Style violations per CLAUDE.md and Google C++ Style Guide
    - Missing error handling
    - Security issues
5. Provide a concise summary of issues found, grouped by severity:
    - **Critical**: Bugs that will cause crashes or incorrect behavior
    - **Important**: Style violations, potential issues
    - **Minor**: Nitpicks, suggestions

Keep feedback actionable and specific. Reference file:line for each issue.