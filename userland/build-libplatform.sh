#!/bin/sh
# libsystem_platform: libplatform has no Xcode project in the macOS 13.5 tag, so compile its arm64 sources
# by hand (what the per-directory xcconfigs do: all of src/ except the arm, i386 and x86_64 directories).
#
#   userland/build-libplatform.sh DXBDIR SRCDIR OUTROOT
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
DXB=$(cd "${1:?usage: $0 DXBDIR SRCDIR OUTROOT}" && pwd)
SRC=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}"
OUTROOT=$(cd "$3" && pwd)
. "$HERE/lib-env.sh"
P=$SRC/libplatform
OBJ=$DXB/build/libplatform.obj
rm -rf "$OBJ"
mkdir -p "$OBJ" "$OUTROOT/usr/lib/system"

SYSINC=
for d in $HDR_PATHS; do SYSINC="$SYSINC -isystem $d"; done
CFLAGS="-target arm64-apple-macos13.0 $CPU_FLAGS -Os -g -isysroot $SDK -w -fno-stack-protector -fdollars-in-identifiers
 -fno-common -momit-leaf-frame-pointer -D_FORTIFY_SOURCE=0 -DOSATOMIC_USE_INLINED=0 -DOSATOMIC_DEPRECATED=0
 -DOSSPINLOCK_USE_INLINED=0 -DOSSPINLOCK_DEPRECATED=0 -DOS_UNFAIR_LOCK_INLINE=0 $BRIDGEOS_DEF
 -I$P/private -I$P/include -I$P/internal -I$P/src/os/resolver -I$P/src/atomics $SYSINC"

n=0
for f in $(cd "$P" && find src -type f \( -name '*.c' -o -name '*.s' \) | sort); do
	case /$f in */arm/* | */i386/* | */x86_64/*) continue ;; esac
	n=$((n + 1))
	o=$OBJ/$n-$(basename "$f").o
	case $f in
	*.s) xcrun clang $CFLAGS -x assembler-with-cpp -c "$P/$f" -o "$o" ;;
	*) xcrun clang $CFLAGS -c "$P/$f" -o "$o" ;;
	esac
done
echo "compiled $n files"

# shellcheck disable=SC2086
xcrun clang -target arm64-apple-macos13.0 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/system/libsystem_platform.dylib -umbrella System \
	-compatibility_version 1 -current_version 1 \
	-Wl,-alias_list,"$P/xcodeconfig/libplatform.aliases" \
	-L"$OUTROOT/usr/lib/system" -lsystem_kernel \
	-all_load "$OBJ"/*.o -o "$OUTROOT/usr/lib/system/libsystem_platform.dylib"
test -f "$OUTROOT/usr/lib/system/libsystem_platform.dylib"
