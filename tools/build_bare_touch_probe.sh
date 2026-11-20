#!/bin/sh
# Build the user-owned Mandex-style bare ARM7/ARM9 touchscreen probe.
# No libnds, no Calico, no BlocksDS IPC; untouched normal ROM builds.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)"
DEVKITPRO="${DEVKITPRO:-/opt/devkitpro}"
DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export PATH="$DEVKITARM/bin:$DEVKITPRO/tools/bin:$PATH"
OUT="${1:-$ROOT/bare-touch-arm7.nds}"
SRC="$ROOT/diagnostics/bare_touch"
W="$(mktemp -d)"
trap 'rm -rf "$W"' EXIT
CC=arm-none-eabi-gcc
$CC -march=armv5te -mtune=arm946e-s -marm -O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -c "$SRC/arm9.c" -o "$W/arm9.o"
$CC -march=armv5te -marm -c "$SRC/crt9.S" -o "$W/crt9.o"
$CC -nostartfiles -nostdlib -march=armv5te -marm -T "$SRC/arm9.ld" "$W/crt9.o" "$W/arm9.o" -o "$W/arm9.elf" -lgcc
arm-none-eabi-objcopy -O binary "$W/arm9.elf" "$W/arm9.body"
$CC -march=armv4t -mcpu=arm7tdmi -marm -O2 -ffreestanding -fno-unwind-tables -fno-asynchronous-unwind-tables -c "$SRC/arm7.c" -o "$W/arm7.o"
$CC -march=armv4t -mcpu=arm7tdmi -marm -c "$SRC/crt7.S" -o "$W/crt7.o"
$CC -nostartfiles -nostdlib -march=armv4t -marm -T "$SRC/arm7.ld" "$W/crt7.o" "$W/arm7.o" -o "$W/arm7.elf" -lgcc
arm-none-eabi-objcopy -O binary "$W/arm7.elf" "$W/arm7.bin"
python3 - "$W/arm9.body" "$W/arm9.bin" <<'PY'
from pathlib import Path
import sys
body = Path(sys.argv[1]).read_bytes()
Path(sys.argv[2]).write_bytes(bytes(0x800) + body)
PY
ndstool -c "$OUT" -h 0x4000 -9 "$W/arm9.bin" -7 "$W/arm7.bin" -r9 0x02000000 -e9 0x02000800 -r7 0x02380000 -e7 0x02380000
test -s "$OUT"
ndstool -i "$OUT"
echo "Built bare-metal ARM7 touch test: $OUT"
