#!/bin/sh
# Build a stand-in for a closed libSystem component from userland/src/stubs/NAME.c: a dylib at
# /usr/lib/system/NAME.dylib exporting just the symbols the rest of libSystem imports from it.
# They link against the SDK's stub libraries (our own, when already built) for libc and friends.
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
# libraries of /usr/lib/system are parts of the System umbrella; others (libncurses) are linked directly
UMBRELLA=
LIBS=-lSystem
if [ "$DIR" = usr/lib/system ]; then UMBRELLA="-umbrella System"; LIBS="-lsystem_kernel -lsystem_c -ldyld -lsystem_blocks"; fi
# shellcheck disable=SC2086
xcrun clang -target arm64-apple-macos13.0 -mcpu=cortex-a72 -Os -g -isysroot "$SDK" -I"$HERE/include" \
	-Wno-error -dynamiclib -nostdlib \
	-install_name "/$DIR/$NAME.dylib" $UMBRELLA -L"$OUTROOT/usr/lib/system" -L"$SDK/usr/lib/system" $LIBS \
	-compatibility_version 1 -current_version 1 \
	"$HERE/src/stubs/$NAME.c" -o "$OUTROOT/$DIR/$NAME.dylib"
