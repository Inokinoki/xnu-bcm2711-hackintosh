# Plan: from the QEMU boot to a real Raspberry Pi 4

Written 2026-11-13. Status of the userland itself is in [userland-plan.md](userland-plan.md).

## Where it stands

Everything below is verified on QEMU `raspi4b` (2 GiB, one CPU) only. Nothing has run on real hardware.

- **Kernel:** xnu-8796.141.3 boots with patches 0001-0043, a GIC-400 driver, and HFS+ and SD-root support.
- **Loader:** drops EL3/EL2 to EL1, unpacks the kernel, device tree and command line, and sets up the GIC for the
  FIQ timer.
- **Userland:** 92 Mach-O files built in CI, booting to a shell from a RAM disk, with `mount` and friends, the
  bootstrap server, notifyd, DNS and libcommonCrypto.

The realistic target is a Darwin system with a shell on the serial console (and later HDMI text). The macOS GUI is
out of reach: WindowServer and the graphics stack are closed and need a GPU driver.

## Phase 0: preparation that needs no hardware

1. **SD-card image in CI.** Build a FAT boot partition (Pi firmware, `config.txt`, loader, payload) plus the root,
   and publish it as an artifact.
2. **Payload loading without QEMU's `-device loader`.** On hardware the firmware must place the payload. Use
   `initramfs payload.bin <address>` in `config.txt`, or append the payload to the loader image.
3. **Boot the full userland with `rd=sdroot0` in QEMU.** Done (userland-boot run 43499519728): the boot job boots
   the same root from an SD card image after the RAM-disk boot, read-write, through to `USERLAND-SELFTEST-OK`.
   It needed two kernel fixes: patch 0038 enabled the SDHCI signal register (`0x38`) instead of the status register
   (`0x34`), so no command completed; patch 0044 drops the root authentication check, which panicked on any
   non-RAM-disk root. The raw HFS+ volume sits at sector 0 of the card: the driver has no partition table support,
   which item 1 needs (a FAT boot partition in front of the root).
4. **4 GiB layout.** Done, in CI (`boot-4g` job of userland-boot, run 43523629464). Upstream QEMU (checked up to
   11.1.2) fixes `raspi4b` at 2 GiB through its board revision, so the job builds QEMU 10.1.0 with the 4 GiB revision
   (`c03115`, cached; `tools/qemu-4g.Dockerfile` does the same in Docker). With `MEM=4G` (kernel window
   `0x10000000`-`0xec000000`, 3.44 GiB) the kernel first printed nothing: the bootstrap page tables (8 pages) only
   cover a window that ends at or below 3 GiB with 4K pages. Patch 0045 raises them to 20 pages; the full userland
   then boots from the RAM disk to `USERLAND-SELFTEST-OK` with `hw.memsize` 3690987520. Not run: 8 GiB (the RAM above
   4 GiB is a second region, which the single contiguous window does not use) and 4 GiB from the SD card.

## Phase 1: first serial output on hardware (RAM-disk root)

The points where hardware differs from QEMU, in the order they will bite. Items 1 and 2 are unverified assumptions
until tried on a board.

1. **GIC security state (largest unknown).** xnu takes the timer as a FIQ in group 0. The stock firmware stub hands
   over in non-secure EL2, where group 0 is not reachable. Recommended: install our loader as the `armstub=` so it
   starts in EL3, and keep the kernel in secure EL1; that matches the register layout the driver already assumes.
   Fallback: move the timer to the IRQ path in the kernel.
2. **GPU memory hole.** The firmware reserves the top of the first 1 GiB for the VideoCore. The kernel's RAM window
   currently runs contiguously from `0x10000000`, so it would overlap. For the first boot, cap the window below the
   hole (about 690 MiB); later, read the real memory map from the firmware.
3. **Mini UART baud.** The driver assumes a 250 MHz core clock. Set `enable_uart=1` and verify the divisor on the
   wire.
4. **RNG.** Build the loader with `LOADER_CFLAGS=-DUSE_RNG200` instead of the counter seed.
5. **Watchdog and boot-args.** Drop `wdt=-1` once timing is real; keep `cpus=1`.

Done when the serial console shows the same `rc:` and `semtest:` lines as the CI boot.

## Phase 2: root on the SD card

- The hardware slot is EMMC2 at `0xfe340000` (`sdbase=` boot-arg), not QEMU's EMMC at `0xfe300000`. Expect clock
  and card-power work through the firmware mailbox.
- Switch the root from the RAM disk to `rd=sdroot0`, then mount read-write and run `fsck_hfs`.

## Phase 3: make it usable

- **HDMI text console:** ask the firmware for a framebuffer over the mailbox and pass it in `boot_args`. Cheap, and
  removes the need for a serial cable.
- **Peripheral interrupts:** confirm the system timer shows 5/5 from hardware, then make the UART interrupt-driven.
- **Missing commands:** `reboot`/`halt` (needs the watchdog reset), `login`/`getty`, `hostinfo`, `arch`.
- **SMP:** bring up secondary CPUs and IPIs through the GIC.

## Phase 4: larger drivers (each its own project)

- Ethernet (GENET), which would put the DNS resolver to real use.
- USB, which on the Pi 4 means PCIe plus the VL805 xHCI controller.
- 4 GiB and 8 GiB memory layouts.

## Hardware needed

A Pi 4, a 3.3 V USB serial adapter on GPIO 14/15, and an SD card. Phases 1-3 can be done with those alone.
