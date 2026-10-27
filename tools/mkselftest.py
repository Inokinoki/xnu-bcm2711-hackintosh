#!/usr/bin/env python3
"""Wrap a raw arm64 code blob into a minimal kernel-style Mach-O for tools/mkpi_payload.py.

  mkselftest.py CODE.bin OUT.macho

One __TEXT segment at the kernel virt base + 16 MiB, LC_UNIXTHREAD pc at the code
(file offset 0x1000). Only for tools/pi4-selftest.sh.
"""
import struct, sys

VIRT_BASE = 0xfffffff006000000
code = open(sys.argv[1], 'rb').read()
text_va = VIRT_BASE + 0x1000000
size = 0x1000 + len(code)
vmsize = (size + 0x3fff) & ~0x3fff
seg = struct.pack('<II16sQQQQiiII', 0x19, 72, b'__TEXT', text_va, vmsize, 0, size, 5, 5, 0, 0)
thread_state = [0] * 68 + []      # arm_thread_state64: x0-x28, fp, lr, sp, pc (u64), cpsr (u32), pad
regs = [0] * 33
regs[32 - 0] = 0
state = struct.pack('<32QII', *([0] * 32), 0, 0)
state = bytearray(state)
struct.pack_into('<Q', state, 32 * 8, text_va + 0x1000)    # pc
thr = struct.pack('<IIII', 0x5, 16 + len(state), 6, len(state) // 4) + bytes(state)
hdr = struct.pack('<8I', 0xfeedfacf, 0x0100000c, 0, 2, 2, len(seg) + len(thr), 0, 0)
out = bytearray(size)
out[:32 + len(seg) + len(thr)] = hdr + seg + thr
out[0x1000:] = code
open(sys.argv[2], 'wb').write(out)
