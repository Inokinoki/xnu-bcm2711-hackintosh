#!/bin/sh
# Build the command-line tools of one Apple project (one xcodebuild per target, so a failing tool does not stop
# the others), installing into OUTROOT. Exits non-zero if any target failed, after trying all of them.
#
#   userland/build-commands.sh DXBDIR SRCDIR OUTROOT PROJECT TARGET...
set -u
HERE=$(cd "$(dirname "$0")" && pwd)
DXB=$1 SRC=$2 OUT=$3 PROJECT=$4
shift 4
failed=
for t in "$@"; do
	echo "::group::$PROJECT/$t"
	if "$HERE/build-xcode-lib.sh" "$DXB" "$SRC" "$OUT" "$PROJECT" "$t" >"$DXB/build/cmd-$PROJECT-$t.log" 2>&1; then
		echo "built $t"
	else
		echo "FAILED $t"
		grep -E "error:|Undefined symbols|^  \"_|ld: " "$DXB/build/cmd-$PROJECT-$t.log" | sort -u | head -12
		failed="$failed $t"
	fi
	echo "::endgroup::"
done
[ -z "$failed" ] || { echo "failed $PROJECT targets:$failed"; exit 1; }
