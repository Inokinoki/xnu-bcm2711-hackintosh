# Shared settings for the library build scripts (source it): the macOS 13.5 header roots and compiler flags.
# The caller sets DXB (userland/dxb-headers.sh output), SRC (fetch-sources.py output) and OUTROOT.
# See build-libsyscall.sh for why the header paths look like this.
FAKEROOT_DIR=$DXB/fakeroot
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
SYSPRIV=$FAKEROOT_DIR/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders
HDR_PATHS="$SYSPRIV $FAKEROOT_DIR/usr/local/include $FAKEROOT_DIR/usr/include"
BRIDGEOS_DEF='-D__API_AVAILABLE_PLATFORM_bridgeos(x)=bridgeos,introduced=x'
CPU_FLAGS="-mcpu=cortex-a72"

# Xcode does not ship BSD.xcconfig; drop the includes of it from a project's xcconfigs
strip_bsd_xcconfig() {
	(cd "$1" && grep -rl 'BSD.xcconfig' . --include='*.xcconfig' | while read -r f; do sed -i '' '/BSD.xcconfig/d' "$f"; done) || true
}
