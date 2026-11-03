# Shared settings for the library build scripts (source it): the macOS 13.5 header roots and compiler flags.
# The caller sets DXB (userland/dxb-headers.sh output), SRC (fetch-sources.py output) and OUTROOT.
# See build-libsyscall.sh for why the header paths look like this.
FAKEROOT_DIR=$DXB/fakeroot
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
SYSPRIV=$FAKEROOT_DIR/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders
# fakeroot first (macOS 13.5), then what only the SDK has, then our stand-ins for headers missing from the open source
# (userland/include; the caller sets HERE to the userland directory)
HDR_PATHS="$SYSPRIV $FAKEROOT_DIR/usr/local/include $FAKEROOT_DIR/usr/include $SDK/usr/local/include $SDK/System/Library/Frameworks/System.framework/PrivateHeaders $HERE/include"
# LIB_EXTRA_HDRS: more stand-in header directories for one library (userland/include-dyld for dyld)
HDR_PATHS="$HDR_PATHS ${LIB_EXTRA_HDRS:-}"
# libc++ must come before the C library headers in the search order, or <cstddef> and friends break
[ -z "${CXXSTD:-}" ] || HDR_PATHS="$SDK/usr/include/c++/v1 $HDR_PATHS"
# Xcode 15's SDK dropped the bridgeos availability macros that the 13.5 headers use
BRIDGEOS_DEF='-D__API_AVAILABLE_PLATFORM_bridgeos(x)=bridgeos,introduced=x'
BRIDGEOS_UNAVAIL='-D__API_UNAVAILABLE_PLATFORM_bridgeos=bridgeos,unavailable'
# old code vs clang 15: these became errors
COMPAT_FLAGS="-Wno-int-conversion -Wno-implicit-function-declaration -Wno-incompatible-function-pointer-types -Wno-error"
CPU_FLAGS="-mcpu=cortex-a72"

# Xcode does not ship BSD.xcconfig; drop the includes of it from a project's xcconfigs
strip_bsd_xcconfig() {
	(cd "$1" && grep -rl 'BSD.xcconfig' . --include='*.xcconfig' | while read -r f; do sed -i '' '/BSD.xcconfig/d' "$f"; done) || true
}
