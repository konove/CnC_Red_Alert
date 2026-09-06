#!/usr/bin/env bash
# Headless save/load check for Red Alert.
#
# Starts a scenario, saves at frame SAVE_AT, keeps running to END_AT, then
# loads that save and runs to END_AT again. Vehicles and vessels must be at
# the same coordinates in both runs; any difference means the save lost
# state. Aircraft, and the infantry that react to them, are logged but not
# compared: two fresh runs already disagree about them.
#
# Usage: tools/ra_saveload_smoke.sh [rasdl] [scenario]
#   rasdl     path to the binary (default: cmake-build-strict-ra-clang/src/ra/rasdl)
#   scenario  scenario name without .INI (default: SCG01EA)
# Set RA_CD to the game data directory if it is not the Steam default.

set -euo pipefail

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
