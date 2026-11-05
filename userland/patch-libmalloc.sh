#!/bin/sh
# libmalloc-425: nano_zone.h (nano v1, the x86_64 allocator) refuses any other architecture, yet internal.h
# includes it whenever CONFIG_NANOZONE is set, which is for every 64-bit target. arm64 uses nano v2 only; give
# v1's address-layout constants an arm64 branch (same values) so the headers compile. Nothing at runtime uses v1.
#
#   userland/patch-libmalloc.sh LIBMALLOC_SRCDIR
set -eu
F=${1:?usage: $0 LIBMALLOC_SRCDIR}/src/nano_zone.h
grep -q 'defined(__x86_64) || defined(__arm64__)' "$F" ||
	sed -i '' 's|^#if defined(__x86_64)$|#if defined(__x86_64) \|\| defined(__arm64__)|' "$F"
grep -q 'defined(__x86_64) || defined(__arm64__)' "$F"

# Nano v2 reserves its regions from 0x600000000000 up; the 4K-page kernel here gives user processes a smaller
# address space (the first boot died on a translation fault at 0x6000026fbe98). Use the scalable zone only.
# LIBMALLOC_NANO=1 keeps nano on. It boots (the kernel rejects the reservation, malloc falls back) but every process then prints
# "nano zone abandoned due to inability to reserve vm space" on stderr, so the default stays off.
P=${1}/src/platform.h
[ "${LIBMALLOC_NANO:-0}" = 1 ] || grep -q 'userland/patch-libmalloc.sh' "$P" || {
	python3 -I - "$P" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
old = "#if MALLOC_TARGET_64BIT\n#define CONFIG_NANOZONE 1\n"
assert s.count(old) == 1
open(p, "w").write(s.replace(old, "#if MALLOC_TARGET_64BIT\n#define CONFIG_NANOZONE 0 /* userland/patch-libmalloc.sh */\n"))
PY
}
