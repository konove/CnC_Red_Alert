#!/usr/bin/env bash
# Starts a TD campaign, saves at frame 60, and compares the next 60 frames
# with an uninterrupted run using the same fixed scenario seed. Set TD_CD to override the Steam data directory.
#
# With --load-fixture it instead loads src/td/testdata/SAVEGAME.SCG01EA, a save
# written by an older binary, and checks that it loads and runs to frame 120:
# the proof that saves already on players' disks still decode. The
# td_save_fixture_test ctest runs this mode.
#
# Usage: tools/td_saveload_smoke.sh [--load-fixture] [tdsdl] [scenario, default SCG01EA] [--factory|--team|--world|--building|--mobile|--map|--globals]
# Exits with 77 when there is no game data; ctest reports that as a skip.
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
LOAD_FIXTURE=0
if [ "${1:-}" = "--load-fixture" ]; then
  LOAD_FIXTURE=1
  shift
fi
TDSDL=$(realpath "${1:-cmake-build-strict-ra-clang/src/td/tdsdl}")
SCENARIO=${2:-SCG01EA}
FIXTURE=()
case ${3:-} in
  --factory) FIXTURE=(-FACTORYTEST);;
  --team) FIXTURE=(-TEAMTEST);;
  --world) FIXTURE=(-WORLDTEST);;
  --building) FIXTURE=(-BUILDINGTEST);;
  --mobile) FIXTURE=(-TEAMTEST -MOBILETEST);;
  --map) FIXTURE=(-MAPTEST);;
  --globals) FIXTURE=(-GLOBALTEST);;
esac
TD_CD=${TD_CD:-"$HOME/.local/share/Steam/steamapps/common/Command & Conquer"}
if [ ! -d "$TD_CD" ]; then
  echo "SKIP: $TD_CD: no game data; set TD_CD"
  exit 77
fi
# A failed load waits at the menu until this many seconds have passed.
TIMEOUT=120
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
ln -s "$TDSDL" "$WORK/tdsdl"
TDSDL="$WORK/tdsdl"
cd "$WORK"

run() {
  local log=$1
  shift
  if ! timeout "$TIMEOUT" env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$TDSDL" -NOMOVIES -SEED1 "$@" -CD"$TD_CD" > "$log" 2>&1; then
    tail -30 "$log"
    return 1
  fi
}

if [ "$LOAD_FIXTURE" = 1 ]; then
  SAVED_FIXTURE="$SCRIPT_DIR/../src/td/testdata/SAVEGAME.SCG01EA"
  # A save of another version is rejected and the game then waits at the menu
  # until the timeout, so compare the versions here. The header is a 44-byte
  # description, the scenario as an int32, the house as one byte, then the
  # version as a little-endian int32.
  want=$(sed -n 's/.*kSaveGameVersion = \([0-9]*\);.*/\1/p' "$SCRIPT_DIR/../src/td/saveload/saveload.h")
  have=$(od -An -tu4 -j49 -N4 "$SAVED_FIXTURE" | tr -d ' ')
  if [ "$have" != "$want" ]; then
    echo "FAIL: $SAVED_FIXTURE is a version $have save and kSaveGameVersion is $want;" \
      "regenerate it (src/td/testdata/README.md)"
    exit 1
  fi
  # A good load reaches frame 120 in a few seconds.
  TIMEOUT=60
  cp "$SAVED_FIXTURE" SAVEGAME.099
  run fixture.log -LOADGAME99 -QUITFRAME120 || { echo 'FAIL: fixture save did not load and run to frame 120'; exit 1; }
  states=$(sed -n 's/^.*] \(frame .*\)/\1/p' fixture.log | awk '$2 == 120' | wc -l)
  if [ "$states" -eq 0 ]; then
    tail -30 fixture.log
    echo 'FAIL: fixture save did not load and run to frame 120'
    exit 1
  fi
  echo "OK: fixture save loaded and ran to frame 120 ($states game states)"
  exit 0
fi

run first.log "-NEWGAME$SCENARIO" "${FIXTURE[@]}" -QUITFRAME60 -SAVESLOT99
test -s SAVEGAME.099 || { tail -30 first.log; echo 'FAIL: no save written'; exit 1; }
run continuous.log "-NEWGAME$SCENARIO" "${FIXTURE[@]}" -QUITFRAME120
run loaded.log -LOADGAME99 -QUITFRAME120

filter() {
  sed -n 's/^.*] \(frame .*\)/\1/p' "$1" | awk '$2 > 60 && ($3 == "unit" || $3 == "factory" || $3 == "trigger" || $3 == "teamtype" || $3 == "team" || $3 == "teamcount" || $3 == "house" || $3 == "template" || $3 == "overlay" || $3 == "smudge" || $3 == "anim" || $3 == "terrain" || $3 == "bullet" || $3 == "building" || $3 == "unitstate" || $3 == "infantrystate" || $3 == "aircraftstate" || $3 == "mapstate" || $3 == "globalstate")'
}
filter continuous.log > continuous.positions
filter loaded.log > loaded.positions
test -s loaded.positions || { tail -30 loaded.log; echo 'FAIL: no loaded positions'; exit 1; }
if [[ ${3:-} == --factory ]]; then
  grep -q ' factory ' loaded.positions || { echo 'FAIL: no loaded factory'; exit 1; }
fi
if [[ ${3:-} == --team ]]; then
  grep -q ' team ' loaded.positions || { echo 'FAIL: no loaded team'; exit 1; }
fi
if [[ ${3:-} == --mobile ]]; then
  for kind in unitstate infantrystate aircraftstate team; do
    grep -q " $kind " loaded.positions || { echo "FAIL: no loaded $kind"; exit 1; }
  done
fi
if [[ ${3:-} == --building ]]; then
  grep -q ' building ' loaded.positions || { echo 'FAIL: no loaded building'; exit 1; }
fi
if [[ ${3:-} == --world ]]; then
  for kind in template overlay smudge terrain bullet anim; do
    grep -q " $kind " loaded.positions || { echo "FAIL: no loaded $kind"; exit 1; }
  done
fi
if diff -u continuous.positions loaded.positions > difference.log; then
  echo "OK: $(wc -l < loaded.positions) game states identical across save/load"
  if [[ ${3:-} == --building ]]; then
    python3 "$SCRIPT_DIR/td_building_save_rejection.py" "$TDSDL" "$TD_CD"
  elif [[ ${3:-} == --mobile ]]; then
    python3 "$SCRIPT_DIR/td_mobile_save_rejection.py" "$TDSDL" "$TD_CD"
  elif [[ ${3:-} == --globals ]]; then
    python3 "$SCRIPT_DIR/td_globals_save_rejection.py" "$TDSDL" "$TD_CD"
  elif [[ ${3:-} == --map ]]; then
    python3 "$SCRIPT_DIR/td_map_save_rejection.py" "$TDSDL" "$TD_CD"
  fi
else
  head -30 difference.log | cut -c1-300
  echo 'FAIL: saved states diverge after load'
  exit 1
fi
