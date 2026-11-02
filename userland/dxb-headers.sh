#!/bin/sh
# Prepare the build roots the userland libraries need: xnu's installed headers (with our kernel patches
# applied, so the commpage and cpu_capabilities definitions match the kernel), Libsystem and libsyscall
# headers, libplatform headers and `mig`, all in DXB/fakeroot. This is blacktop/darwin-xnu-build up to
# (not including) libfirehose_kernel and the kernel itself.
#
#   userland/dxb-headers.sh DXBDIR      # needs macOS + Xcode, brew jq/gum/bash; see userland-arm64.yml
#
# The darwin-xnu-build revision is the one of the kernel workflow; keep the two in sync.
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$HERE/.." && pwd)
DXB=${1:?usage: $0 DXBDIR}
DXB_REPO=${DXB_REPO:-https://github.com/blacktop/darwin-xnu-build}
DXB_SHA=${DXB_SHA:-663906528d1cd5ea8fde7de6f16de73db9511549}
export MACOS_VERSION=${MACOS_VERSION:-13.5}
export KERNEL_CONFIG=${KERNEL_CONFIG:-DEVELOPMENT}
export MACHINE_CONFIG=${MACHINE_CONFIG:-BCM2837}

if [ ! -d "$DXB/.git" ]; then
	git init -q "$DXB"
	git -C "$DXB" fetch -q --depth 1 "$DXB_REPO" "$DXB_SHA"
	git -C "$DXB" checkout -q FETCH_HEAD
fi
cp "$ROOT"/xnu/patches-8796.141.3/*.patch "$ROOT"/xnu/patches-8796.141.3/*.sh "$DXB/patches/"

# stop before build_libdispatch: everything after it is the kernel build
python3 -I - "$DXB/build.sh" <<'PY'
import sys
p = sys.argv[1]
s = open(p).read()
old = "    build_libdispatch\n    if [ \"$BUILDLIB\""
assert s.count(old) == 1, s.count(old)
open(p, "w").write(s.replace(old, "    echo 'userland: headers and mig done'\n    exit 0\n" + old))
PY

cd "$DXB"
bash build.sh
test -f fakeroot/usr/include/os/proc.h
test -n "$(find fakeroot -name mig -type f)"
