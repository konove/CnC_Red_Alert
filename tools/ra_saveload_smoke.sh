#!/usr/bin/env bash
# Headless save/load check for Red Alert.
#
# Starts a scenario, saves at frame SAVE_AT, keeps running to END_AT, then
# loads that save and runs to END_AT again. Vehicles and vessels must be at
# the same coordinates in both runs; any difference means the save lost
# state. Aircraft, and the infantry that react to them, are logged but not
# compared: two fresh runs already disagree about them.
#
# With --load-fixture it instead loads src/ra/testdata/SAVEGAME.SCG01EA, a save
# written by an older binary, and checks that it loads and runs to END_AT:
# the proof that saves already on players' disks still decode.
#
# Usage: tools/ra_saveload_smoke.sh [--load-fixture] [rasdl] [scenario]
#   rasdl     path to the binary (default: cmake-build-strict-ra-clang/src/ra/rasdl)
#   scenario  scenario name without .INI (default: SCG01EA)
# Set RA_CD to the game data directory if it is not the Steam default.

set -euo pipefail

LOAD_FIXTURE=0
if [ "${1:-}" = "--load-fixture" ]; then
  LOAD_FIXTURE=1
  shift
fi
FIXTURE=$(realpath "$(dirname "$0")/../src/ra/testdata/SAVEGAME.SCG01EA")

RASDL=${1:-cmake-build-strict-ra-clang/src/ra/rasdl}
SCENARIO=${2:-SCG01EA}
RA_CD=${RA_CD:-"$HOME/.local/share/Steam/steamapps/common/Command & Conquer Red Alert"}
SAVE_AT=60
END_AT=120
SLOT=99

RASDL=$(realpath "$RASDL")
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
cd "$(dirname "$RASDL")"

run() {
  timeout 300 env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$RASDL" -NOMOVIES "$@" -CD"$RA_CD" 2>&1 | grep -o 'frame .*' || true
}

if [ "$LOAD_FIXTURE" = 1 ]; then
  cp "$FIXTURE" "SAVEGAME.0$SLOT"
  run "-LOADGAME$SLOT" "-QUITFRAME$END_AT" > "$WORK/fixture.log"
  rm -f "SAVEGAME.0$SLOT"
  positions=$(awk -v at="$END_AT" '$2 == at && ($3 == "unit" || $3 == "vessel")' "$WORK/fixture.log" | wc -l)
  if [ "$positions" -eq 0 ]; then
    echo "FAIL: fixture save did not load and run to frame $END_AT"
    exit 1
  fi
  echo "OK: fixture save loaded and ran to frame $END_AT ($positions object positions)"
  exit 0
fi

rm -f "SAVEGAME.0$SLOT"
run "-NEWGAME$SCENARIO" "-QUITFRAME$SAVE_AT" "-SAVESLOT$SLOT" > "$WORK/first.log"
test -s "SAVEGAME.0$SLOT" || { echo "no save written"; exit 1; }
run "-NEWGAME$SCENARIO" "-QUITFRAME$END_AT" > "$WORK/continuous.log"
run "-LOADGAME$SLOT" "-QUITFRAME$END_AT" > "$WORK/loaded.log"
rm -f "SAVEGAME.0$SLOT"

filter() { awk -v from="$SAVE_AT" '$2 > from && ($3 == "unit" || $3 == "vessel")' "$1"; }
filter "$WORK/continuous.log" > "$WORK/a"
filter "$WORK/loaded.log" > "$WORK/b"

if [ ! -s "$WORK/b" ]; then
  echo "FAIL: no object positions logged after load"
  exit 1
fi
if diff -q "$WORK/a" "$WORK/b" > /dev/null; then
  echo "OK: $(wc -l < "$WORK/b") object positions identical across save/load"
else
  echo "FAIL: object positions diverge after load"
  diff "$WORK/a" "$WORK/b" | head -20
  exit 1
fi
