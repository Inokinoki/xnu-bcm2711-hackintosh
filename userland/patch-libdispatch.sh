#!/bin/sh
# libdispatch installs darwin/module.modulemap twice (a Public and a Private header entry, same destination
# without Apple's per-header-dir settings), which xcodebuild rejects as "Multiple commands produce". Drop the
# Private entries from the Headers phases; the headers themselves stay.
#
#   userland/patch-libdispatch.sh SRCDIR/libdispatch
set -eu
P=${1:?usage: $0 LIBDISPATCHDIR}/libdispatch.xcodeproj/project.pbxproj
for id in C90144661C73A9F6002638FC E4053A7226EAF67D00362F72 E43B884A2241F19000215272; do
	sed -i.bak "/^[[:space:]]*$id \/\* module.modulemap in Headers \*\/,\$/d" "$P"
done
rm -f "$P.bak"
grep -c 'module.modulemap in Headers \*/,' "$P"

# queue_internal.h uses dispatch_pthread_root_queue_observer_hooks_s in dispatch_workloop_attr_s before it defines
# it (Apple's internal SDK declares it earlier through private/queue_private.h): move the definition up
python3 -I - "${1}/src/queue_internal.h" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
blk = """typedef struct dispatch_pthread_root_queue_observer_hooks_s {
	void (*queue_will_execute)(dispatch_queue_t queue);
	void (*queue_did_execute)(dispatch_queue_t queue);
} dispatch_pthread_root_queue_observer_hooks_s;
typedef dispatch_pthread_root_queue_observer_hooks_s
		*dispatch_pthread_root_queue_observer_hooks_t;
"""
anchor = "#define DISPATCH_WORKLOOP_ATTR_HAS_SCHED "
assert s.count(blk) == 1 and s.count(anchor) == 1
s = s.replace(blk, "")
s = s.replace(anchor, blk + "\n" + anchor)
open(p, "w").write(s)
PY
