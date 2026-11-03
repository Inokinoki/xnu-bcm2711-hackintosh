#!/bin/sh
# libdispatch installs darwin/module.modulemap twice (a Public and a Private header entry, same destination
# without Apple's per-header-dir settings), which xcodebuild rejects as "Multiple commands produce". Drop the
# Private entries from the Headers phases; the headers themselves stay.
#
#   userland/patch-libdispatch.sh SRCDIR/libdispatch
set -eu
P=${1:?usage: $0 LIBDISPATCHDIR}/libdispatch.xcodeproj/project.pbxproj
for id in C90144661C73A9F6002638FC E4053A7226EAF67D00362F72 E43B884A2241F19000215272; do
	sed -i.bak "/^[[:space:]]*$id \/\* module.modulemap in Headers \*\/,\$/d" "$P"
done
rm -f "$P.bak"
grep -c 'module.modulemap in Headers \*/,' "$P"
