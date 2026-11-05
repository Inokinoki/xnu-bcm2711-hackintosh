#!/bin/sh
# libplatform: make bzero return its first argument.
#
# On macOS arm64 the closed assembly bzero leaves its argument in x0, and clang relies on that: for
# "memset(p, 0, n); return p;" it emits a tail call to bzero (sort_list_item_alloc in text_cmds does this).
# libplatform's generic bzero is void and leaves whatever its memset_pattern4 helper left in x0 (p + 12 for
# 16 bytes), so those callers returned a pointer into the middle of the next heap block.
#
#   userland/patch-libplatform.sh SRCDIR/libplatform
set -eu
P=${1:?usage: $0 LIBPLATFORMDIR}
python3 - "$P" <<'PY'
import sys
root = sys.argv[1]

def patch(path, pairs):
    s = open(path).read()
    if "userland/patch-libplatform.sh" in s:
        return
    for old, new in pairs:
        assert old in s, (path, old)
        s = s.replace(old, new, 1)
    open(path, "w").write(s)

patch(root + "/private/platform/string.h", [
    ("void\n_platform_bzero(void *s, size_t n);",
     "void * /* userland/patch-libplatform.sh: returns s */\n_platform_bzero(void *s, size_t n);"),
])
patch(root + "/src/string/generic/bzero.c", [
    ("void\n_platform_bzero(void *s, size_t n)\n{\n\t_platform_memset(s, 0, n);\n}",
     "void * /* userland/patch-libplatform.sh: returns s */\n_platform_bzero(void *s, size_t n)\n{\n\t_platform_memset(s, 0, n);\n\treturn s;\n}"),
    ("void\nbzero(void *s, size_t n) {\n\t_platform_bzero(s, n);\n}",
     "void *\nbzero(void *s, size_t n) {\n\treturn _platform_bzero(s, n);\n}"),
    ("void\n__bzero(void *s, size_t n) {\n\t_platform_bzero(s, n);\n}",
     "void *\n__bzero(void *s, size_t n) {\n\treturn _platform_bzero(s, n);\n}"),
])
PY
