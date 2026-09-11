#!/usr/bin/env python3
"""Exercise version-8 mobile validation through the real loader.

Run by td_saveload_smoke.sh --mobile in its isolated working directory.
"""
import os
from pathlib import Path
import re
import struct
import subprocess
import sys

binary, cd_path = sys.argv[1:]
save = Path("SAVEGAME.099")
original = save.read_bytes()
log = Path("first.log").read_text()

def fields(kind):
    return [
        bytes.fromhex(value)
        for value in re.findall(r"frame 60 " + kind + r" \d+ fields ([0-9a-f]+)", log)
    ]

# Derive shared Techno size from a building (56-byte building suffix). Mobile
# records then add eight flags, speed, three TARGETs, team, group, and member.
base_size = len(fields("building")[0]) - 56
path_offset = base_size + 22
unit = next(value for value in fields("unitstate") if struct.unpack_from("<i", value, len(value) - 4)[0] >= 0)
infantry = fields("infantrystate")[-1]
aircraft = fields("aircraftstate")[-1]
cases = [
    ("oversized path", unit, path_offset, 1000000, "invalid saved path length"),
    ("invalid path direction", unit, path_offset + 4, 1000000, "invalid saved path facing"),
    ("invalid team slot", unit, base_size + 15, 1000000, "invalid saved mobile team index"),
    ("invalid drive track", unit, len(unit) - 29, 1000000, "invalid saved drive state"),
    ("invalid flag owner", unit, len(unit) - 4, 1000000, "invalid saved unit flag owner"),
    ("invalid infantry action", infantry, len(infantry) - 18, 1000000, "invalid saved infantry state"),
    ("missing aircraft type", aircraft, len(aircraft) - 31, -1, "missing saved aircraft type"),
]
try:
    for name, payload, field_offset, value, error in cases:
        offset = original.find(payload)
        assert offset >= 0 and original.count(payload) == 1, name + ": payload not unique"
        corrupted = bytearray(original)
        struct.pack_into("<i", corrupted, offset + field_offset, value)
        save.write_bytes(corrupted)
        result = subprocess.run(
            [binary, "-NOMOVIES", "-LOADGAME99", "-QUITFRAME61", "-CD" + cd_path],
            env={**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"},
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30,
        )
        assert error in result.stdout, f"{name}: missing rejection\n{result.stdout[-2000:]}"
        assert result.returncode in (0, 1), f"{name}: abnormal exit {result.returncode}"
        assert "-LOADGAME: could not load slot" in result.stdout, name
        assert not re.search(r"frame 61 ", result.stdout), name
    print(f"OK: {len(cases)} corrupt mobile saves rejected before gameplay")
finally:
    save.write_bytes(original)
