#!/bin/sh
# Userland build driver (stage A: toolchain and gate only; the libraries come in later stages).
#
#   userland/build.sh [OUTDIR]        # default build/userland; needs macOS + Xcode
#
# 1. builds src/smoke.c with the flags of env.sh and runs tools/check-arm64-plain.sh on it (must pass);
# 2. builds the same file as arm64e and with the Apple CPU and runs the gate on those (must FAIL), so a
#    gate that stops detecting PAC or LSE is noticed.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
OUT=${1:-$ROOT/build/userland}
GATE=$ROOT/tools/check-arm64-plain.sh
. "$HERE/env.sh"
mkdir -p "$OUT/root/usr/bin" "$OUT/negative"

# shellcheck disable=SC2086
$CC $USERLAND_CFLAGS $USERLAND_LDFLAGS "$HERE/src/smoke.c" -o "$OUT/root/usr/bin/smoke"
"$GATE" "$OUT/root/usr/bin/smoke"

# shellcheck disable=SC2086
$CC -arch arm64e -O2 -isysroot "$SDK" -mmacosx-version-min=13.0 "$HERE/src/smoke.c" -o "$OUT/negative/smoke-arm64e"
# shellcheck disable=SC2086
$CC -target "$USERLAND_TARGET" -mcpu=apple-m1 -O2 -isysroot "$SDK" "$HERE/src/smoke.c" -o "$OUT/negative/smoke-lse"
for f in "$OUT"/negative/*; do
	if "$GATE" "$f"; then
		echo "gate accepted $f, it must reject it" >&2
		exit 1
	fi
done
echo "stage A OK: gate accepts plain arm64/cortex-a72 and rejects arm64e and LSE"
