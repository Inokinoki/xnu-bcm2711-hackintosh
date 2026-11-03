#!/bin/sh
# libdispatch.dylib. The project's own dylib target needs Apple-internal scripts (install-plists.sh) and an empty
# product name without them, so build its static archive target (libdispatch_mp_static) and link the dylib here.
#
#   userland/build-libdispatch.sh DXBDIR SRCDIR OUTROOT     # env as for build-xcode-lib.sh
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
DXB=$(cd "${1:?usage: $0 DXBDIR SRCDIR OUTROOT}" && pwd)
SRC=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}"
OUT=$(cd "$3" && pwd)
SDK=$(xcrun --sdk macosx --show-sdk-path)

"$HERE/patch-libdispatch.sh" "$SRC/libdispatch"
"$HERE/build-xcode-lib.sh" "$DXB" "$SRC" "$OUT" libdispatch "libdispatch mp static" \
	CLANG_CXX_LANGUAGE_STANDARD=gnu++17 GCC_ENABLE_CPP_EXCEPTIONS=NO

echo "== archives the target produced"
find "$OUT" "$DXB/build" -name 'libdispatch*.a' | sort
A=$(find "$OUT/usr/local/lib" -name 'lib*dispatch.a' | head -1)
test -f "$A"

xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot "$SDK" -c "$HERE/src/libdispatch_stubs.c" -o "$DXB/build/libdispatch_stubs.o"
mkdir -p "$OUT/usr/lib/system"
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/system/libdispatch.dylib -umbrella System \
	-compatibility_version 1 -current_version 1 \
	-L"$OUT/usr/lib/system" -L"$SDK/usr/lib/system" \
	-lsystem_kernel -lsystem_c -lsystem_pthread -lsystem_malloc -lsystem_platform -lsystem_blocks -lcompiler_rt -ldyld \
	-Wl,-all_load "$A" "$DXB/build/libdispatch_stubs.o" -o "$OUT/usr/lib/system/libdispatch.dylib"
