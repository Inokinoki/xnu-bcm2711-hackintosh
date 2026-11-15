#!/usr/bin/env python3
"""Check a QEMU screendump (PPM, P6) against the picture userland/src/fbtest.c draws.

  check-fbtest.py SCREEN.ppm

Exits 1 and prints the mismatches when the bars, gradient or marker are not where fbtest puts them. A swapped
red and blue channel (wrong pixel order) is reported as such.
"""
import sys


def read_ppm(path):
    d = open(path, 'rb').read()
    parts = d.split(None, 4)
    assert parts[0] == b'P6' and parts[3] == b'255', 'not an 8-bit P6 PPM'
    w, h = int(parts[1]), int(parts[2])
    return w, h, parts[4][:w * h * 3]


def main():
    w, h, px = read_ppm(sys.argv[1])
    at = lambda x, y: tuple(px[(y * w + x) * 3:(y * w + x) * 3 + 3])
    expect = {
        (w // 16, 100): (255, 255, 255),              # bar 0 white
        (w * 3 // 16, 100): (255, 255, 0),            # bar 1 yellow
        (w * 5 // 16, 100): (0, 255, 255),            # bar 2 cyan
        (w * 7 // 16, 100): (0, 255, 0),              # bar 3 green
        (w * 9 // 16, 100): (255, 0, 255),            # bar 4 magenta
        (w * 11 // 16, 100): (255, 0, 0),             # bar 5 red
        (w * 13 // 16, 100): (0, 0, 255),             # bar 6 blue
        (w * 15 // 16, 100): (0, 0, 0),               # bar 7 black
        (0, h - 40): (0, 0, 0),                       # gradient start
        (w - 1, h - 40): (255, 255, 255),             # gradient end
        (70, 450): (255, 255, 255),                   # marker
        (96, 480): (255, 0, 0),                       # marker centre
    }
    bad = [(p, at(*p), c) for p, c in expect.items() if at(*p) != c]
    for p, got, want in bad:
        print('pixel %s: got %s, want %s' % (p, got, want))
    if bad and all(got == want[::-1] for _, got, want in bad):
        print('red and blue are swapped: the framebuffer pixel order is not 0x00RRGGBB')
    print('%dx%d screen: %d of %d probes match' % (w, h, len(expect) - len(bad), len(expect)))
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
