# XNU on Raspberry Pi 4 (BCM2711)

Fork of [xnu-bcm2837-hackintosh](https://github.com/Inokinoki/xnu-bcm2837-hackintosh) (Raspberry Pi 3) retargeted
at the Raspberry Pi 4: Cortex-A72, the BCM2711 peripheral map at `0xFE000000`, and up to 4 GiB of RAM.

## Status

Fork point: upstream `rootfs-ramdisk` (xnu-8796.141.3, HFS+ RAM disk root, SD root driver). What has been checked here:

- **Loader + packing on QEMU `raspi4b`: works.** `tools/pi4-selftest.sh` boots the loader with a stand-in kernel
  (`tools/pi-selftest.S`) and sees the loader banner, correct `boot_args`, the command line, a write/read at the top
  of the kernel's RAM window and `SELFTEST PASS` on the mini UART (checked with `-m 2G`).
- **The BCM2711 kernel builds** (CI run 40557062839, artifact `xnu-8796.141.3-bcm2711-kernel`, file `kernel.development.bcm2837`)
  **and boots on `raspi4b -cpu cortex-a72 -m 2G` when the kernel is given up to 1 GiB** (`MEM_SIZE=0x40000000`):
  VM, zones, IOKit, the platform expert and the boot CPU all come up (`BCM2837 CPU: boot CPU 0 registered`), the same
  point the Pi 3 port reaches. Retargeting by hand worked on the first CI run.
- **Open: with 1.25 GiB or more for the kernel (the 2G default is ~1.5 GiB), it panics early** with
  `kmem_range_init: vm_map_locate_space failing for claim <zones|compressor|kmem_*_range>` (vm_kern.c:3645). The claim
  varies per run, so it is the kernel VA layout (claims sized to fill the largest hole; more RAM = bigger physmap
  in the 39-bit window), not one claim. Until fixed: `MEM_SIZE=0x40000000 MEM=2G tools/pi4-qemu-boot.sh KERNEL`.
- Boot args now include `wdt=-1`: QEMU is slow enough to trip the interrupts-disabled watchdog otherwise.
- **`-m 4G` needs a newer QEMU than 9.0.90**, which refuses it (`Invalid RAM size, should be 2 GiB`). The layout code
  supports 1G/2G/4G/8G but only 2G has been run.
- Real Pi 4 hardware: untested. Known open points: mini UART baud (kernel driver assumes `core_freq=250`),
  SD slot is EMMC2 on hardware (`sdbase=0xfe340000` boot-arg; QEMU uses EMMC at `0xfe300000`), rng200 seed
  (loader built with `LOADER_CFLAGS=-DUSE_RNG200`; QEMU has no RNG so the default is a non-secure counter seed).
- Still one CPU (`cpus=1`), no GIC-400 driver yet (the architected timer goes through FIQ as on the Pi 3 port).

xnu's own Pi board config is spelled `BCM2837` (`MACHINE_CONFIG`, `ARM64_BOARD_CONFIG_BCM2837`); it is kept so the
upstream xnu sources still match, the patches change what it means.

## Build, run and validate

```
# kernel: push, then GitHub Actions "xnu-8796.141.3-bcm2711" -> artifact xnu-8796.141.3-bcm2711-kernel
# loader + payload + QEMU (needs aarch64 clang/gcc, qemu-system-aarch64 with raspi4b, python3):
MEM=2G tools/pi4-qemu-boot.sh KERNEL 60                  # runs: qemu-system-aarch64 -machine raspi4b -cpu cortex-a72 -m 2G ...
RAMDISK=root.img MEM=2G tools/pi4-qemu-boot.sh KERNEL    # HFS+ root: tools/mkpi-init.sh + tools/mkhfsroot.py as before
SDIMG=sd.img tools/pi4-qemu-boot.sh KERNEL               # rd=sdroot0
# validate the loader without a kernel build:
MEM=2G CC="clang --target=aarch64-linux-gnu" OBJCOPY=llvm-objcopy tools/pi4-selftest.sh
```

Equivalent to `qemu-system-aarch64 -machine raspi4b -cpu cortex-a72 -m 2G -kernel loader.elf -device loader,file=payload.bin,addr=PAYLOAD_PA,force-raw=on -serial null -serial file:uart1.log`.
Memory layout: RAM from `0x10000000`; the payload sits 256 MiB under the top of RAM (below the `0xfc000000`
peripheral window on 4G) and the kernel gets everything in between (about 1.5 GiB with `-m 2G`, about 3.4 GiB with 4G).

## Upstream roadmap (Pi 3 port, carried over)


XNU Kernel:

- [x] Compile Mach-O (xnu-8796.141.3, `xnu/patches-8796.141.3/`, built by `.github/workflows/xnu-8796.141.3-bcm2711.yml`)
- [x] Mach-O loader (`loader/`): EL3/EL2 to EL1, copy the segments, boot_args and a minimal ADT, jump
- [x] Make sure the kernel is bootable (on QEMU `raspi4b`; real hardware untested)
- [x] Make sure UART works (the mini UART, set up without an ADT)
- [ ] Root filesystem and `launchd`
- [ ] Try to make a VC4 driver

Hackintosh:

- [ ] Try to make an ARM64 macOS sysroot

## How it is built and booted

- `xnu/patches-8796.141.3/` are applied on top of xnu-8796.141.3 by
  [blacktop/darwin-xnu-build](https://github.com/blacktop/darwin-xnu-build) on a macOS runner.
  The kernel is plain arm64 (no PAC), built for the Cortex-A72, with 4K pages and the
  standard non-large-memory layout (linked at `0xfffffff007004000`).
- `tools/mkpi_payload.py` packs the kernel Mach-O, a generated ADT (`tools/pi_adt.py`) and the
  command line; `loader/` is the Pi-side image that unpacks it.
- `tools/pi4-qemu-boot.sh KERNEL [SECONDS]` builds the loader, packs the kernel and boots QEMU
  `raspi4b`; the kernel console is QEMU's second serial port (the mini UART).
  `tools/pi4-qemu-panic.sh` catches the first panic under gdb; `tools/macho_sym.py` looks up symbols.

Needs: an aarch64 gcc, `qemu-system-aarch64`, `gdb-multiarch` (optional) and Python 3.

The first attempt used xnu-7195.81.3 ([notes](Build/XNU-7195.81.3.md), patches in `xnu/patches-7195.81.3/`).
It is kept for reference but superseded: iOS 16 userland needs the xnu-8796.141.3 kernel.

## 16K user pages on the 4K kernel

iOS 16 userland is built for 16K pages. `tools/pi-pagesize.S` (build with
`SRC=pi-pagesize.S tools/mkpi-init.sh OUT`, run as `/sbin/launchd`) shows what a process sees on this
4K-page kernel (QEMU `raspi4b`): `hw.pagesize` = 0x4000, commpage user page shift = 14 (kernel shift 12),
a 64K anonymous mmap is 16K aligned, and a 4K `mprotect` inside a 16K block fails. So user maps use 16K
pages on top of 4K hardware pages. Real dyld and libsystem are still untested (they need the dyld shared
cache, about 2 GB, so the root must be on block storage, not a RAM disk).

