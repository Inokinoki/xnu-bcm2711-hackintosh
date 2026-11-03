#!/bin/sh
# dyld-1066: the Xcode project links /usr/local/lib/dyld/*.a from the SDK (the archives of Libc, libplatform,
# libpthread and libsyscall built for dyld) and libCrashReporterClient, which is closed. Take the archives from
# DYLD_LIBS_DIR (a build setting, so it reaches the script phase as an environment variable) and drop the closed library.
#
#   userland/patch-dyld.sh DYLD_SRCDIR
# The PrebuiltLoader layout-hash script runs its own clang++ with only dyld's -I paths; DYLD_EXTRA_INC (also a
# build setting) lets it find the private headers (_simple.h, CrashReporterClient.h) as a last resort.
set -eu
D=${1:?usage: $0 DYLD_SRCDIR}
sed -i '' 's|\${SDKROOT}/usr/local/lib/dyld|${DYLD_LIBS_DIR}|g' "$D/build-scripts/configure-dyld-archives.sh"
sed -i '' 's| -lCrashReporterClient||' "$D/configs/libdyld.xcconfig"
sed -i '' 's|-Ilsl \${DERIVED_FILE_DIR}|-Ilsl ${DYLD_EXTRA_INC} ${DERIVED_FILE_DIR}|' "$D/build-scripts/prebuilt-loader-hash.sh"
grep -q 'DYLD_EXTRA_INC' "$D/build-scripts/prebuilt-loader-hash.sh"
grep -q 'DYLD_LIBS_DIR' "$D/build-scripts/configure-dyld-archives.sh"
! grep -q lCrashReporterClient "$D/configs/libdyld.xcconfig"
