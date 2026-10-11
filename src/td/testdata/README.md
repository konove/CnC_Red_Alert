# Tiberian Dawn test data

## SAVEGAME.SCG01EA

A save of scenario SCG01EA at frame 60, written by `tdsdl` built from the commit that added it, at
save version 10:

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./tdsdl -NOMOVIES -SEED1 -NEWGAMESCG01EA -QUITFRAME60 -SAVESLOT98 -CD"<game data>"
```

`tools/td_saveload_smoke.sh --load-fixture` loads it and runs to frame 120; the
`td_save_fixture_test` ctest runs that, and is skipped where there is no game data. The script's
other modes save and load with the same binary, so this is the only check that a save written before
a change still loads after it.

The fixture is valid only while `kSaveGameVersion` in `src/td/saveload/saveload.h` is unchanged:
`Load_Game` rightly rejects any other version. When the version is bumped, regenerate the file with
the first binary that writes the new version, and update the version above. The script compares the
two versions first and says so when they differ.

## ui/

The scripts and screenshot hashes `tools/ui_check.py` plays; see `docs/TEST_HARNESS_PLAN.md`.
