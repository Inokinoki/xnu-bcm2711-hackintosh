#!/bin/sh
# Libinfo: swap the two sources that need closed services (OpenDirectory, mDNSResponder) for stand-ins.
#
#   userland/patch-libinfo.sh SRCDIR/Libinfo
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
cp "$HERE/src/libinfo/membership.c" "${1:?usage: $0 LIBINFODIR}/membership.subproj/membership.c"
cp "$HERE/src/libinfo/mdns_module.c" "$1/lookup.subproj/mdns_module.c"
cp "$HERE/src/libinfo/ds_module.c" "$1/lookup.subproj/ds_module.c"
