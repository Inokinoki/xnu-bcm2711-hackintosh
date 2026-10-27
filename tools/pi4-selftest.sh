#!/bin/sh
# Check the BCM2711 loader on QEMU raspi4b without an xnu build: boots tools/pi-selftest.S
# as the "kernel" and expects its banner, the boot_args, a top-of-RAM write/read and
# SELFTEST DONE on the mini UART.
#
#   [MEM=2G|4G] [CC=...] [OBJCOPY=...] tools/pi4-selftest.sh [SECONDS]
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
CC=${CC:-aarch64-linux-gnu-gcc}
OBJCOPY=${OBJCOPY:-aarch64-linux-gnu-objcopy}
OUT=${OUT:-$HERE/build/selftest}
mkdir -p "$OUT"
$CC -c -march=armv8-a "$HERE/tools/pi-selftest.S" -o "$OUT/selftest.o"
$CC -nostdlib -static -Wl,-Ttext=0 -o "$OUT/selftest.elf" "$OUT/selftest.o"
$OBJCOPY -O binary "$OUT/selftest.elf" "$OUT/selftest.bin"
python3 "$HERE/tools/mkselftest.py" "$OUT/selftest.bin" "$OUT/selftest.macho"
OUT="$OUT" "$HERE/tools/pi4-qemu-boot.sh" "$OUT/selftest.macho" "${1:-10}" || true
cat "$OUT/uart1.log"
grep -q "SELFTEST DONE" "$OUT/uart1.log" && grep -q "write/read OK" "$OUT/uart1.log" && echo "SELFTEST PASS"
