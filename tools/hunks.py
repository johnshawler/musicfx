#!/usr/bin/env python3
"""hunks.py <amiga-program> - lists the parts (hunks) of a built program and the kind of RAM each asks for.
A module, and anything else Paula plays directly, must sit in a part marked CHIP."""
import struct, sys
d = open(sys.argv[1], "rb").read()
if struct.unpack(">I", d[:4])[0] != 0x3F3:
    sys.exit("not an Amiga program")
count, first, last = struct.unpack(">III", d[8:20])
for i in range(last - first + 1):
    s = struct.unpack(">I", d[20 + i * 4:24 + i * 4])[0]
    print("hunk %d: %7d bytes  %s" % (i, (s & 0x3FFFFFFF) * 4, ("any RAM", "CHIP", "FAST", "?")[s >> 30]))
