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
Stand-ins also for `libsystem_asl` (syslog family) and `libcommonCrypto` (SHA-256 only, checked against sha256sum), and
`libcopyfile`, `libremovefile`, `libutil` are built from source.
Programs (all plain arm64, linked against our libSystem): `/sbin/launchd` (our minimal PID 1, `userland/src/init/init.c`:
console, `/etc/rc`, a respawning `/bin/sh`), `bash` 3.2, shell_cmds (echo test sleep env kill pwd true false uname hostname id
date printenv basename dirname which), file_cmds (ls cp mv rm mkdir rmdir ln chmod touch df stat dd), text_cmds (cat sed wc
head tail sort tr cut). `grep` is missing: it needs liblzma. Not run on QEMU yet.
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
