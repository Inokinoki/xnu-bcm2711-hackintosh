#!/bin/sh
# Build libsystem_kernel (xnu's libsyscall) as plain arm64 for the Cortex-A72, from the xnu tree and
# fakeroot that userland/dxb-headers.sh prepared.
#
#   userland/build-libsyscall.sh DXBDIR OUTROOT
#
# Installs OUTROOT/usr/lib/system/libsystem_kernel.dylib and OUTROOT/usr/local/lib/libsystem_kernel.a
# (dyld links the static library).
set -eu
DXB=$(cd "${1:?usage: $0 DXBDIR OUTROOT}" && pwd)
OUTROOT=${2:?usage: $0 DXBDIR OUTROOT}
mkdir -p "$OUTROOT"
OUTROOT=$(cd "$OUTROOT" && pwd)
FAKEROOT_DIR=$DXB/fakeroot
BUILD=$DXB/build

echo "== where xnu installed the mach .defs"
find "$FAKEROOT_DIR" -name "restartable*" -o -name "mach_port.defs" | sort

cd "$DXB/xnu/libsyscall"
env LD="$(xcrun -find clang)" LDPLUSPLUS="$(xcrun -find clang++)" \
	xcodebuild install -sdk macosx -target Libsyscall_dynamic \
	ARCHS=arm64 VALID_ARCHS=arm64 ONLY_ACTIVE_ARCH=NO \
	MACOSX_DEPLOYMENT_TARGET=13.0 CODE_SIGN_IDENTITY=- \
	OTHER_CFLAGS='$(inherited) -mcpu=cortex-a72' \
	OTHER_MIGFLAGS="-novouchers -I$FAKEROOT_DIR/System/Library/Frameworks/System.framework/PrivateHeaders -I$FAKEROOT_DIR/usr/local/include -I$FAKEROOT_DIR/usr/include -DKOBJECT_SERVER" \
	HEADER_SEARCH_PATHS='$(inherited) '"$FAKEROOT_DIR/usr/include $FAKEROOT_DIR/usr/local/include" \
	OBJROOT="$BUILD/libsyscall-arm64.obj" SYMROOT="$BUILD/libsyscall-arm64.sym" \
	DSTROOT="$OUTROOT" FAKEROOT_DIR="$FAKEROOT_DIR" \
	RC_ProjectSourceVersion=1 RC_ProjectNameAndSourceVersion=Libsyscall-1

find "$OUTROOT" -name 'libsystem_kernel*'
test -f "$OUTROOT/usr/lib/system/libsystem_kernel.dylib"
mkdir -p "$OUTROOT/usr/local/lib"
cp "$BUILD/libsyscall-arm64.sym/Release/libsystem_kernel.a" "$OUTROOT/usr/local/lib/" 2>/dev/null ||
	cp "$(find "$BUILD/libsyscall-arm64.sym" "$BUILD/libsyscall-arm64.obj" -name 'libsystem_kernel.a' | head -1)" "$OUTROOT/usr/local/lib/"
