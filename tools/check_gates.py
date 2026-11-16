#!/usr/bin/env python3
"""check_gates.py bench.txt --voices N --max-cpu PCT - the gates of tools/check.sh. Exit code 0 = PASS."""
import argparse, re, sys
ap = argparse.ArgumentParser()
ap.add_argument("bench"); ap.add_argument("--voices", type=int, default=4); ap.add_argument("--max-cpu", type=float, default=15)
a = ap.parse_args()
v = dict(re.findall(r"(\w+)=([\d.]+)", open(a.bench).read()))
fail = []
def need(name):
    if name not in v:
        fail.append("bench.txt has no %s" % name); return 0.0
    return float(v[name])
if need("music_in_chip") != 1: fail.append("the module is not in chip RAM: Paula plays silence (kit/incbin.h)")
if need("overload_played") != a.voices: fail.append("%s of %d simultaneous effects played" % (v.get("overload_played"), a.voices))
if need("overload_refused") != 1: fail.append("the extra low-priority effect was not refused: an important one was cut")
cpu = max(need("music_pct"), need("one_fx_pct"), need("all_fx_pct"))
if cpu > a.max_cpu: fail.append("audio takes %.1f%% of the machine, the limit is %g%%" % (cpu, a.max_cpu))
if need("idle") < 1000: fail.append("the idle count is too small to measure with")
for f in fail: print("FAIL:", f)
if fail: sys.exit(1)
print("PASS (music on 3 channels untouched, %d effects at once, audio %.1f%% of an A500)" % (a.voices, cpu))
