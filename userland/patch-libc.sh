#!/bin/sh
# Libc-1534: stdio_init.c and chk_fail.c use dyld_platform_version_macOS_* macros that are generated into the
# SDK's dyld_priv.h but absent from dyld's sources. Include our definitions (userland/include/dyld_version_shim.h)
# in those two files; a global -include would also hit Libc's generated sources, whose include setup differs.
#
# The cancelable variants (libvCancelable) must define the plain symbols (_creat, _recv, _send...) while the base
# library defines the $NOCANCEL ones. Libc gets that from its own sanitised copies of the system headers; we
# compile against the SDK/fakeroot headers, whose __DARWIN_ALIAS_C follows __DARWIN_NON_CANCELABLE, so the
# variant macros must clear it (as VARIANT_DYLD_MACROS already does).
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
python3 -I - "$D/xcodescripts/variants.xcconfig" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
add = " -U__DARWIN_NON_CANCELABLE -D__DARWIN_NON_CANCELABLE=0"
out = []
for line in s.split("\n"):
    if line.startswith(("VARIANT_CANCELABLE_MACROS =", "VARIANT_DARWINEXTSN_CANCELABLE_MACROS =")) and add not in line:
        line += add
    out.append(line)
open(p, "w").write("\n".join(out))
PY
grep -q 'D__DARWIN_NON_CANCELABLE=0' "$D/xcodescripts/variants.xcconfig"
grep -q dyld_version_shim.h "$D/stdio/stdio_init.c" "$D/secure/chk_fail.c"

# Xcode 16's build system refuses a link whose inputs no script phase declares as outputs. The libsystem_c.dylib
# target's "Build Link Lists" phase writes interposable.list and unexport.list (which libc.xcconfig passes to the
# linker) without saying so; Xcode 15 ran the phase first anyway. Declare them.
python3 -I - "$D/Libc.xcodeproj/project.pbxproj" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
head = '/* Build Link Lists */ = {\n'
i = s.index(head)
if '$(DERIVED_FILE_DIR)/interposable.list' not in s[i:i + 700]:
    old = '\t\t\toutputPaths = (\n\t\t\t);\n'
    j = s.index(old, i)
    assert j - i < 600
    new = ('\t\t\toutputPaths = (\n\t\t\t\t"$(DERIVED_FILE_DIR)/interposable.list",\n'
           '\t\t\t\t"$(DERIVED_FILE_DIR)/unexport.list",\n\t\t\t);\n')
    open(p, "w").write(s[:j] + new + s[j + len(old):])
PY
