#!/bin/sh
# Boot an xnu-bcm2711 kernel on QEMU raspi4b through the loader.
#
#   [RAMDISK=image | SDIMG=image] [CMDLINE=...] tools/pi4-qemu-boot.sh KERNEL [SECONDS]
#
# MEM (default 2G) is the -m value and sets the layout: RAM is 0..MEM minus the
# peripheral window at 0xfc000000, the payload sits 256 MiB below the top of
# that, and the kernel gets 0x10000000 up to the payload. QEMU 9.0 only accepts
# -m 2G for raspi4b ("Invalid RAM size, should be 2 GiB"); 4G needs a newer QEMU.
#
# Needs an aarch64 gcc (CC, default aarch64-linux-gnu-gcc), qemu-system-aarch64
# (QEMU, default qemu-system-aarch64) and python3. The mini UART (what the
# kernel's Pi 4 driver uses) is QEMU's serial1; the log lands in OUT/uart1.log.
set -eu
KERNEL=${1:?usage: $0 KERNEL [SECONDS]}
SECS=${2:-60}
CC=${CC:-aarch64-linux-gnu-gcc}
QEMU=${QEMU:-qemu-system-aarch64}
HERE=$(cd "$(dirname "$0")/.." && pwd)
OUT=${OUT:-$HERE/build/qemu-pi4}
MEM=${MEM:-2G}
case "$MEM" in
    1G) TOP=0x40000000 ;; 2G) TOP=0x80000000 ;; 4G|8G) TOP=0xfc000000 ;;
    *) echo "MEM must be 1G, 2G, 4G or 8G"; exit 1 ;;
esac
PAYLOAD_PA=$((TOP - 0x10000000))
MEM_SIZE=${MEM_SIZE:-$((PAYLOAD_PA - 0x10000000))}
mkdir -p "$OUT"

# 1. loader (loader/*.S, loader/*.c, loader/link.ld) at 0x80000
$CC -c -march=armv8-a "$HERE/loader/start.S" -o "$OUT/start.o"
$CC -c -march=armv8-a ${LOADER_CFLAGS:-} -DPAYLOAD_PA=$(printf 0x%xUL $PAYLOAD_PA) -mgeneral-regs-only -ffreestanding -fno-builtin \
    -fno-stack-protector -fno-pic -Wall -Wextra -O2 "$HERE/loader/main.c" -o "$OUT/main.o"
$CC -nostdlib -static -T "$HERE/loader/link.ld" -o "$OUT/loader.elf" "$OUT/start.o" "$OUT/main.o"

# 2. payload: the kernel's segments + ADT + command line, placed at PAYLOAD_PA
# RAMDISK=image makes the payload carry a RAM disk root (boot-arg rd=md0);
# SDIMG=image attaches it as the SD card (a power-of-two size) and roots on it (rd=sdroot0).
# CMDLINE overrides the default boot-args.
set -- "$KERNEL" "$OUT/payload.bin" --mem-size "$MEM_SIZE"
if [ -n "${RAMDISK:-}" ]; then
    set -- "$@" --ramdisk "$RAMDISK"
    : "${CMDLINE:=debug=0xc serial=3 cpus=1 wdt=-1 keepsyms=1 kt_fixed_zones=120 -noprogress rd=md0}"
fi
if [ -n "${SDIMG:-}" ]; then
    : "${CMDLINE:=debug=0xc serial=3 cpus=1 wdt=-1 keepsyms=1 kt_fixed_zones=120 -noprogress rd=sdroot0}"
fi
[ -n "${CMDLINE:-}" ] && set -- "$@" --cmdline "$CMDLINE"
python3 "$HERE/tools/mkpi_payload.py" "$@"
echo "$PAYLOAD_PA" > "$OUT/payload.pa"
echo "$MEM" > "$OUT/payload.mem"

# 3. run; QEMU's generic loader puts the payload at PAYLOAD_PA
rm -f "$OUT/uart1.log"
set +e
timeout "$SECS" "$QEMU" -M raspi4b -cpu cortex-a72 -m "${QEMU_M:-$MEM}" -kernel "$OUT/loader.elf" \
    -device loader,file="$OUT/payload.bin",addr=$PAYLOAD_PA,force-raw=on \
    ${SDIMG:+-drive file="$SDIMG",if=sd,format=raw} \
    -display none -serial null -serial file:"$OUT/uart1.log" \
    -D "$OUT/qemu.log" -d ${QEMU_D:-guest_errors} -no-reboot
rc=$?
set -e
echo "qemu exit=$rc (124 = still running at the timeout)"
echo "uart1 log: $OUT/uart1.log"
