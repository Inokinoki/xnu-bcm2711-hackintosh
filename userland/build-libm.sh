#!/bin/sh
# libsystem_m.dylib: Apple's Libm is closed, so use openlibm (FreeBSD msun based, builds for Darwin/aarch64) as a
# static archive and link it into the dylib the umbrella re-exports.
#
#   userland/build-libm.sh WORKDIR OUTROOT
set -eu
mkdir -p "${1:?usage: $0 WORKDIR OUTROOT}" "${2:?}/usr/lib/system"
WORK=$(cd "$1" && pwd)
OUT=$(cd "$2" && pwd)
TAG=${OPENLIBM_TAG:-v0.8.3}
SDK=$(xcrun --sdk macosx --show-sdk-path)

[ -d "$WORK/openlibm/.git" ] || git clone -q --depth 1 --branch "$TAG" https://github.com/JuliaMath/openlibm "$WORK/openlibm"
git -C "$WORK/openlibm" log -1 --format="openlibm %H"
make -C "$WORK/openlibm" -j4 libopenlibm.a ARCH=aarch64 OS=Darwin USECLANG=1 USEGCC=0 \
	CC="$(xcrun -f clang) -target arm64-apple-macos13.0 -mcpu=cortex-a72 -isysroot $SDK" AR="$(xcrun -f ar)"

HERE=$(cd "$(dirname "$0")" && pwd)
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot "$SDK" -I"$WORK/openlibm/include" -c "$HERE/src/libm_extra.c" -o "$WORK/libm_extra.o"
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/system/libsystem_m.dylib -umbrella System -compatibility_version 1 -current_version 3252 \
	-L"$OUT/usr/lib/system" -lsystem_kernel -lsystem_c -lcompiler_rt \
	-Wl,-all_load "$WORK/openlibm/libopenlibm.a" "$WORK/libm_extra.o" "$(xcrun clang -print-resource-dir)/lib/darwin/libclang_rt.osx.a" \
	-o "$OUT/usr/lib/system/libsystem_m.dylib"
