/*
 * ___chkstk_darwin: clang calls it (x9 = frame size) before allocating a large stack frame, and macOS's
 * libcompiler_rt provides it. The Xcode toolchain's libclang_rt.osx.a does not, so supply the probe: touch
 * every page of the new frame so the guard page faults in order. Preserves all registers except x9 and the
 * flags. The page step is 4K, a divisor of both our kernel page size and the 16K user page size.
 */
	.text
	.align	2
	.globl	___chkstk_darwin
___chkstk_darwin:
	stp	x10, x11, [sp, #-16]
	mov	x10, sp
	cmp	x9, #0x1000
	b.lo	2f
1:
	sub	x10, x10, #0x1000
	ldr	x11, [x10]
	sub	x9, x9, #0x1000
	cmp	x9, #0x1000
	b.hs	1b
2:
	ldp	x10, x11, [sp, #-16]
	ret
