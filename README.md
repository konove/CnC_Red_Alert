# Command & Conquer / Red Alert SDL

This repository is a fork of https://github.com/electronicarts/CnC_Red_Alert, partially merged with
https://github.com/electronicarts/CnC_Tiberian_Dawn. It builds more portable, somewhat functional
versions of both games.

## Dependencies

All this fork needs to compile is:

- A C++ compiler
- CMake
- SDL2
- Ninja (optional but recommended)

## Compiling (Everywhere)

1. `cmake -Bbuild -G Ninja`
2. `JOBS=$(($(getconf _NPROCESSORS_ONLN) / 2)) && cmake --build build --parallel $JOBS`
3. Find some data files
4. Run `tdsdl` or `rasdl`
5. (optional) Command and/or conquer

## Code Quality Tools

This project uses [Include-What-You-Use (IWYU)](https://include-what-you-use.org/) to analyze C++
headers. Install with `sudo apt install iwyu` (Linux) or `brew install include-what-you-use`
(macOS). See [docs/IWYU.md](docs/IWYU.md) for details.

To disable: `cmake -Bbuild -DENABLE_IWYU=OFF`

### Markdown formatting

Use [Prettier](https://prettier.io/docs/) for Markdown. The shared `.prettierrc.json` wraps prose at
100 columns and formats lists and tables, leaving fenced code unchanged. Format Markdown files you
edit before finishing a change.

In VS Code, install the recommended **Prettier - Code formatter** extension; Markdown then formats
automatically on save. Other editors can use the same config with their Prettier integration.

From the repository root, with Node.js/npm installed:

```bash
npx prettier@3.6.2 --write README.md
npx prettier@3.6.2 --check README.md
```

Replace `README.md` with the files you edited, or use `"**/*.md"` for all Markdown.
`.prettierignore` excludes build output, third-party files, and original license/README files.

#### Check before committing

Enable the Git hook once per clone:

```bash
npm install --global prettier@3.6.2
git config core.hooksPath .githooks
```

The hook blocks commits when staged Markdown fails Prettier's check. It checks the staged contents,
so partial staging is supported, and does not modify files or the index. Format the reported files,
stage the fixes, and retry. Prettier must be on the committing process's `PATH`, including when
committing from CLion; keep CLion's **Run Git hooks** option enabled.

Git hooks are local and can be bypassed with `--no-verify`. For team-wide enforcement, also run
formatting checks in CI and require that check before merging.

## Status

Both games compile on Linux/macOS/Windows and run at least to the menus. I have lightly tested a few
campain missions, RA is more stable than TD.

There's some support for network multiplayer in RA. (I've successfully tested one game between
Linux/Windows.)

Code only used by later missions is likely still broken, or possibly missing entirely if it's part
of something I had to translate from assembly.

## Original README

[Over here](README-EA.md)
