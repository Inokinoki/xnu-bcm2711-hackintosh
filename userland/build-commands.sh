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
	# a target that hangs (notifyd did, for the two hours of the job limit) must not eat the whole job: give it 20 minutes,
	# then show where it stuck
	log=$DXB/build/cmd-$PROJECT-$t.log
	"$HERE/build-xcode-lib.sh" "$DXB" "$SRC" "$OUT" "$PROJECT" "$t" >"$log" 2>&1 &
	pid=$!
	( sleep 1200; echo "TIMEOUT $PROJECT/$t: still running after 20 minutes"; tail -30 "$log"; ps -axo pid,ppid,etime,command | grep -E "xcodebuild|clang|ld|codesign|sh -c|Script" | grep -v grep | cut -c1-240; pkill -P $pid 2>/dev/null; kill $pid 2>/dev/null ) &
	wd=$!
	if wait $pid; then kill $wd 2>/dev/null; pkill -P $wd 2>/dev/null;
		echo "built $t"
	else
		kill $wd 2>/dev/null
		echo "FAILED $t"
		grep -E "error:|Undefined symbols|^  \"_|ld: " "$DXB/build/cmd-$PROJECT-$t.log" | sort -u | head -12
		grep -A10 "^Undefined symbols" "$DXB/build/cmd-$PROJECT-$t.log" | grep '^  "' | sort -u | head -12
		failed="$failed $t"
	fi
	echo "::endgroup::"
done
[ -z "$failed" ] || { echo "failed $PROJECT targets:$failed"; exit 1; }
