#!/usr/bin/env python3
"""Corrupt version-7 building fields and verify the real TD loader rejects them.

Called by td_saveload_smoke.sh --building inside its isolated working directory.
Uses the frame-60 field dump to locate a building without relying on raw map sizes.
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
fields = [
    bytes.fromhex(value)
    for value in re.findall(r"frame 60 building \d+ fields ([0-9a-f]+)", Path("first.log").read_text())
]
# Building's own suffix is 56 bytes: two indices, ActLike, eight flags, two
# countdowns, two animation enums, last attacker, TARGET, and last strength.
payload = next(value for value in fields if struct.unpack_from("<i", value, len(value) - 52)[0] >= 0)
offset = original.find(payload)
assert offset >= 0 and original.count(payload) == 1, "building payload must be unique in the save"
# Object is 17 bytes; Mission adds 22; Radio stores a 4-byte enum then a TARGET.
cases = [
    ("negative factory index", len(payload) - 52, "<i", -2, "invalid saved building factory index"),
    ("large factory index", len(payload) - 52, "<i", 1000000, "invalid saved building factory index"),
    ("missing building type", len(payload) - 56, "<i", -1, "invalid saved building state"),
    ("unknown building animation", len(payload) - 27, "<i", 1000000, "invalid saved building state"),
    ("unknown mission", 17, "<i", 1000000, "invalid saved mission"),
    ("radio points to bullet", 43, "<H", 8 << 12, "saved target has the wrong kind"),
]
try:
    for name, field_offset, fmt, value, error in cases:
        corrupted = bytearray(original)
        struct.pack_into(fmt, corrupted, offset + field_offset, value)
        save.write_bytes(corrupted)
        result = subprocess.run(
            [binary, "-NOMOVIES", "-LOADGAME99", "-QUITFRAME61", "-CD" + cd_path],
            env={**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"},
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30,
        )
        assert error in result.stdout, f"{name}: missing expected rejection\n{result.stdout[-2000:]}"
        assert result.returncode in (0, 1), f"{name}: abnormal exit {result.returncode}"
        assert "-LOADGAME: could not load slot" in result.stdout, name
        assert not re.search(r"frame 61 ", result.stdout), name
    print(f"OK: {len(cases)} corrupt building saves rejected before gameplay")
finally:
    save.write_bytes(original)
