/*
 * Stand-in "kernel" for checking the loader on QEMU without a real xnu build
 * (tools/pi4-selftest.sh). Entered with the MMU off, x0 = boot_args PA. Prints a
 * banner and the boot_args fields on the mini UART, then spins.
 */
	.text
	.globl _start
_start:
	mov	x19, x0
	ldr	x20, =0xfe215040		/* AUX_MU_IO */
	adr	x1, banner
	bl	puts
	adr	x1, l_va;   ldr x0, [x19, #8];    bl line	/* virtBase */
	adr	x1, l_pa;   ldr x0, [x19, #16];   bl line	/* physBase */
	adr	x1, l_mem;  ldr x0, [x19, #24];   bl line	/* memSize */
	adr	x1, l_top;  ldr x0, [x19, #32];   bl line	/* topOfKernelData */
	adr	x1, l_cl;   bl puts
	add	x1, x19, #0x6c			/* CommandLine */
	bl	puts
	adr	x1, nl
	bl	puts
	/* touch RAM at the top of the kernel window: proves the memory is really there */
	ldr	x0, [x19, #16]
	ldr	x2, [x19, #24]
	add	x0, x0, x2
	sub	x0, x0, #8
	ldr	x3, =0x600df00d600df00d
	str	x3, [x0]
	ldr	x4, [x0]
	cmp	x3, x4
	adr	x1, ok
	adr	x2, bad
	csel	x1, x1, x2, eq
	bl	puts
	adr	x1, done
	bl	puts
1:	wfe
	b	1b
puts:	ldrb	w2, [x1], #1
	cbz	w2, 9f
2:	ldr	w3, [x20, #0x14]		/* AUX_MU_LSR */
	tbz	w3, #5, 2b
	str	w2, [x20]
	b	puts
9:	ret
line:	mov	x23, x30
	bl	puts
	mov	x5, #60
3:	lsr	x6, x0, x5
	and	x6, x6, #0xf
	cmp	x6, #10
	mov	x7, #'0'
	mov	x8, #('a' - 10)
	csel	x7, x7, x8, lt
	add	x2, x6, x7
4:	ldr	w3, [x20, #0x14]
	tbz	w3, #5, 4b
	str	w2, [x20]
	subs	x5, x5, #4
	b.ge	3b
	adr	x1, nl
	mov	x30, x23
	b	puts
banner:	.asciz	"selftest kernel running\r\n"
l_va:	.asciz	"virtBase "
l_pa:	.asciz	"physBase "
l_mem:	.asciz	"memSize  "
l_top:	.asciz	"topKData  "
l_cl:	.asciz	"cmdline: "
nl:	.asciz	"\r\n"
ok:	.asciz	"top-of-RAM write/read OK\r\n"
bad:	.asciz	"top-of-RAM write/read FAILED\r\n"
done:	.asciz	"SELFTEST DONE\r\n"
