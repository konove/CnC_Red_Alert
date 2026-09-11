#!/usr/bin/env python3
"""Validate sparse cell fixtures and reject corrupt TD version-9 map records."""
import os
from pathlib import Path
import struct
import subprocess
import sys

binary, cd_path = sys.argv[1:]
save = Path("SAVEGAME.099")
original = save.read_bytes()
assert original.count(b"MAPS") == original.count(b"MCEL") == 1
maps = original.index(b"MAPS")
cells = original.index(b"MCEL")
count = struct.unpack_from("<i", original, cells + 4)[0]
records = {}
cursor = cells + 8
for _ in range(count):
    cell = struct.unpack_from("<h", original, cursor)[0]
    data = cursor + 2
    flags = original[data]
    overlaps = struct.unpack_from("<i", original, data + 26)[0]
    assert 0 <= overlaps <= 3
    land = data + 31 + 2 * overlaps
    trigger = land + 4 if flags & 16 else None
    records[cell] = (cursor, data, land, trigger)
    cursor = land + 4 + (2 if trigger is not None else 0)

assert all(cell in records for cell in range(16)), "a sparse fixture cell was omitted"
assert struct.unpack_from("<q", original, maps + 24)[0] == 1 << 35
assert original[records[11][3]:records[11][3] + 2] == original[records[12][3]:records[12][3] + 2]
first, second = sorted(records)[:2]
cases = [
    ("unknown theater", maps + 4, "<i", 1000000, "invalid saved theater"),
    ("zero map width", maps + 16, "<i", 0, "invalid map dimensions"),
    ("oversized growth list", maps + 32, "<i", 51, "invalid map dimensions"),
    ("too many cells", cells + 4, "<i", 1000000, "invalid saved cell count"),
    ("duplicate cell", records[second][0], "<h", first, "invalid or duplicate saved cell index"),
    ("too many overlappers", records[14][1] + 26, "<i", 4, "invalid cell overlapper count"),
    ("invalid land type", records[10][2], "<i", 1000000, "invalid saved cell attributes"),
    ("wrong trigger kind", records[11][3], "<H", 8 << 12, "invalid saved trigger target"),
]
try:
    for name, offset, fmt, value, error in cases:
        corrupted = bytearray(original)
        struct.pack_into(fmt, corrupted, offset, value)
        save.write_bytes(corrupted)
        result = subprocess.run(
            [binary, "-NOMOVIES", "-LOADGAME99", "-QUITFRAME61", "-CD" + cd_path],
            env={**os.environ, "SDL_VIDEODRIVER": "dummy", "SDL_AUDIODRIVER": "dummy"},
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=30,
        )
        assert error in result.stdout, f"{name}: missing rejection\n{result.stdout[-2000:]}"
        assert result.returncode in (0, 1), f"{name}: abnormal exit {result.returncode}"
        assert "-LOADGAME: could not load slot" in result.stdout, name
        assert "frame 61 " not in result.stdout, name
    print(f"OK: 16 sparse cells and wide map value preserved; {len(cases)} corrupt maps rejected")
finally:
    save.write_bytes(original)
