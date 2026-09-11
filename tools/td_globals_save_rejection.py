#!/usr/bin/env python3
"""Validate TD version-10 globals and reject malformed object lists/base records."""
import os
from pathlib import Path
import struct
import subprocess
import sys

binary, cd_path = sys.argv[1:]
save = Path("SAVEGAME.099")
original = save.read_bytes()

def section(tag):
    assert original.count(tag) == 1, tag
    return original.index(tag)

score, base, misc = map(section, (b"SCOR", b"BASE", b"MISC"))
assert struct.unpack_from("<Q", original, score + 44)[0] >= 1 << 40
assert struct.unpack_from("<i", original, score + 4)[0] == 101
assert struct.unpack_from("<i", original, base + 8)[0] == 2
# Two fixed 512-byte movie names follow the house index and scenario number.
selection = misc + 4 + 4 + 4 + 512 * 2
assert struct.unpack_from("<i", original, selection)[0] == 2
assert original[selection + 4:selection + 6] != original[selection + 6:selection + 8]
layer = original.index(b"LAYR")
assert struct.unpack_from("<i", original, layer + 4)[0] >= 2
first_target = struct.unpack_from("<H", original, layer + 8)[0]
cases = [
    ("negative layer count", layer + 4, "<i", -1, "invalid saved object list count"),
    ("oversized layer count", layer + 4, "<i", 65537, "invalid saved object list count"),
    ("null layer reference", layer + 8, "<H", 0, "invalid saved object list reference"),
    ("unallocated layer slot", layer + 8, "<H", 0x2000 | 299, "saved object list references an unallocated slot"),
    ("out-of-range layer slot", layer + 8, "<H", 0x2000 | 4095, "saved object index outside its heap"),
    ("wrong layer kind", layer + 8, "<H", 0xF000, "saved target is not an object kind"),
    ("duplicate layer reference", layer + 10, "<H", first_target, "duplicate saved object list reference"),
    ("invalid base house", base + 4, "<i", 1000000, "invalid saved base house or count"),
    ("negative base count", base + 8, "<i", -1, "invalid saved base house or count"),
    ("oversized base count", base + 8, "<i", 4097, "invalid saved base house or count"),
    ("invalid base building", base + 12, "<i", 1000000, "invalid saved base building type"),
    ("missing player", misc + 4, "<i", -1, "invalid saved player house"),
    ("oversized selection", selection, "<i", 65537, "invalid saved object list count"),
    ("duplicate selection", selection + 6, "<H", struct.unpack_from("<H", original, selection + 4)[0], "duplicate saved object list reference"),
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
    print(f"OK: wide score, base nodes and ordered selection preserved; {len(cases)} corrupt globals rejected")
finally:
    save.write_bytes(original)
