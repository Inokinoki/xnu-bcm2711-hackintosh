#!/bin/sh
# Install the headers of the userland projects into DXB/fakeroot, next to xnu's, so the libraries compile
# against macOS 13.5 headers throughout. Mixing xnu 13.5 headers with the runner's newer SDK libc headers
# breaks (e.g. availability macros, __sized_by_or_null).
#
#   userland/install-headers.sh DXBDIR SRCDIR PROJECT:TARGET...     # e.g. Libc:Libc libmalloc:libmalloc
#
# SRCDIR is the fetch-sources.py output. Needs the fakeroot of userland/dxb-headers.sh.
set -eu
DXB=$(cd "${1:?usage: $0 DXBDIR SRCDIR PROJECT:TARGET...}" && pwd)
SRC=$(cd "${2:?usage: $0 DXBDIR SRCDIR PROJECT:TARGET...}" && pwd)
shift 2
FAKEROOT_DIR=$DXB/fakeroot

for pt in "$@"; do
	proj=${pt%%:*}
	target=${pt#*:}
	echo "== installhdrs $proj ($target)"
	cd "$SRC/$proj"
	env LD="$(xcrun -find clang)" LDPLUSPLUS="$(xcrun -find clang++)" \
		xcodebuild installhdrs -sdk macosx -target "$target" \
		ARCHS=arm64 VALID_ARCHS=arm64 ONLY_ACTIVE_ARCH=NO CODE_SIGN_IDENTITY=- \
		OBJROOT="$DXB/build/$proj-hdrs.obj" SYMROOT="$DXB/build/$proj-hdrs.sym" \
		DSTROOT="$FAKEROOT_DIR" FAKEROOT_DIR="$FAKEROOT_DIR" \
		RC_ProjectSourceVersion=1 RC_ProjectNameAndSourceVersion="$proj-1"
done
test -f "$FAKEROOT_DIR/usr/include/stdlib.h"
test -f "$FAKEROOT_DIR/usr/include/malloc/malloc.h"
