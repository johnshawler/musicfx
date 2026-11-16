#!/bin/bash
# Usage: tools/build.sh <output-exe> <source files: .c .s .asm ...>
# Run from the project directory; the output and all sources must be inside it. Cross-compiles for a
# 68000 Amiga (Kickstart 1.3 and later) inside the amigadev/crosstools Docker image (m68k-amigaos-gcc + vasm),
# the same compiler image as a1200-final. First use: tools/setup.sh
# Extra flags: CFLAGS=... ASFLAGS=...   Other CPU: CPU=68020 tools/build.sh ...
OUT="$1"; shift
CPU="${CPU:-68000}"
docker run --rm -v "$PWD":/work -w /work -e CFLAGS="$CFLAGS" -e ASFLAGS="$ASFLAGS" -e CPU="$CPU" amigadev/crosstools:m68k-amigaos bash -c '
set -e
OUT="'"$OUT"'"; OBJS=""
NDK=/opt/m68k-amigaos/m68k-amigaos/ndk-include
mkdir -p "$(dirname "$OUT")" /tmp/obj
for f in '"$*"'; do
  o=/tmp/obj/$(echo "$f" | tr "/." "__").o
  case "$f" in
    *.c)       m68k-amigaos-gcc -m$CPU -O2 -fomit-frame-pointer -mcrt=nix13 -Wall $CFLAGS -c "$f" -o "$o" ;;
    */mixer.asm) vasmm68k_mot -quiet -Fhunk -kick1hunks -m$CPU -nowarn=62 -DBUILD_MIXER -I. $ASFLAGS -I"$(dirname "$f")" -I$NDK -o "$o" "$f" ;;
    *.s|*.asm) vasmm68k_mot -quiet -Fhunk -kick1hunks -m$CPU -nowarn=62 -I. $ASFLAGS -I"$(dirname "$f")" -I$NDK -o "$o" "$f" ;;
  esac
  OBJS="$OBJS $o"
done
m68k-amigaos-gcc -m$CPU -mcrt=nix13 $OBJS -o "$OUT"
m68k-amigaos-size "$OUT" | tail -1
'
