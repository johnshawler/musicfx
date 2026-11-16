#!/bin/bash
# Usage: tools/check.sh
# The referee. Converts the effects, builds the program described in musicfx.conf, runs "<program> bench" on
# an emulated stock Amiga 500 (FS-UAE: cycle-exact 68000, 512K chip + 512K slow RAM, PAL), and checks the gates:
#   - the module is in chip RAM (music_in_chip=1): otherwise Paula plays silence
#   - four effects at once all play, a fifth of lower priority is refused, nothing else is dropped
#   - music + mixer with every voice busy take at most MAX_CPU_PCT of the machine
# Leaves bench.txt in the project folder. Exit code 0 = PASS. CI runs exactly this.
cd "$(dirname "$0")/.."
. ./musicfx.conf
SFXOPT="--period $PERIOD --gain ${GAIN:-1.0}"
if [ "$HQ" = 1 ]; then SFXOPT="$SFXOPT --hq"; else SFXOPT="$SFXOPT --voices $VOICES"; fi
rm -rf assets/fx; mkdir -p assets/fx dist
python3 tools/sfx.py $SFXOPT assets/fx $EFFECTS || { echo "FAIL: effects"; exit 1; }
rm -f "dist/$NAME" bench.txt
CFLAGS="$CFLAGS" ASFLAGS="-I$CONFIG_DIR -DMIXER_HQ_MODE=$HQ -Dmixer_sw_channels=$VOICES -Dmixer_period=$PERIOD $ASFLAGS" \
  tools/build.sh "dist/$NAME" $SOURCES || { echo "FAIL: build"; exit 1; }
[ -s "dist/$NAME" ] || { echo "FAIL: build produced no dist/$NAME"; exit 1; }
python3 tools/hunks.py "dist/$NAME" | grep -q CHIP || echo "warning: no part of dist/$NAME asks for chip RAM (see kit/incbin.h)"

D=$(mktemp -d /tmp/mfxcheck.XXXXXX)
cp "dist/$NAME" "$D/"; mkdir -p "$D/S"; printf '%s bench %s\n' "$NAME" "$FRAMES" > "$D/S/startup-sequence"
UAE_DONE="$D/bench.txt" tools/run_uae.sh "$D" $(( FRAMES / 10 + 90 )) -
if [ -e "$D/bench.txt" ]; then cp "$D/bench.txt" bench.txt; else echo "FAIL: no bench.txt (crashed, hung, or did not finish)"; rm -rf "$D"; exit 1; fi
rm -rf "$D"
cat bench.txt
python3 tools/check_gates.py bench.txt --voices "$VOICES" --max-cpu "$MAX_CPU_PCT"
