#!/usr/bin/env python3
"""wavtrim.py in.wav out.wav - converts OpenAL's 32-bit float recording to 16 bit, cuts the silence before the
first sound and after the last, and prints the level of the left and right side for every second.
Paula's channels 0 and 3 are on the left, 1 and 2 on the right (FS-UAE blends the sides a little)."""
import math, struct, sys
d = open(sys.argv[1], "rb").read()
fmt = d.find(b"fmt "); data = d.find(b"data")
tag, ch, rate, _, _, bits = struct.unpack("<HHIIHH", d[fmt + 8:fmt + 24])
body = d[data + 8:]
if bits == 32 and tag in (3, 0xFFFE):
    vals = struct.unpack("<%df" % (len(body) // 4), body[:len(body) // 4 * 4])
    vals = [max(-32768, min(32767, int(v * 32767))) for v in vals]
elif bits == 16:
    vals = list(struct.unpack("<%dh" % (len(body) // 2), body[:len(body) // 2 * 2]))
else:
    sys.exit("unexpected WAV format: %d bits, tag %d" % (bits, tag))
frames = len(vals) // ch
loud = [i for i in range(frames) if any(abs(vals[i * ch + c]) > 64 for c in range(ch))]
if not loud:
    sys.exit("FAIL: the recording is silent")
a, b = max(0, loud[0] - rate // 10), min(frames, loud[-1] + rate // 2)
out = struct.pack("<%dh" % ((b - a) * ch), *vals[a * ch:b * ch])
with open(sys.argv[2], "wb") as f:
    f.write(b"RIFF" + struct.pack("<I", 36 + len(out)) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, ch, rate, rate * ch * 2, ch * 2, 16))
    f.write(b"data" + struct.pack("<I", len(out)) + out)
print("%s: %.1f s, %d Hz, %d channels" % (sys.argv[2], (b - a) / rate, rate, ch))
for s in range(0, (b - a) // rate):
    lv = []
    for c in range(ch):
        seg = vals[(a + s * rate) * ch + c:(a + (s + 1) * rate) * ch:ch]
        lv.append(math.sqrt(sum(v * v for v in seg) / max(1, len(seg))))
    print("%3d s  " % s + "  ".join("%s %5.0f" % ("LR"[c] if ch == 2 else c, lv[c]) for c in range(ch)))
