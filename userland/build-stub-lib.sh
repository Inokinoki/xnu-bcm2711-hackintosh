#!/bin/sh
# Build a stand-in for a closed libSystem component from userland/src/stubs/NAME.c: a dylib at
# /usr/lib/system/NAME.dylib exporting just the symbols the rest of libSystem imports from it.
# Undefined symbols (libc and friends) are bound at run time, so the stubs link in any order.
#
#   userland/build-stub-lib.sh OUTROOT NAME [INSTALL_DIR]      # INSTALL_DIR defaults to usr/lib/system
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
mkdir -p "${1:?usage: $0 OUTROOT NAME [INSTALL_DIR]}"
OUTROOT=$(cd "$1" && pwd)
NAME=${2:?}
DIR=${3:-usr/lib/system}
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
mkdir -p "$OUTROOT/$DIR"
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -g -isysroot "$SDK" -I"$HERE/include" \
	-Wno-error -dynamiclib -nostdlib -Wl,-undefined,dynamic_lookup \
	-install_name "/$DIR/$NAME.dylib" -umbrella System -L"$OUTROOT/usr/lib/system" -L"$SDK/usr/lib/system" -lsystem_kernel -ldyld \
	-compatibility_version 1 -current_version 1 \
	"$HERE/src/stubs/$NAME.c" -o "$OUTROOT/$DIR/$NAME.dylib"
