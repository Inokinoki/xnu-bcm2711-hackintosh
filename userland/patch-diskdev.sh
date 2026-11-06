#!/bin/sh
# diskdev_cmds-701: mount and umount link the private APFS.framework. This system has no APFS: drop the framework
# from the link phases (the headers it supplies are userland/include/APFS).
#
#   userland/patch-diskdev.sh DISKDEV_CMDS_SRCDIR
set -eu
python3 -I - "${1:?usage: $0 DISKDEV_CMDS_SRCDIR}/diskdev_cmds.xcodeproj/project.pbxproj" <<'PY'
import re, sys
p = sys.argv[1]
s = open(p).read()
t = re.sub(r'\n\t+[0-9A-F]{24} /\* APFS\.framework in Frameworks \*/,', '', s)
open(p, "w").write(t)
PY
! awk '/files = \(/{f=1} /\);/{f=0} f && /APFS.framework in Frameworks/{bad=1} END{exit !bad}' "$1/diskdev_cmds.xcodeproj/project.pbxproj"
