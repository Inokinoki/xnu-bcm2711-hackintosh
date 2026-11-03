#!/bin/sh
# libSystem.B.dylib: the umbrella that re-exports every library of the C runtime, as on macOS. Re-exports what
# exists in OUTROOT/usr/lib/system (and libunwind in OUTROOT/usr/lib when it is built), so it can be run once
# early (STAGE mode: output to a build directory, for linking the LLVM runtimes against) and again at the end.
#
#   [LIBSYSTEM_DXB=DXBDIR LIBSYSTEM_SRC=SRCDIR] userland/build-libsystem.sh OUTROOT [DESTDIR]    # default DESTDIR OUTROOT/usr/lib (gets libSystem.B.dylib + libSystem.dylib link)
set -eu
mkdir -p "${1:?usage: $0 OUTROOT [DESTDIR]}"
OUT=$(cd "$1" && pwd)
DEST=${2:-$OUT/usr/lib}
mkdir -p "$DEST"
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}

RE=
for l in libsystem_kernel libsystem_platform libsystem_pthread libsystem_c libsystem_malloc libsystem_blocks \
	libsystem_trace libsystem_m libxpc libsystem_asl libcommonCrypto libcopyfile libremovefile libsystem_info libsystem_notify libsystem_collections libsystem_darwin libsystem_configuration libmacho libcorecrypto libsystem_featureflags libsystem_dnssd libquarantine libcompiler_rt libkeymgr libdyld libdispatch libunwind; do
	f=$OUT/usr/lib/system/$l.dylib
	[ -f "$f" ] && RE="$RE -Wl,-reexport_library,$f"
done

# libSystem_initializer (Libsystem-1319.100.3/init.c): the constructor that initializes every library in the
# right order (libkernel, libplatform, pthread, libc, malloc, keymgr, dyld, dispatch ...). Without it none of
# them is set up when main runs. Needs the header roots, so only when LIBSYSTEM_DXB and LIBSYSTEM_SRC are set.
INIT_OBJ=
if [ -n "${LIBSYSTEM_DXB:-}" ] && [ -n "${LIBSYSTEM_SRC:-}" ]; then
	HERE=$(cd "$(dirname "$0")" && pwd)
	DXB=$(cd "$LIBSYSTEM_DXB" && pwd)
	SRC=$(cd "$LIBSYSTEM_SRC" && pwd)
	OUTROOT=$OUT
	. "$HERE/lib-env.sh"
	SYSINC=
	for d in $HDR_PATHS "$SRC/Libc/darwin" "$SRC/xnu/libsyscall/wrappers" "$SRC/libmalloc/private" "$SRC/xnu/EXTERNAL_HEADERS"; do
		SYSINC="$SYSINC -isystem $d"
	done
	INIT_OBJ=$DXB/build/libsystem_init.o
	# shellcheck disable=SC2086
	xcrun clang -target arm64-apple-macos13.0 $CPU_FLAGS -Os -isysroot "$SDK" -w $BRIDGEOS_DEF $BRIDGEOS_UNAVAIL $COMPAT_FLAGS \
		$SYSINC -c "$SRC/Libsystem/init.c" -o "$INIT_OBJ"
fi

# shellcheck disable=SC2086
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/libSystem.B.dylib -compatibility_version 1.0.0 -current_version 1319.100.3 \
	$RE $INIT_OBJ -o "$DEST/libSystem.B.dylib"
ln -sf libSystem.B.dylib "$DEST/libSystem.dylib"
echo "libSystem.B.dylib re-exports:$RE" | tr ' ' '\n' | grep -c reexport_library
