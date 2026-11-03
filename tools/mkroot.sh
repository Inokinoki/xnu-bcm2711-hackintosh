#!/bin/sh
# Turn the userland root that CI builds (artifact userland-arm64-root) into an HFS+ root image for the RAM disk
# of tools/pi4-qemu-boot.sh. macOS only: the image is made with hdiutil, which, unlike tools/mkhfsroot.py,
# handles hundreds of files and symlinks.
#
#   tools/mkroot.sh ROOT OUT.img [SIZE_MB]
#
# Trimmed: no dSYM bundles, no .tbd/.a files, nothing under usr/local. Added: userland/rootfs (etc, SystemVersion.plist),
# the directories a boot needs (dev, tmp, var, private), /bin/sh -> bash.
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
ROOT=${1:?usage: $0 ROOT OUT.img [SIZE_MB]}
OUT=${2:?}
STAGE=$(mktemp -d "${TMPDIR:-/tmp}/rootstage.XXXXXX")

rsync -a --exclude '*.dSYM' --exclude '*.tbd' --exclude '*.a' --exclude '/usr/local' --exclude MANIFEST "$ROOT"/ "$STAGE"/
rsync -a "$HERE/userland/rootfs"/ "$STAGE"/
mkdir -p "$STAGE"/dev "$STAGE"/tmp "$STAGE"/var/root "$STAGE"/var/empty "$STAGE"/var/tmp "$STAGE"/private "$STAGE"/sbin "$STAGE"/bin
chmod 1777 "$STAGE"/tmp "$STAGE"/var/tmp
ln -sf bash "$STAGE"/bin/sh
find "$STAGE" -name .DS_Store -delete

used=$(du -sm "$STAGE" | cut -f1)
SIZE=${3:-$((used * 5 / 4 + 24))}
echo "staged root: ${used} MB in $(find "$STAGE" | wc -l | tr -d ' ') entries; image ${SIZE} MB"

NAME=BCM2711Root
rm -f "$OUT" "$OUT.dmg" "$OUT.cdr"
hdiutil create -size "${SIZE}m" -fs HFS+ -volname "$NAME" -layout NONE -ov "$OUT" >/dev/null
DEV=$(hdiutil attach -owners on -nobrowse -noverify "$OUT.dmg" | awk '/Apple_HFS|HFS/ {print $1; exit}')
MNT=$(mount | awk -v d="$DEV" '$1 == d {print $3; exit}')
[ -n "$MNT" ] || MNT=/Volumes/$NAME
sudo mdutil -i off "$MNT" >/dev/null 2>&1 || true
sudo rsync -a "$STAGE"/ "$MNT"/
sudo chown -R 0:0 "$MNT" 2>/dev/null || true
sudo rm -rf "$MNT"/.fseventsd "$MNT"/.Spotlight-V100 "$MNT"/.Trashes "$MNT"/.DS_Store
sync
hdiutil detach "$DEV" >/dev/null
hdiutil convert "$OUT.dmg" -format UDTO -o "$OUT" >/dev/null
mv "$OUT.cdr" "$OUT"
rm -f "$OUT.dmg"
rm -rf "$STAGE"

# raw HFS+ volume header at offset 1024: 'H+' (or 'HX' for HFSX)
python3 -I - "$OUT" <<'PY'
import sys
b = open(sys.argv[1], 'rb').read(1026)
sig = b[1024:1026]
assert sig in (b'H+', b'HX'), f'not a raw HFS+ image (signature {sig!r}); layout?'
print(f'{sys.argv[1]}: raw HFS+ image ({sig.decode()})')
PY
