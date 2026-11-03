#!/bin/sh
# Libc-1534: stdio_init.c and chk_fail.c use dyld_platform_version_macOS_* macros that are generated into the
# SDK's dyld_priv.h but absent from dyld's sources. Include our definitions (userland/include/dyld_version_shim.h)
# in those two files; a global -include would also hit Libc's generated sources, whose include setup differs.
#
#   userland/patch-libc.sh LIBC_SRCDIR
set -eu
D=${1:?usage: $0 LIBC_SRCDIR}
python3 -I - "$D/stdio/stdio_init.c" "$D/secure/chk_fail.c" <<'PY'
import sys
for p in sys.argv[1:]:
    s = open(p).read()
    line = "#include <dyld_version_shim.h>\n"
    if line in s:
        continue
    i = s.index("#include")
    open(p, "w").write(s[:i] + line + s[i:])
PY
grep -q dyld_version_shim.h "$D/stdio/stdio_init.c" "$D/secure/chk_fail.c"
