#!/bin/sh
# diskdev_cmds-701: the tools link APFS, MediaKit, CoreFoundation and IOKit frameworks, which this system does not have
# (APFS and MediaKit are private, the others would need a CoreFoundation port). mount.c and umount.c use none of them;
# only the iPhone-only code of edt_fstab.c and mount_ramdisk.c does, and that is compiled out here. Drop them from the
# link phases; the few APFS headers mount needs are stand-ins in userland/include/APFS.
#
#   userland/patch-diskdev.sh DISKDEV_CMDS_SRCDIR
set -eu
python3 -I - "${1:?usage: $0 DISKDEV_CMDS_SRCDIR}/diskdev_cmds.xcodeproj/project.pbxproj" <<'PY'
import re, sys
p = sys.argv[1]
s = open(p).read()
t = re.sub(r'\n\t+[0-9A-F]{24} /\* (?:APFS|MediaKit|CoreFoundation|IOKit)\.framework in Frameworks \*/,', '', s)
open(p, "w").write(t)
PY
! awk '/files = \(/{f=1} /\);/{f=0} f && /(APFS|MediaKit|CoreFoundation|IOKit).framework in Frameworks/{bad=1} END{exit !bad}' "$1/diskdev_cmds.xcodeproj/project.pbxproj"
