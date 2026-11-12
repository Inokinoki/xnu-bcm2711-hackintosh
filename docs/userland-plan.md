# macOS 13.5 userland for the BCM2711 kernel: what to build, in which order

Kernel: xnu-8796.141.3 (macOS 13.5), plain arm64 (no PAC), Cortex-A72 (ARMv8.0), 4K kernel pages with 16K user pages.
Component tags below come from Apple's manifest
`apple-oss-distributions/distribution-macOS`, tag `macos-135` (`release.json`). Pin them from that file, do not guess.

## 0. Constraints that shape everything

1. **Real macOS binaries cannot be reused.** On Apple Silicon every system binary is arm64e and lives in the dyld shared
   cache. arm64e uses `retaa`/`braa`/`ldraa`/`aut*`, which are UNDEFINED on ARMv8.0 (the A72 has no FEAT_PAuth). So
   everything is rebuilt from source as plain `arm64`.
2. **Not everything is open.** Missing from the 13.5 manifest: `launchd`, `libxpc`, `Libm`, `corecrypto`, `cctools/ld64`,
   `compiler-rt`, `libc++/libc++abi` (only upstream LLVM), and the closed `libsystem_*` pieces (trace, sandbox, featureflags,
   containermanager, coreservices, symptoms, networkextension, darwin, quarantine). These need a stub, an upstream
   replacement or an older open release (section 3).
3. **No shared cache.** Since macOS 11 `/usr/lib/libSystem.B.dylib` is not on disk. We build our own libSystem and its
   `/usr/lib/system/*.dylib` and put them on the root. dyld-1066 runs without a cache if every dylib exists on disk.
4. **No compiler on this host** (no clang/lld/aarch64 gcc). Build on the macOS runner like the kernel
   (`macos-14`, Xcode 15, `mig` from `bootstrap_cmds`), then pack the image on Linux with `tools/mkhfsroot.py`.

## 1. Compile flags (all userland)

```
-target arm64-apple-macos13.0 -mcpu=cortex-a72 -O2 -g      # NOT arm64e, no -fptrauth-*, no -mcpu=apple-*
-isysroot $(xcrun --sdk macosx --show-sdk-path)             # headers only; runtime libs are ours
LDFLAGS: -arch arm64 -Wl,-platform_version,macos,13.0,13.0  # ld64 aligns segments to 16K by default; keep it
         -nostdlib / -Wl,-no_adhoc_codesign is NOT used: keep the linker's ad-hoc signature, or `codesign -s -`
```

- `-mcpu=cortex-a72` is mandatory: the default Apple CPU emits LSE atomics (`ldadd`, `cas`, `swp`) which trap on the A72
  (the kernel CI already needed this for libfirehose_kernel).
- Gate for every Mach-O, as a script `tools/check-arm64-plain.sh`: `lipo -archs` = `arm64` only; `otool -hv` cpusubtype
  `ALL` (not `PAC_ABI`/`ARM64E`, i.e. not 0x80000002); `otool -tv` has none of
  `pac* aut* bra* blra* retaa retab ldra* ldadd* ldclr* ldset* ldeor* swp* cas*`; `otool -L` lists only our own paths;
  code signature present.

## 2. What the boot needs, by layer

### 2a. Executables

| Path | Source (tag) | Notes |
|---|---|---|
| `/usr/lib/dyld` | `dyld-1066.10` | builds with Xcode; the kernel execs it for every dynamic binary |
| `/sbin/launchd` (PID 1) | closed in 13.5 | see section 4 |
| `/bin/sh`, `/bin/bash` | `bash-131` (3.2) | `sh` = bash in POSIX mode; `zsh-97`/`tcsh-70` optional |
| `/bin/ls cp mv rm mkdir ln chmod ...` | `file_cmds-403.100.6` | |
| `/bin/cat`, grep, sed, wc, sort | `text_cmds-154`, `basic_cmds-67` | |
| `/bin/ps`, `/usr/bin/…` | `adv_cmds-205`, `shell_cmds-279.120.2` | `shell_cmds`: echo, test, kill, sleep, env |
| `login`, `getty`, `reboot`, `sysctl`, `dmesg` | `system_cmds-953.140.2` | needs OpenPAM for login; skip login first |
| `mount`, `mount_hfs`, `fsck_hfs`, `newfs_hfs` | `diskdev_cmds-701.100.2`, `hfs-627.100.6` | needed once the root moves to SD |
| `ifconfig`, `ping`, `netstat` | `network_cmds-644.140.2` | later |
| `top`, `less`, `vim`, `awk`, patch | `top-137`, `less-44`, `vim-146`, `awk-35`, `patch_cmds-45.120.3` | convenience |
| `mig` (build-time, host) | `bootstrap_cmds-129` | already in Xcode on the runner |

### 2b. Libraries, in dependency order (this is also the build order)

`libSystem.B.dylib` is only an umbrella that re-exports the `/usr/lib/system/*.dylib` pieces.

| # | Library | Source | Status |
|---|---|---|---|
| 1 | `libsystem_kernel` | `libsyscall/` inside xnu-8796.141.3 | open; needs MIG output and xnu's fakeroot headers from the kernel build |
| 2 | `libsystem_platform` | `libplatform-292.120.1.0.1` | open; reads commpage (patch 0010 already provides the 4K-kernel commpage) |
| 3 | `libsystem_pthread` | `libpthread-514.100.2` | open; patch 0024 already stages the kernel half of the same tag |
| 4 | `libsystem_blocks` | `libclosure-87` | open |
| 5 | `libcompiler_rt` | upstream LLVM `compiler-rt/builtins` | open, built `-mcpu=cortex-a72` |
| 6 | `libsystem_c` | `Libc-1534.100.14` | open; large; pulls assumes/os_log, so needs stubs below |
| 7 | `libsystem_malloc` | `libmalloc-425.100.7` | open; needs a featureflags stub |
| 8 | `libunwind`, `libdyld`, `libkeymgr` | libunwind and libdyld are in `dyld-1066.10`; `keymgr-31` | open |
| 9 | `libdispatch` | `libdispatch-1415.140.2` | open; needs kernel workqueue syscalls (builtin pthread, check) |
| 10 | `libsystem_info`, `libsystem_notify` | `Libinfo-564.120.2`, `Libnotify-312.140.2` | open; notify needs `notifyd` or a stub client |
| 11 | `libsystem_m` | **Libm is not in the manifest** | use the last public Libm, or FreeBSD `msun` |
| 12 | `libcopyfile`, `libremovefile`, `libcache`, `libsystem_dnssd` | `copyfile-191.100.3`, `removefile-68`, (cache), `mDNSResponder-1807.140.16` | open; only what the first commands link |
| 13 | closed `libsystem_*` | trace, featureflags, sandbox, containermanager, coreservices, symptoms, networkextension, darwin, quarantine, `libxpc` | **write stubs** (return ENOSYS or no-op); each exports only the symbols libSystem re-exports |
| 14 | `libSystem.B.dylib` | `Libsystem-1319.100.3` (umbrella, reexport list) | open; trim the re-export list to what exists |
| 15 | `libc++.1.dylib`, `libc++abi.dylib` | upstream LLVM runtimes | needed by anything C++ (including dyld tools) |
| 16 | `libz`, `libncurses`, `libedit`, `libiconv`, `libresolv`, `libutil`, `libxml2`, `libffi` | `zlib-83`, `ncurses-61.140.2`, `libedit-57`, `libiconv-64`, `libresolv-74`, `libutil-64`, `libxml2-36.26`, `libffi-30` | open; per command needs |
| later | `libobjc.A.dylib`, CoreFoundation | `objc4-876`, CF | not needed for command-line tools; defer |

Also needed on disk: `/dev`, `/private/var/{tmp,run}`, `/private/etc/{passwd,master.passwd,group}`, and
`/System/Library/CoreServices/SystemVersion.plist` for `sw_vers`. HFS+ root as today (`tools/mkhfsroot.py`).

## 3. Closed pieces: decisions

- **Stubs** for the closed `libsystem_*`: generate from the SDK's `.tbd` export list, keep only the symbols that
  libSystem or Libc import, implement them as no-ops. This is the largest unknown; expect iteration against dyld's
  "symbol not found" errors.
- **libm**: pick before step 11, because Libc and nearly every command import it.
- **corecrypto**: Libc only needs it for a few hashes and `arc4random`; use BearSSL (already staged for the kernel by
  patch 0025) or the portable code in Libc.

## 4. launchd (the one real fork)

`launchd` has not been open since 10.9. Options:

1. **Own minimal PID 1** (recommended first): static arm64 C program using only libsyscall, run an rc script, spawn `sh`.
   Fast, no libxpc dependency. The current test init (`tools/pi-init.S`) is the starting point; the sibling project
   `hackios-xnu-userland/userland/src/init.c` shows a working shape.
2. **launchd-842.92.1** (macOS 10.9, last open): port onto 13.5 libSystem; needs `liblaunch`/`vproc` and a bootstrap
   server, i.e. a real `libxpc` replacement. Do it only after step 1 boots to a shell.

## 5. Stages, each with a boot test on QEMU `raspi4b`

| Stage | Deliverable | Pass criterion |
|---|---|---|
| A | CI job `userland-arm64` (macos-14): fetch tags from `release.json`, build, run the gate script, upload `userland-root.tar` | all Mach-O pass section 1's gate |
| B | static C programs on `libsystem_kernel.a` + a tiny libc (no dyld) | C PID 1 prints and reaps children; 16K pages OK |
| C | libs 1-8 + `dyld` + umbrella `libSystem`, `hello` dynamically linked | `hello` prints via `puts`; dyld loads with no shared cache |
| D | libs 9-16 + `bash`, `file_cmds`, `text_cmds`, `shell_cmds` | interactive `sh` on the mini UART, `ls`, `cat`, `ps` work |
| E | `diskdev_cmds`/`hfs`, SD root (`rd=sdroot0`) | `mount` works, root on block storage, no RAM disk size limit |
| F | launchd route 2, `login`, OpenSSH, `network_cmds` | optional |

Stage A and B can start at once (B needs only the xnu `libsyscall` output from the existing kernel CI). From C on it
is strictly sequential: expect most of the time to go into libSystem initialisation (`libSystem_initializer` calls
`__libkernel_init`, `__libplatform_init`, `__pthread_init`, `_libc_initializer`, `dispatch_init` in that order) and
into the stubs.

## 5a. Build status (CI `userland-arm64`, macos-14, Xcode 15.4)

Built as plain arm64 for the Cortex-A72 and accepted by `tools/check-arm64-plain.sh`:
`libsystem_kernel` (dylib and static), `libsystem_platform`, `libsystem_pthread`, `libsystem_blocks`, `libkeymgr`,
`libcompiler_rt`, `libsystem_c`, `libsystem_malloc`, `/usr/lib/dyld`, `libdyld.dylib`, `libdispatch` (C object model, no ObjC runtime; linked from the static archive
target by `userland/build-libdispatch.sh`) the stand-in `libsystem_trace`, `libunwind`, `libc++abi` and `libc++` (upstream LLVM 17.0.6 runtimes, `userland/build-llvm-runtimes.sh`;
libunwind has the PAC authenticate hints patched out) and the `libSystem.B.dylib` umbrella (`userland/build-libsystem.sh`,
re-exports the libraries above), plus `libsystem_info` (Libinfo with the OpenDirectory and mDNSResponder modules replaced by
stand-ins in `userland/src/libinfo/`: files-only lookups), `libsystem_notify` (client library only), `libsystem_m` (openlibm;
the long double entry points are the double ones) and a `libxpc` stand-in (bootstrap look-ups fail, xpc objects are no-ops).
Stand-ins also for `libsystem_asl` (syslog family) and `libcommonCrypto` (digests, HMAC, PBKDF2, AES CCCrypt; see the status table), and
`libcopyfile`, `libremovefile`, `libutil` are built from source.
Programs (all plain arm64, linked against our libSystem): `/sbin/launchd` (our minimal PID 1, `userland/src/init/init.c`:
console, `/etc/rc`, a respawning `/bin/sh`), `bash` 3.2, shell_cmds (echo test sleep env kill pwd true false uname hostname id
date printenv basename dirname which), file_cmds (ls cp mv rm mkdir rmdir ln chmod touch df stat dd), text_cmds (cat sed wc
head tail sort tr cut grep). `grep` links `-lz -lbz2 -llzma`, built by `userland/build-compress-libs.sh`: zlib-83 and bzip2-45
from Apple's sources (bzip2's PMULL `crc32_vec` assembly is replaced by `userland/src/bz2_crc32_vec.c`, the Cortex-A72 has no
crypto extension) and upstream xz 5.4.6 (pinned by hash). All of this runs on QEMU (section 5c).
Next, in order: a trimmed root (no dSYM, headers or static libraries; symlinks such as `libc++.dylib` need the HFS root tool
to support them), `/etc` files, QEMU boot of `/sbin/launchd` and `sh`; then the remaining closed-library stubs (asl, featureflags, sandbox, corecrypto, macho ... the umbrella grows
with them), then init/launchd (a real bootstrap server replaces the libxpc stand-in) and the commands.

What the build needed (all in `userland/`, reuse it for the next libraries):

- Headers come from `dxb/fakeroot` (xnu 13.5 with our kernel patches, plus Libc, libmalloc, libpthread, dyld private
  headers), searched before the runner's SDK; `lib-env.sh` has the order. Xcode 15.x, not 16 (bridgeos availability macros).
- Xcode lacks `BSD.xcconfig`: the includes are stripped. clang 15 turned old-code warnings into errors: `COMPAT_FLAGS`.
- Stand-ins for what the open source lacks: `include/os/alloc_once_private.h`, `include/pthread_shim.h`,
  `src/thread_self_restrict_stub.c` (APRR, reports unsupported).
- libplatform has no Xcode project in the 13.5 tag: `build-libplatform.sh` compiles `src/` directly.
- C++ projects need `CXXSTD=1` (libc++ headers before the C library headers).
- More stand-ins in `include/` (`os/feature_private.h`, `os/log_private.h`, `apfs/apfs_fsctl.h`, `corecrypto/ccrng.h` on
  getentropy, `dyld_version_shim.h`) and `src/` (`libplatform_bits.c` ffs/fls, `chkstk_darwin.s`, `stubs/libsystem_trace.c`
  built by `build-stub-lib.sh`). `patch-libmalloc.sh` and `patch-libc.sh` fix source-level gaps (nano_zone.h on arm64,
  dyld macros, cancelable variants must clear `__DARWIN_NON_CANCELABLE`).
- libmalloc needs `-DOS_VARIANT_RESOLVED=1 -DOS_VARIANT_NOTRESOLVED=1` (defined by Apple's build, not in the sources);
  libraries link against our own already-built libraries first (`LIBRARY_SEARCH_PATHS`), so build order matters.
- compiler-rt: Xcode's `libclang_rt.osx.a` wrapped into a dylib, plus our `___chkstk_darwin`.

## 6. Known risks to check early

- 16K user pages on a 4K kernel: real dyld and libSystem are still untested (README); dyld maps with 16K segments.
- The kernel must accept plain-arm64 Mach-O from an ad-hoc signature with no entitlements (built-in AMFI, patch 0034);
  boot-args `cs_enforcement_disable=1` / `amfi_get_out_of_my_way=1` may be needed.
- `mach_absolute_time` reads the counter at EL0 and the commpage; confirm `CNTKCTL_EL1` and the commpage fields libplatform uses.
- The kernel does not set up the shared region for a cacheless dyld the way macOS does; check `DYLD_SHARED_REGION=avoid`
  handling and the `apple[]` strings the kernel passes.

## 5b. Stand-ins: what each one is, and what is left

`tools/macho_undef.py ROOT --fail` (CI step "Undefined symbols and missing libraries") checks that every non-weak
undefined symbol of every Mach-O in the root is exported by the library it is bound to (two-level namespace, re-exports
followed) and that every library an image names exists. It found the libraries dyld would have failed to load
(libmacho, libcorecrypto, libsystem_featureflags, libsystem_dnssd, libquarantine, libsystem_collections, libsystem_darwin,
libunwind, libobjc, libncurses) and the symbols libSystem imports from them; the root now has no unresolved symbol.
New work comes from this report, not from guessing.

| Library | State | Notes |
|---|---|---|
| libmacho, libsystem_collections, libc `fenv` (libm_extra.c) | real | libmacho written from loader.h; collections from Libc |
| libcommonCrypto | working subset | MD5, SHA-1/224/256/384/512, HMAC, PBKDF2, AES `CCCrypt` (CBC/ECB, PKCS#7), `CCRandomGenerateBytes` on the pinned BearSSL (`userland/fetch-bearssl.sh`, same files as kernel patch 0025); known-answer tests run in the QEMU boot. Not done: CCCryptor streaming API, other ciphers, SHA-3 |
| libsystem_trace, libsystem_asl | no-op | with `OS_LOG_STDERR=1` in the environment the messages go to stderr |
| libxpc, liblaunch (`bootstrap_*`, `vproc_swap_integer`) | working subset | init (`/sbin/launchd`) is the bootstrap server (`userland/include/bootstrap_proto.h`: check_in / look_up / register over one Mach port, children inherit the port); xpc event publishers are static stand-ins that only deliver the initial barrier |
| notifyd (`Libnotify-312.140.2`) | real | built from source (osascript shim for its admin-password script phase, `libbsm.0` stand-in, sandbox constants force-included), started by init after the bootstrap server; QEMU boot checks `notify_post` from one process seen by `notify_check` in another (libnotify disables itself in a fork child, so the test execs) |
| libsystem_darwin (`os_variant_*`), libsystem_featureflags, libcorecrypto, libsystem_dnssd, libquarantine | constant / empty | correct for this system (not an internal build, no flags, no quarantine); libraries only have to exist |
| libncurses.5.4 | termcap stub | `tgetent` fails; real ncurses-61 when a command needs curses |
| Libinfo membership, ds modules | ENOENT / unavailable | files-only lookups |
| Libinfo mdns module | UDP stub resolver | `userland/src/libinfo/mdns_module.c`: A/AAAA over UDP to the `/etc/resolv.conf` nameservers, no search list/TCP/PTR; QEMU boot checks `getaddrinfo` against a loopback DNS server |
| libobjc | none | Blocks is built with `HAVE_OBJC=0`, libdispatch with `HAVE_OBJC 0` |

## 5c. Boot on QEMU: the userland runs (workflow `userland-boot`, macos-14, QEMU 11, RAM disk root)

`tools/mkroot.sh` (hdiutil, macOS only) turns the CI root into a raw HFS+ image; `tools/pi4-qemu-boot.sh` boots it with the
kernel of the kernel workflow; `-d int` output is filtered into `el0-exceptions.log`. **Result (run 41894470283): the
kernel execs our `/sbin/launchd`, dyld loads libSystem and its 30 dylibs, libSystem_initializer runs, init remounts `/`
read-write, runs `/etc/rc` with bash 3.2, and the selftest prints `USERLAND-SELFTEST-OK`** (uname says
`Darwin 22.6.0 ... arm64`, `id`, `ls`, pipes, `cp`/`rm`/`cat` on `/tmp` all work); an interactive `sh` is left on the console.

What it took, in order: the missing dependency libraries (found by `tools/macho_undef.py`), Apple's `Libsystem/init.c` linked
into libSystem.B.dylib (without `libSystem_initializer` no library is initialized), and libmalloc without the nano zone
(nano v2 reserves 0x600000000000.., outside this kernel's user address space: the first crashes were a translation fault there,
and munmap of that range panics the kernel in `pmap_remove_options`). The patch for that had never run for two builds
(its guard grep matched the 32-bit branch's `CONFIG_NANOZONE 0`), which is what the debugging harness found:
`tools/pi-spawn.S`, a static PID 1 that runs `smoke` and the real init as children with `DYLD_PRINT_LIBRARIES/INITIALIZERS/
SEGMENTS=1` (workflow input `spawn`), so a crash leaves the dyld log instead of a kernel panic.

Fixed in the kernel (`xnu/patches-8796.141.3/0042`): the macOS-platform build gave user vm_maps a 47-bit maximum while the
4K-page user pmap covers 39 bits, so a `munmap` above 512 GiB reached `pmap_remove_options()` and panicked. The default
vm_map maximum is now clamped to the pmap's end minus 256 MB. `semtest vm` (run from `/etc/rc`, checked by the boot job)
probes 0x600000000000: `mmap(MAP_FIXED)` fails, `vm_allocate(FIXED)` returns KERN_INVALID_ADDRESS, `munmap` returns 0, no panic.
With `LIBMALLOC_NANO=1` the nano zone was tried (boot run 42021731524): it fails to reserve, malloc falls back and everything
passes, but each process prints "nano zone abandoned" on stderr, so `patch-libmalloc.sh` keeps nano off by default.
Nano v2 itself can not work here: it needs 0x600000000000, past the 39-bit user pmap.

Fixed after the first boot:

- `getpwuid(0)` failed ("I have no name!"): as root Libinfo's file module reads `/etc/master.passwd`, which has 10 fields
  (name:passwd:uid:gid:class:change:expire:gecos:dir:shell); ours had the 7 of `/etc/passwd`. The prompt's `PS1` was
  applied all along (its `\h` was empty because no hostname was set; `/etc/rc` now runs `hostname macpi`).
- `sort` died with exit 255 and no output on every input. Its crash handler hides the signal, so `userland/patch-sort.sh`
  makes it print signal, address and symbol. Cause: clang compiles `memset(p, 0, n); return p;` (`sort_list_item_alloc`) to a
  tail call to `bzero` and returns whatever `bzero` leaves in `x0`. Apple's closed arm64 `bzero` leaves the destination there;
  libplatform's generic `bzero` is `void` and left the end of its helper's loop (`p + 12` for 16 bytes). Every `sort` item
  therefore pointed into the next heap block and the following write clobbered its key pointer.
  `userland/patch-libplatform.sh` makes `bzero`, `__bzero` and `_platform_bzero` return their argument. Any other code
  that clang compiles the same way was affected too.

`grep` is built and checked at boot (`rc: grep ok`); `-Z/-J/-M` decompression is linked but not exercised (no compressors in the root).

`/dev` is the kernel's devfs, as on macOS: null, zero, tty, stdout, random, urandom work (checked at boot), creating regular
files there fails with ENOTSUP by design. That was never a fault. `mount` is missing (it is in `system_cmds`).

`system_cmds` (dmesg sysctl sync pagesize getconf vm_stat mkfile nologin) and `diskdev_cmds` (mount umount mount_devfs mount_fdesc
fuser) are built and run at boot (`sysctl kern.ostype` prints Darwin, `mount` lists `/dev/md0 on / (hfs)` and devfs). Notes:

- `diskdev_cmds` links APFS, MediaKit, CoreFoundation and IOKit frameworks that this system does not have; nothing on the macOS
  path of mount/umount uses them, so `userland/patch-diskdev.sh` drops them from the link phases. mount gets stand-ins for
  libdarwin's closed `os/errno.h`, `os/bsd.h` and `sysexit_np` (`userland/include/os`, `userland/include/APFS`,
  `userland/src/sysexit_np.c`).
- Not built because they need closed parts: `reboot`/`halt` (IOKit kext MIG), `login`/`getty` (OpenDirectory, SoftLinking),
  `hostinfo` (`slot_name`), `arch` (`NXGetArchInfo`, `sysdir`).
- CI moved to `macos-15`/Xcode 16.4 (the `macos-14` runners no longer exist and no hosted image has Xcode 15). Needed fixes:
  Libc's link-list script phase declares its outputs (`patch-libc.sh`), a hidden `wmemchr` for the dyld executable
  (`userland/src/dyld_wmemchr.c`), zlib includes stdio.h first. The kernel workflow is on `macos-15` too: Xcode 16's
  `iig` emits `rpc.kernelContent`, which patch 0043 adds to `IORPC` (kernel run 42269023162 built; boot run 42271770237 passed
  with that kernel).

Known gaps: section 5b lists what is still a stand-in.
