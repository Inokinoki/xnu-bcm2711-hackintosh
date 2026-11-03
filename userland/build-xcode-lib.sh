#!/bin/sh
# Build one target of an Apple project's Xcode project as plain arm64 for the Cortex-A72 against the
# macOS 13.5 header roots, installing into OUTROOT.
#
#   userland/build-xcode-lib.sh DXBDIR SRCDIR OUTROOT PROJECT TARGET [SETTING=value...]
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
DXB=$(cd "${1:?usage: $0 DXBDIR SRCDIR OUTROOT PROJECT TARGET [SETTING=value...]}" && pwd)
SRC=$(cd "${2:?}" && pwd)
mkdir -p "${3:?}"
OUTROOT=$(cd "$3" && pwd)
PROJECT=${4:?}
TARGET=${5:?}
shift 5
. "$HERE/lib-env.sh"

# LIB_CFLAGS_VAR: build setting that carries our compiler flags (default OTHER_CFLAGS; Libc's variants set their
# cancelable/non-cancelable macros in OTHER_CFLAGS at target level, which a command-line value would reorder)
# LIB_EXTRA_SRC: a C file of stand-ins for symbols the closed parts of macOS provide, linked into the library
EXTRA_LD=
if [ -n "${LIB_EXTRA_SRC:-}" ]; then
	mkdir -p "$DXB/build/extra"
	extra_obj=$DXB/build/extra/$PROJECT-$(basename "$LIB_EXTRA_SRC").o
	xcrun clang -target arm64-apple-macos13.0 $CPU_FLAGS -Os -isysroot "$SDK" -c "$LIB_EXTRA_SRC" -o "$extra_obj"
	EXTRA_LD=$extra_obj
fi

# <System/sys/...> framework-style includes: the version directory has no Headers/PrivateHeaders/Current links
SF=$FAKEROOT_DIR/System/Library/Frameworks/System.framework
[ -e "$SF/Versions/Current" ] || ln -sfn B "$SF/Versions/Current"
for d in Headers PrivateHeaders; do [ -e "$SF/$d" ] || ln -sfn "Versions/Current/$d" "$SF/$d"; done

strip_bsd_xcconfig "$SRC/$PROJECT"
cd "$SRC/$PROJECT"
HDRS=
for d in $HDR_PATHS; do HDRS="$HDRS $d"; done
env LD="$(xcrun -find clang)" LDPLUSPLUS="$(xcrun -find clang++)" \
	xcodebuild install -sdk macosx -target "$TARGET" \
	ARCHS=arm64 VALID_ARCHS=arm64 ONLY_ACTIVE_ARCH=NO \
	MACOSX_DEPLOYMENT_TARGET=13.0 CODE_SIGN_IDENTITY=- GCC_TREAT_WARNINGS_AS_ERRORS=NO \
	LIBRARY_SEARCH_PATHS="$OUTROOT/usr/lib/system \$(inherited)" \
	OTHER_LDFLAGS="\$(inherited) $EXTRA_LD ${LIB_EXTRA_LDFLAGS:-}" \
	SUPPORTS_TEXT_BASED_API=NO BUILD_VARIANTS=normal \
	${LIB_CFLAGS_VAR:-OTHER_CFLAGS}="\$(inherited) $CPU_FLAGS $COMPAT_FLAGS ${LIB_EXTRA_CFLAGS:-} \"$BRIDGEOS_DEF\" $BRIDGEOS_UNAVAIL" \
	HEADER_SEARCH_PATHS="\$(inherited)" \
	SYSTEM_HEADER_SEARCH_PATHS="$HDRS" \
	SYSTEM_FRAMEWORK_SEARCH_PATHS="$FAKEROOT_DIR/System/Library/Frameworks" \
	OBJROOT="$DXB/build/$PROJECT-$TARGET.obj" SYMROOT="$DXB/build/$PROJECT-$TARGET.sym" \
	DSTROOT="$OUTROOT" FAKEROOT_DIR="$FAKEROOT_DIR" \
	RC_ProjectSourceVersion=1 RC_ProjectNameAndSourceVersion="$PROJECT-1" "$@"
