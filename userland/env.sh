# Compiler settings for every userland binary (source it: . userland/env.sh).
# Plain arm64 for the Cortex-A72: no arm64e / pointer authentication, no LSE atomics, 16K segments.
# Needs macOS with Xcode; tools/check-arm64-plain.sh checks the result.
SDK=${SDK:-$(xcrun --sdk macosx --show-sdk-path)}
export SDK
export CC="xcrun --sdk macosx clang"
export USERLAND_TARGET=arm64-apple-macos13.0
export USERLAND_CFLAGS="-target $USERLAND_TARGET -mcpu=cortex-a72 -O2 -g -isysroot $SDK"
export USERLAND_LDFLAGS="-target $USERLAND_TARGET -isysroot $SDK"
