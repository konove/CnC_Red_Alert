---
allowed-tools: Read, Edit, Write
description: Add a new entry to TODO.md, rephrased for clarity and placed in the appropriate section
---

Add a TODO item to TODO.md based on the user's input: $ARGUMENTS

Steps:

1. Read TODO.md to understand the current structure and existing sections
2. Analyze the user's input and:
    - Rephrase it to be concise and readable (use imperative mood, e.g., "Fix X" not "X should be fixed")
    - Add relevant details for completeness (context, affected files/areas if known)
    - Determine the appropriate section based on the type of work:
        - **Bugs**: Issues, crashes, incorrect behavior
        - **Features**: New functionality, enhancements
        - **Refactoring**: Code cleanup, modernization, technical debt
        - **Build**: CMake, dependencies, CI/CD
        - **Documentation**: Comments, README, guides
        - **Testing**: Unit tests, test infrastructure
        - **Other**: Anything that doesn't fit above
3. If the appropriate section doesn't exist in TODO.md, create it with a `## Section` header
4. Add the new entry as a bullet point (`- `) under the appropriate section
5. Write the updated TODO.md

Format for entries:

```
- Brief imperative description
  - Additional context or details (optional, indented)
```

Keep the file organized: sections in logical order, entries within sections grouped by relevance.