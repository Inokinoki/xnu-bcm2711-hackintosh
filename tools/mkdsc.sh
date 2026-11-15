#!/bin/sh
# Build the dyld shared cache for the userland root and install it at ROOT/System/Library/dyld/dyld_shared_cache_arm64.
# macOS only (otool, file); needs the cache builder binary that the dyld-cache workflow publishes (artifact dsc-builder).
#
#   tools/mkdsc.sh BUILDER ROOT [WORKDIR]
#
# Every dylib directly under usr/lib and usr/lib/system whose install name is its own path goes in; the Mach-O
# executables of bin, sbin, usr/bin and usr/sbin are given to the builder so it can build loaders (it requires a main
# executable). The dylibs stay on disk: dyld uses the cache for them and the files are the fallback.
set -eu
BUILDER=${1:?usage: $0 BUILDER ROOT [WORKDIR]}
ROOT=${2:?usage: $0 BUILDER ROOT [WORKDIR]}
W=${3:-$(mktemp -d "${TMPDIR:-/tmp}/dsc.XXXXXX")}
mkdir -p "$W"
LIST=$W/list.txt
: > "$LIST"
(cd "$ROOT" && find usr/lib -name '*.dylib' -type f | sort) | while read -r f; do
	[ "$(otool -D "$ROOT/$f" | tail -1)" = "/$f" ] && echo "/$f" || echo "skip /$f: install name differs" >&2
done >> "$LIST"
(cd "$ROOT" && for d in bin usr/bin sbin usr/sbin; do [ -d "$d" ] && find "$d" -type f; done | sort) | while read -r f; do
	file "$ROOT/$f" | grep -q "Mach-O 64-bit executable arm64" && echo "exe /$f" || true
done >> "$LIST"
echo "cache inputs: $(grep -vc '^exe ' "$LIST") dylibs, $(grep -c '^exe ' "$LIST") executables"
chmod +x "$BUILDER"
"$BUILDER" "$W/out" "$ROOT" "$LIST" arm64 2> "$W/builder.log" || { tail -n 40 "$W/builder.log"; exit 1; }
tail -n 4 "$W/builder.log"
mkdir -p "$ROOT/System/Library/dyld"
cp "$W/out/dyld_shared_cache_arm64" "$ROOT/System/Library/dyld/"
ls -l "$ROOT/System/Library/dyld"
