#!/bin/bash
# Usage: tools/record.sh [out.wav] [program arguments, default "script"]
# Runs dist/$NAME on the emulated Amiga 500 and records what the emulator plays into a WAV file (48 kHz
# stereo, 16 bit; silence before the first sound is cut off). Build first with tools/check.sh.
# The recording is what FS-UAE sends to the sound card: right for listening and for comparing levels, but the
# emulator runs against the wall clock, so two recordings are not sample-for-sample comparable.
cd "$(dirname "$0")/.."
. ./musicfx.conf
OUT="${1:-recording.wav}"; ARGS="${2:-script}"
[ -s "dist/$NAME" ] || { echo "no dist/$NAME: run tools/check.sh first"; exit 1; }
D=$(mktemp -d /tmp/mfxrec.XXXXXX)
cp "dist/$NAME" "$D/"; mkdir -p "$D/S"; printf '%s %s\necho >done.txt x\n' "$NAME" "$ARGS" > "$D/S/startup-sequence"
printf '[general]\ndrivers = wave\n[wave]\nfile = %s\n' "$D/raw.wav" > "$D/alsoft.conf"     # OpenAL writes a file
ALSOFT_CONF="$D/alsoft.conf" UAE_DONE="$D/done.txt" tools/run_uae.sh "$D" "${SECS:-120}" -
[ -s "$D/raw.wav" ] || { echo "FAIL: nothing was recorded"; rm -rf "$D"; exit 1; }
python3 tools/wavtrim.py "$D/raw.wav" "$OUT"
rm -rf "$D"
