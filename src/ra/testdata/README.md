# Red Alert test data

## SAVEGAME.SCG01EA

A save of scenario SCG01EA at frame 60, written by `rasdl` built from commit 1bbefc04 (save version
16):

```
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./rasdl -NOMOVIES -NEWGAMESCG01EA -QUITFRAME60 -SAVESLOT98 -CD"<game data>"
```

`tools/ra_saveload_smoke.sh --load-fixture` loads it and runs to frame 120. It is the only check
that saves written before a change to the save streams (LZO blocks, Blowfish, the SHA-1 digest and
its position) still load after it.

The fixture is valid only while `kSaveGameVersion` in `src/ra/saveload.h` is unchanged: `Load_Game`
rightly rejects any other version. When the version is bumped, regenerate the file with the first
binary that writes the new version, and update the commit and version above.
