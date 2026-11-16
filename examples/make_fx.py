#!/usr/bin/env python3
"""make_fx.py - writes four small original test effects (WAV, 22050 Hz, 16 bit mono) into examples/fx/.
They are synthetic and belong to this template: use them until the real game's effects are in original/.
tools/sfx.py turns them into mixer samples."""
import math, os, random, struct, wave
RATE = 22050
random.seed(7)

def env(i, n, attack=0.01, release=0.3):
    t = i / n
    return min(1.0, t / attack) * min(1.0, (1 - t) / release)

def laser(n):   # falling sweep
    ph = 0; out = []
    for i in range(n):
        f = 2400 * (1 - i / n) ** 2 + 300
        ph += f / RATE
        out.append((1 if ph % 1 < 0.5 else -1) * 0.6 * env(i, n))
    return out

def boom(n):    # filtered noise, slow decay
    out = []; y = 0
    for i in range(n):
        y += (random.uniform(-1, 1) - y) * (0.5 * (1 - i / n) + 0.02)
        out.append(max(-1, min(1, y * 2.0)) * 0.8 * (1 - i / n) ** 1.5)
    return out

def coin(n):    # two high notes
    out = []
    for i in range(n):
        f = 988 if i < n // 4 else 1319
        out.append(math.sin(2 * math.pi * f * i / RATE) * 0.9 * env(i, n, 0.005, 0.6))
    return out

def jump(n):    # rising tone
    ph = 0; out = []
    for i in range(n):
        ph += (300 + 900 * i / n) / RATE
        out.append(math.sin(2 * math.pi * ph) * 0.9 * env(i, n, 0.02, 0.2))
    return out

here = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fx")
os.makedirs(here, exist_ok=True)
for name, fn, secs in (("laser", laser, 0.35), ("boom", boom, 0.9), ("coin", coin, 0.4), ("jump", jump, 0.3)):
    data = fn(int(RATE * secs))
    with wave.open(os.path.join(here, name + ".wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(v * 32000)) for v in data))
    print("examples/fx/%s.wav" % name, len(data), "samples")
