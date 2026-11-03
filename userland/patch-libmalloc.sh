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
