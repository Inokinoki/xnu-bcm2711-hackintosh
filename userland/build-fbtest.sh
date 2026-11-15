#!/bin/sh
# /usr/bin/fbtest and the IOKit-lite client code it uses, as plain arm64 against our libSystem.
#
#   userland/build-fbtest.sh OUTROOT SRCDIR
#
# The MIG user stubs come from the device.defs of the xnu we run (SRCDIR/xnu), so the message ids match the kernel.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "${1:?usage: $0 OUTROOT SRCDIR}/usr/bin"
OUT=$(cd "$1" && pwd)
SRC=$(cd "${2:?usage: $0 OUTROOT SRCDIR}" && pwd)
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
GEN=$OUT/../fbtest-gen
mkdir -p "$GEN"
GEN=$(cd "$GEN" && pwd)
(cd "$GEN" && xcrun mig -arch arm64 -isysroot "$SDK" -DIOKIT=1 -I"$SRC/xnu/osfmk" \
	-user iokitmig.c -header iokitmig.h -server /dev/null -sheader /dev/null "$SRC/xnu/osfmk/device/device.defs")
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -isysroot "$SDK" -Wall -Wno-unused-variable \
	-iquote "$HERE/include" -iquote "$HERE/src" -I"$GEN" -L"$OUT/usr/lib" \
	"$HERE/src/fbtest.c" "$HERE/src/iokit-lite/iokit_lite.c" "$GEN/iokitmig.c" -o "$OUT/usr/bin/fbtest"
