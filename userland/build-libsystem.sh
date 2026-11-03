#!/bin/sh
# libSystem.B.dylib: the umbrella that re-exports every library of the C runtime, as on macOS. Re-exports what
# exists in OUTROOT/usr/lib/system (and libunwind in OUTROOT/usr/lib when it is built), so it can be run once
# early (STAGE mode: output to a build directory, for linking the LLVM runtimes against) and again at the end.
#
#   userland/build-libsystem.sh OUTROOT [DESTDIR]    # default DESTDIR OUTROOT/usr/lib (gets libSystem.B.dylib + libSystem.dylib link)
set -eu
mkdir -p "${1:?usage: $0 OUTROOT [DESTDIR]}"
OUT=$(cd "$1" && pwd)
DEST=${2:-$OUT/usr/lib}
mkdir -p "$DEST"
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}

RE=
for l in libsystem_kernel libsystem_platform libsystem_pthread libsystem_c libsystem_malloc libsystem_blocks \
	libsystem_trace libcompiler_rt libkeymgr libdyld libdispatch; do
	f=$OUT/usr/lib/system/$l.dylib
	[ -f "$f" ] && RE="$RE -Wl,-reexport_library,$f"
done
[ -f "$OUT/usr/lib/libunwind.1.dylib" ] && RE="$RE -Wl,-reexport_library,$OUT/usr/lib/libunwind.1.dylib"

# shellcheck disable=SC2086
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/libSystem.B.dylib -compatibility_version 1.0.0 -current_version 1319.100.3 \
	$RE -o "$DEST/libSystem.B.dylib"
ln -sf libSystem.B.dylib "$DEST/libSystem.dylib"
echo "libSystem.B.dylib re-exports:$RE" | tr ' ' '\n' | grep -c reexport_library
