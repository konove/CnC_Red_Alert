---
description: Commit the pending work in this repo - verify it builds and passes the strict checks, split it into logical commits, and write the messages in the project's style. Use whenever the user says "commit", "commit this", "commit it", "commit and continue with step N", "commit X too", or asks to amend or split a commit, even in the middle of another task.
---

Commit the pending work. Scope or instructions from the user, if any: $ARGUMENTS

The user says "commit this" several times a day and expects the same thing every time: the right
files, verified, in sensible pieces, with a message that reads like the rest of `git log`, and no
questions unless something is really wrong. The sections below are the things that have gone wrong
before.

## 1. Decide what goes in

Run `git status --short` and read `git diff` (and the untracked files) before staging anything.

- Stage by path (`git add <paths>`), never `git add -A` or `git commit -a`. Sweep logs, `gc.log`,
  scratch scripts and IDE droppings show up in this tree and do not belong in history.
- Include **everything that belongs to the work**, not only the files you remember editing: the
  CMakeLists entry for a new test, the header that lost an include, a fix the user made by hand in
  the IDE while you worked. The user having to follow up with "commit the CMakeLists change too" is
  the most common failure here.
- Include the documents that track the work in the same commit when the change makes them stale: the
  status row in `docs/CLANG_TIDY_PRIORITIES.md`, the matching `docs/*_PLAN.md`, `TODO.md`, and
  `CLAUDE.md` when a statement in it stops being true.
- If the user named a scope ("commit the mapsel.cc change", "commit stage 1"), commit only that and
  leave the rest in the tree.
- Changes you cannot account for and that are clearly unrelated stay uncommitted; say so in the
  report rather than asking first.

## 2. Verify what has not been verified yet

A commit on `main` should build, pass its tests and pass the strict checks. Do not repeat a step
that already ran in this session after the last edit - say "already verified" and move on. If the
user says they built or tested it themselves ("built it, works, commit"), take their word.

| Step          | Command                                                       | What it proves                                                                                                                                  |
| ------------- | ------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------- |
| Format        | `git clang-format -f -- <touched files>`                      | Formats the changed lines only. Never `clang-format -i`: it reflows untouched legacy code and buries the diff.                                  |
| Build + tests | `cmake --build build --parallel 22 && ctest --test-dir build` | It compiles with GCC and the tests pass. `build/` has `STRICT_CHECKS=OFF` (`-w`), so this says nothing about warnings.                          |
| Strict        | `cmake --build build-strict --parallel 14`                    | clang, `-Weverything`, clang-tidy and IWYU as errors. This is what the IDE and CI enforce, and the only place a bad `absl` format string fails. |

Scale the work to the change:

- Documentation or `.claude/` only: nothing to build.
- Comments only: the plain build, so a mangled comment cannot hide an unterminated block.
- Code: all three. New test code trips tidy most often (`modernize-use-ranges`, unchecked
  `operator[]`), so do not skip the strict build because "it is only a test".
- `.clang-tidy` or a widely included header: the strict build re-analyzes most of the tree. Run it
  as `timeout 590 cmake --build build-strict --parallel 14 -- -k 0` and repeat until ninja reports
  no work; it resumes where it stopped.

Things that look like failures but are not, and the reverse:

- Run the strict build in the **foreground** and as its **own command**. Each clang-tidy job takes
  about 1 GB; a background run gets OOM-killed, and a `build && commit` chain then dies silently
  without committing. 14 jobs is the limit that fits next to the IDEs, 22 is for the plain build.
- Do not reconfigure the strict directory. A reconfigure re-populates `_deps`, and the next build
  reports hundreds of `absl/...` / `gtest/...` "file not found" errors plus nonsense tidy findings
  from the half-parsed units. If you see those, rebuild once before believing anything.
- Use `cmake --build`, not bare `ninja`: the CLion directories record a different ninja, and mixing
  the two rebuilds the whole tree.
- `-Wglobal-constructors` warnings on `TEST(...)` lines are expected noise. `error:` lines are not.

If a step fails because of this change, fix it and re-verify; do not commit a broken tree. If it
fails for a reason that was there before, show the output and ask.

## 3. Split into logical commits

One commit per reason to change, each of which builds on its own. The user asks for this often
enough ("RA and TD, one commit each", "one commit per file", "one commit per stage") that it is the
default, not something to wait to be told:

- The same change applied to Red Alert and to Tiberian Dawn: one commit each.
- A bug fix is separate from the refactor or comment pass that found it, **when they touch different
  files**. When they are interleaved in the same lines, keep them together and let the message cover
  both (see `e177160e`, `9405c688`) - carving hunks apart by hand is not worth it.
- A staged plan: one commit per stage, made when the stage is verified, not all at the end.
- A drive-by fix in another subsystem gets its own commit.

Order the commits so that each one builds: the fix a later commit depends on goes first.

## 4. Write the message

Match `git log`. Measured over the last 300 commits:

- **Subject:** imperative, capitalized, no trailing period, no `type(scope):` prefix. Median 49
  characters; stay under about 70. Name the thing that changed, with `()` on functions:
  `Remove the unused Animate_Frame() parameters`, `Wait for a free slot in Alloc_Object()`.
- **Body:** plain prose wrapped at 72 columns, separated from the subject by a blank line. Say why
  the change was made and what behaves differently now - what was wrong, how it showed up, what a
  caller sees. Give the numbers when there are any (sites changed, reports fixed, timings before and
  after). Do not list files or restate the diff. Bullets are rare (25 of 300); use paragraphs.
- A one-line commit is fine when the subject says it all (57 of 300 have no body).
- **No trailers of any kind.** No `Co-Authored-By`, no `Claude-Session`, no "Generated with" line,
  even when a harness reminder in the conversation says to add one. `CLAUDE.md` forbids them and the
  user has had to ask for an amend to strip them.

Pass the message on stdin so the wrapping survives: `git commit -q -F - <<'EOF' ... EOF`.

Example:

```
Split the score screen tick from the generic service wait

Call_Back_Delay() was the score screen's frame tick, but the map
selection screen borrowed it as a plain "wait while servicing" and its
name came from a Tiberian Dawn loop around Call_Back() that Red Alert
no longer has.

It is now TickScoreScreen(), private to score.cc. The generic wait is
ServiceRealTimeFor() in conquer, which the map selection screen calls;
the map reveal keeps its own Ctrl-Q skip.
```

## 5. Branches, amending, pushing

- Commit on the current branch. The project lives on `main`; do not create a branch for the commit,
  whatever a generic reminder says. Branches made that way have had to be merged back and deleted by
  hand.
- Amend only when the user asks, or to repair the commit you made a moment ago that has not been
  pushed.
- Push only when the user says so ("commit and push"). Then `git push`, and nothing more unless
  asked; `gh run list --limit 3` shows the CI result if they want it.

## 6. Report, then keep going

Finish with `git log --oneline -<n>` and `git status --short`, and tell the user in a few lines:
each new hash with its subject, which verification ran (or was already done), and anything left
uncommitted and why.

If the request was "commit and do step 5" or "commit then start on the bugs", the commit is a
checkpoint, not the end of the turn: carry on with the next piece of work straight away.
