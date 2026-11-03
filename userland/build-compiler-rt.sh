#!/bin/sh
# libcompiler_rt.dylib: the toolchain's compiler-rt builtins (libclang_rt.osx.a, arm64 slice) wrapped
# into the dylib the other libraries link as -lcompiler_rt. The gate checks that the builtins carry no
# LSE or PAC code (Xcode's are generic arm64).
#
#   userland/build-compiler-rt.sh OUTROOT
set -eu
mkdir -p "${1:?usage: $0 OUTROOT}/usr/lib/system"
OUTROOT=$(cd "$1" && pwd)
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
A=$(xcrun clang -print-resource-dir)/lib/darwin/libclang_rt.osx.a
test -f "$A"
xcrun clang -target arm64-apple-macos13.0 -isysroot "$SDK" -dynamiclib -nostdlib \
	-install_name /usr/lib/system/libcompiler_rt.dylib -umbrella System \
	-compatibility_version 1 -current_version 1 \
	-Wl,-all_load "$A" -o "$OUTROOT/usr/lib/system/libcompiler_rt.dylib"
