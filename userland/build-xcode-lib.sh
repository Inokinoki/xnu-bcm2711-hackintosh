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

strip_bsd_xcconfig "$SRC/$PROJECT"
cd "$SRC/$PROJECT"
HDRS=
for d in $HDR_PATHS; do HDRS="$HDRS $d"; done
env LD="$(xcrun -find clang)" LDPLUSPLUS="$(xcrun -find clang++)" \
	xcodebuild install -sdk macosx -target "$TARGET" \
	ARCHS=arm64 VALID_ARCHS=arm64 ONLY_ACTIVE_ARCH=NO \
	MACOSX_DEPLOYMENT_TARGET=13.0 CODE_SIGN_IDENTITY=- GCC_TREAT_WARNINGS_AS_ERRORS=NO \
	SUPPORTS_TEXT_BASED_API=NO BUILD_VARIANTS=normal \
	OTHER_CFLAGS="\$(inherited) $CPU_FLAGS $COMPAT_FLAGS \"$BRIDGEOS_DEF\" $BRIDGEOS_UNAVAIL" \
	HEADER_SEARCH_PATHS="\$(inherited)" \
	SYSTEM_HEADER_SEARCH_PATHS="$HDRS" \
	OBJROOT="$DXB/build/$PROJECT-$TARGET.obj" SYMROOT="$DXB/build/$PROJECT-$TARGET.sym" \
	DSTROOT="$OUTROOT" FAKEROOT_DIR="$FAKEROOT_DIR" \
	RC_ProjectSourceVersion=1 RC_ProjectNameAndSourceVersion="$PROJECT-1" "$@"
