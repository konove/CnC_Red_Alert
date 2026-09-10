#!/usr/bin/env bash
# Starts a TD campaign, saves at frame 60, and compares the next 60 frames
# with an uninterrupted run using the same fixed scenario seed. Set TD_CD to override the Steam data directory.
# Usage: tools/td_saveload_smoke.sh [tdsdl] [scenario, default SCG01EA] [--factory|--team]
set -euo pipefail

TDSDL=$(realpath "${1:-cmake-build-strict-ra-clang/src/td/tdsdl}")
SCENARIO=${2:-SCG01EA}
FIXTURE=()
case ${3:-} in
  --factory) FIXTURE=(-FACTORYTEST);;
  --team) FIXTURE=(-TEAMTEST);;
esac
TD_CD=${TD_CD:-"$HOME/.local/share/Steam/steamapps/common/Command & Conquer"}
WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
ln -s "$TDSDL" "$WORK/tdsdl"
TDSDL="$WORK/tdsdl"
cd "$WORK"

run() {
  local log=$1
  shift
  if ! timeout 120 env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$TDSDL" -NOMOVIES -SEED1 "$@" -CD"$TD_CD" > "$log" 2>&1; then
    tail -30 "$log"
    return 1
  fi
}

run first.log "-NEWGAME$SCENARIO" "${FIXTURE[@]}" -QUITFRAME60 -SAVESLOT99
test -s SAVEGAME.099 || { tail -30 first.log; echo 'FAIL: no save written'; exit 1; }
run continuous.log "-NEWGAME$SCENARIO" "${FIXTURE[@]}" -QUITFRAME120
run loaded.log -LOADGAME99 -QUITFRAME120

filter() {
  sed -n 's/^.*] \(frame .*\)/\1/p' "$1" | awk '$2 > 60 && ($3 == "unit" || $3 == "factory" || $3 == "trigger" || $3 == "teamtype" || $3 == "team" || $3 == "teamcount" || $3 == "house")'
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
if diff -u continuous.positions loaded.positions > difference.log; then
  echo "OK: $(wc -l < loaded.positions) unit/factory/trigger/team/house states identical across save/load"
else
  head -30 difference.log | cut -c1-300
  echo 'FAIL: saved states diverge after load'
  exit 1
fi
