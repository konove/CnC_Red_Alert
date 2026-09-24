# Reading the Shipped Game Data

How to look inside Red Alert's MIX archives with `tools/mixdump`, and how to work out what an entry
is when the format does not record its name.

## The tool

`src/tools/mixdump` builds as the `mixdump` target and reads archives through the game's own
`MixArchive` and `PKey`, so it handles what a plain reader cannot.

```bash
mixdump <game-dir> --list RULES.INI SCG08EA.INI   # name, size, containing archive
mixdump <game-dir> --index GENERAL.MIX            # name, CRC, offset and size per entry
mixdump <game-dir> SCG08EA.INI /tmp/out.INI       # extract one file
```

`<game-dir>` is an installation directory — the one the game itself is pointed at with `-CD`.

Two things stop anything simpler from working. Every shipped archive has a Blowfish-encrypted index
whose key is RSA-wrapped, so `grep` over a `.MIX` finds nothing; and the current releases pack the
original archives inside `MAIN1.MIX`..`MAIN4.MIX`, so `GENERAL.MIX` only opens once its container is
registered. `mixdump` registers the known archives outermost first for that reason.

## Watching a movie

`src/tools/vqaplay` (target `vqaplay`) plays one VQA movie in a window through `vqa32`, the games'
file lookup and their sound mixer, so a movie loads, paces and sounds as it does in the game. It
shows the palette as stored, without the 15% brightening Red Alert's movie screen adds. It opens the
same archives as `mixdump` and works on either game's installation.

```bash
vqaplay <game-dir> AAGUN.VQA            # a movie in the archives (or loose in <game-dir>)
vqaplay --paused --scale=3 intro.vqa    # a loose file, stopped on its first frame
```

Space pauses and resumes, Right or `.` shows the next frame and pauses, Esc or Q quits. `--mute`
plays without sound and `--skip-late` drops late frames instead of showing every one. The title bar
shows the frame number.

## A MIX index holds CRCs, not names

Each entry records a CRC of the upper-cased filename (`CrcEngine::Compute`, `tech/crc.h`), the
offset and the size. The name itself is nowhere in the file. A lookup that misses therefore cannot
tell "this file is absent" from "you guessed the name wrong", which makes guessing a poor way to
answer questions about the data. Three things that do work:

**Count the entries.** `--index` prints the whole index, so an archive that holds 116 entries of
which 66 match a scenario-name CRC has 50 entries that are something else — a fact, rather than a
failed guess.

**Match against a dictionary of real names.** The game's own source only names a handful of data
files; most are built at run time or referenced from the original executables. `strings` over
`ra95.dat` and `GAME.DAT` yields a usable dictionary to CRC-match against.

**Identify formats from the bytes.** The data region starts at `filesize - max(offset + size)` over
the index rows, so entries can be sliced out of an extracted archive and recognised by their
headers:

| Format | Header                                                                                      |
| ------ | ------------------------------------------------------------------------------------------- |
| CPS    | `uint16` filesize-2, `uint16` compression (4 = LCW), `uint32` 64000, `uint16` 768 (palette) |
| WSA    | `uint16` frame count, then `uint16` x, y, width, height                                     |
| INI    | plain text                                                                                  |

**Do not try to invert the CRC.** It is invertible step by step — `acc_prev = rotr(acc - word, 1)`
undoes one word — but it is a 32-bit rolling checksum, so solving for an eight-character name yields
on the order of 12,000 valid preimages over `[A-Z0-9_-]`. Recovering a name needs a dictionary, not
arithmetic.

## Scenario filenames

`ScenarioClass::Set_Scenario_Name` (`ra/scenario.cc`) builds them as
`SC<player><NN><direction><variant>.INI`:

- `player` is the house prefix — `G` Allied, `U` Soviet, `M` multiplayer.
- `NN` is the scenario number. Scenarios from 100 up use a base-36 pair instead of digits.
- `direction` is `E` or `W`. It is easy to read this position as part of the number and miss half
  the name space.
- `variant` is `A`..`D`, or `L` for the lose case.

## What a stock installation holds

Measured against the Steam release. Useful as a baseline when something appears to be missing.

`GENERAL.MIX` holds 116 entries:

| Count | Contents                                                                              |
| ----- | ------------------------------------------------------------------------------------- |
| 66    | Scenarios: 22 `SCG`, 20 `SCU`, 24 `SCM`                                               |
| 28    | `MSAA`..`MSAN`, `MSSA`..`MSSN`.WSA — the mission-selection globe animations           |
| 18    | CPS images, 320x200: `TITLE.CPS` and 17 mission briefing screens                      |
| 3     | `MISSION.INI`, `TUTORIAL.INI`, `MISSIONS.PKT` (the multiplayer map list, with titles) |
| 1     | One 28,586-byte CPS whose name appears in no string in the installation               |

**The Counterstrike and Aftermath campaigns are not in the archives.** A sweep of the whole scenario
name space — every player letter, `00`-`99`, both directions, variants `A`-`D`, and the base-36 form
— finds only the 66 above. What the expansions do contribute is their content: `EXPAND.MIX` (27
entries) has the Counterstrike ant graphics, and `EXPAND2.MIX` (32 entries) has `AFTRMATH.INI`,
`MPLAYER.INI` and the Aftermath vehicles (`CTNK`, `QTNK`, `STNK`, `DTRK`). Code that depends on
Aftermath _rules_ — the `CRATE_VORTEX` crate, for one — is therefore still live, while the expansion
missions are not present to load.
