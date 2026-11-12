#!/bin/sh
# /sbin/launchd (our minimal PID 1) as plain arm64, linked against our libSystem.
#
#   userland/build-init.sh OUTROOT
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "${1:?usage: $0 OUTROOT}/sbin"
OUT=$(cd "$1" && pwd)
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot "$SDK" -Wall \
	-iquote "$HERE/include" -L"$OUT/usr/lib" "$HERE/src/init/init.c" -o "$OUT/sbin/launchd"
mkdir -p "$OUT/usr/bin"
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot "$SDK" -Wall \
	-iquote "$HERE/include" -L"$OUT/usr/lib" "$HERE/src/semtest.c" -o "$OUT/usr/bin/semtest"
