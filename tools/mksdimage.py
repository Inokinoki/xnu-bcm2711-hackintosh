#!/usr/bin/env python3
"""Assemble a Raspberry Pi 4 SD card image: a FAT32 boot partition and the HFS+ root.

  mksdimage.py OUT.img --root root.img --boot NAME=FILE [--boot NAME=FILE ...]
                       [--boot-size 128M] [--pad-pow2]

  MBR partition 1: FAT32 (LBA), the files the Pi firmware reads (start4.elf, config.txt, kernel8.img, ...)
  MBR partition 2: type 0xAF, the HFS+ volume from tools/mkhfsroot.py or tools/mkroot.sh

The kernel's SD driver exposes partition 2 as sdroot0 with the boot-arg `sdpart=2`. --pad-pow2 rounds the
card up to a power of two, which QEMU requires for an SD card. The FAT volume is flat (no subdirectories)
and written without any tool, so it can be built in CI on any host. Check it with: fsck.fat -n -v -s1 on
the partition, and sfdisk -l on the image.
"""
import argparse, os, struct, sys

SEC = 512
ALIGN = 2048                    # 1 MiB, in sectors
FATSEC_RESERVED = 32


def parse_size(s):
    mult = {'k': 1 << 10, 'm': 1 << 20, 'g': 1 << 30}
    s = s.strip().lower()
    return int(s[:-1]) * mult[s[-1]] if s[-1] in mult else int(s)


def short_name(name, used):
    """8.3 name for NAME; returns (11 bytes, needs_lfn). Names already in upper-case 8.3 need no LFN."""
    base, dot, ext = name.rpartition('.')
    if not dot:
        base, ext = name, ''
    okc = lambda t: all(c.isalnum() or c in '_-' for c in t) and t == t.upper()
    if len(base) <= 8 and len(ext) <= 3 and base and okc(base) and okc(ext):
        return (base.ljust(8) + ext.ljust(3)).encode('ascii'), False
    clean = lambda t: ''.join(c if c.isalnum() or c in '_-' else '_' for c in t).upper()
    b, e = clean(base)[:6] or 'FILE', clean(ext)[:3]
    for i in range(1, 1000):
        cand = (b + '~%d' % i).ljust(8) + e.ljust(3)
        if cand not in used:
            return cand.encode('ascii'), True
    raise SystemExit('no short name for ' + name)


def lfn_checksum(sfn):
    s = 0
    for c in sfn:
        s = (((s & 1) << 7) + (s >> 1) + c) & 0xff
    return s


def dir_entries(name, sfn, lfn, first_cluster, size):
    out = b''
    if lfn:
        chars = name + '\0'
        chars += '￿' * ((-len(chars)) % 13)
        n = len(chars) // 13
        cs = lfn_checksum(sfn)
        for i in range(n, 0, -1):
            part = chars[(i - 1) * 13:i * 13]
            u = [ord(c) for c in part]
            seq = i | (0x40 if i == n else 0)
            out += struct.pack('<B5HBBB6HH2H', seq, *u[0:5], 0x0f, 0, cs, *u[5:11], 0, *u[11:13])
    hi, lo = first_cluster >> 16, first_cluster & 0xffff
    out += struct.pack('<11sBBBHHHHHHHI', sfn, 0x20, 0, 0, 0, 0x5821, 0x5821, hi, 0x5821, 0x5821, lo, size)
    return out


def build_fat32(files, size_bytes, label='BCM2711BOOT'):
    total = size_bytes // SEC
    # smallest cluster count that is still FAT32 (>= 65525 clusters), else 1 sector/cluster
    for cand in (8, 4, 2, 1):
        if total // cand >= 65525 + 2048:
            spc = cand
            break
    else:
        spc = 1
    csize = spc * SEC
    fatsz = 1
    while True:
        clusters = (total - FATSEC_RESERVED - 2 * fatsz) // spc
        need = (clusters + 2 + 127) // 128
        if need <= fatsz:
            break
        fatsz = need
    if clusters < 65525:
        raise SystemExit('boot partition too small for FAT32')
    data_start = FATSEC_RESERVED + 2 * fatsz

    used = set()
    ents = []
    for name, data in files:
        sfn, lfn = short_name(name, used)
        used.add(sfn.decode('ascii'))
        ents.append([name, sfn, lfn, data])
    # root directory: volume label + entries
    dir_bytes = 32 + sum(len(dir_entries(n, s, l, 0, 0)) for n, s, l, _ in ents)
    root_clusters = max(1, -(-dir_bytes // csize))

    fat = [0] * (clusters + 2)
    fat[0], fat[1] = 0x0ffffff8, 0x0fffffff
    nxt = 2

    def alloc(nbytes):
        nonlocal nxt
        n = max(1, -(-nbytes // csize)) if nbytes else 0
        if not n:
            return 0, 0
        first = nxt
        if first - 2 + n > clusters:
            raise SystemExit('boot files do not fit in the boot partition (%d bytes)' % size_bytes)
        for i in range(n):
            fat[nxt + i] = nxt + i + 1 if i < n - 1 else 0x0fffffff
        nxt += n
        return first, n

    root_first, _ = alloc(root_clusters * csize)
    placed = []
    for name, sfn, lfn, data in ents:
        first, n = alloc(len(data))
        placed.append((name, sfn, lfn, first, data))

    img = bytearray(total * SEC)
    # boot sector
    bs = bytearray(SEC)
    bs[0:3] = b'\xeb\x58\x90'
    bs[3:11] = b'MSWIN4.1'
    struct.pack_into('<HBHBHHBHHHII', bs, 11, SEC, spc, FATSEC_RESERVED, 2, 0, 0, 0xf8, 0, 32, 64, ALIGN, total)
    struct.pack_into('<IHHIHH', bs, 36, fatsz, 0, 0, 2, 1, 6)
    struct.pack_into('<BBBI', bs, 64, 0x80, 0, 0x29, 0x42424242)
    bs[71:82] = label.encode().ljust(11)
    bs[82:90] = b'FAT32   '
    bs[510:512] = b'\x55\xaa'
    img[0:SEC] = bs
    img[6 * SEC:7 * SEC] = bs
    fsi = bytearray(SEC)
    struct.pack_into('<I', fsi, 0, 0x41615252)
    struct.pack_into('<III', fsi, 484, 0x61417272, clusters - (nxt - 2), nxt)
    fsi[510:512] = b'\x55\xaa'
    img[SEC:2 * SEC] = fsi
    img[7 * SEC:8 * SEC] = fsi
    fatb = struct.pack('<%dI' % len(fat), *fat)
    for k in range(2):
        o = (FATSEC_RESERVED + k * fatsz) * SEC
        img[o:o + len(fatb)] = fatb
    # root directory and file data
    d = struct.pack('<11sBBBHHHHHHHI', label.encode().ljust(11), 0x08, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0)
    for name, sfn, lfn, first, data in placed:
        d += dir_entries(name, sfn, lfn, first, len(data))
    o = (data_start + (root_first - 2) * spc) * SEC
    img[o:o + len(d)] = d
    for name, sfn, lfn, first, data in placed:
        if data:
            o = (data_start + (first - 2) * spc) * SEC
            img[o:o + len(data)] = data
    return bytes(img)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('out')
    ap.add_argument('--root', required=True, help='HFS+ root volume image (partition 2)')
    ap.add_argument('--boot', action='append', default=[], metavar='NAME=FILE')
    ap.add_argument('--boot-size', default='128M')
    ap.add_argument('--pad-pow2', action='store_true')
    a = ap.parse_args()

    files = []
    for spec in a.boot:
        name, _, path = spec.partition('=')
        if not path:
            sys.exit('--boot wants NAME=FILE: ' + spec)
        files.append((name, open(path, 'rb').read()))
    root = open(a.root, 'rb').read()
    boot_sectors = parse_size(a.boot_size) // SEC
    boot = build_fat32(files, boot_sectors * SEC)
    root_start = ALIGN + boot_sectors
    root_start += (-root_start) % ALIGN
    root_sectors = -(-len(root) // SEC)

    mbr = bytearray(SEC)
    def part(i, boot_flag, typ, start, count):
        struct.pack_into('<B3sB3sII', mbr, 446 + 16 * i, boot_flag, b'\xfe\xff\xff', typ, b'\xfe\xff\xff', start, count)
    part(0, 0x80, 0x0c, ALIGN, boot_sectors)
    part(1, 0x00, 0xaf, root_start, root_sectors)
    struct.pack_into('<I', mbr, 440, 0x4d42b0a5)
    mbr[510:512] = b'\x55\xaa'

    total = (root_start + root_sectors) * SEC
    if a.pad_pow2:
        p = 1
        while p < total:
            p *= 2
        total = p
    with open(a.out, 'wb') as f:
        f.truncate(total)
        f.seek(0)
        f.write(mbr)
        f.seek(ALIGN * SEC)
        f.write(boot)
        f.seek(root_start * SEC)
        f.write(root)
    print('%s: %d bytes; p1 FAT32 sector %d (%d sectors, %d files), p2 HFS+ sector %d (%d sectors)' %
          (a.out, total, ALIGN, boot_sectors, len(files), root_start, root_sectors))


if __name__ == '__main__':
    main()
