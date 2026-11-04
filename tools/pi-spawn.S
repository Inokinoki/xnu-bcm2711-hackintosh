/*
 * A static PID 1 for debugging the dynamic userland (docs/userland-plan.md): runs /usr/bin/smoke and then
 * /sbin/init-real (our real init) as children, each with dyld's DYLD_PRINT_* variables set and the console as
 * stdin/stdout/stderr, prints how each one ended (the wait4 status: 0x0 = exit 0, 0xb = SIGSEGV ...) and then
 * stays alive. Unlike a crashing PID 1 (a kernel panic) a crashing child leaves the dyld log on the console.
 * Built like tools/pi-init.S (tools/mkpi-init.sh with SRC=pi-spawn.S): position independent, no relocations,
 * no writable data. Raw XNU syscalls: number in x16, svc #0x80.
 */
#define SYS_exit	1
#define SYS_fork	2
#define SYS_write	4
#define SYS_open	5
#define SYS_wait4	7
#define SYS_execve	59
#define SYS_dup2	90
#define SYS_select	93
#define O_RDWR		2

	.text
	.globl	_start
	.p2align 2
_start:
	mov	x9, sp
	and	x9, x9, #~15
	mov	sp, x9

	adr	x0, console
	mov	x1, #O_RDWR
	mov	x2, #0
	mov	x16, #SYS_open
	svc	#0x80
	b.cs	hang
	mov	x19, x0
	mov	x1, #0
	bl	dup_fd
	mov	x1, #1
	bl	dup_fd
	mov	x1, #2
	bl	dup_fd

	adr	x0, smoke
	bl	run
	adr	x0, real_init
	bl	run

	adr	x0, done
	bl	say
idle:
	mov	x0, #0
	mov	x1, #0
	mov	x2, #0
	mov	x3, #0
	adr	x4, five_sec
	mov	x16, #SYS_select
	svc	#0x80
	b	idle

hang:
	b	hang

/* dup2(x19, x1) */
dup_fd:
	mov	x0, x19
	mov	x16, #SYS_dup2
	svc	#0x80
	ret

/* write the NUL-terminated string at x0 to the console */
say:
	mov	x1, x0
	mov	x2, #0
1:	ldrb	w3, [x1, x2]
	cbz	w3, 2f
	add	x2, x2, #1
	b	1b
2:	mov	x0, x19
	mov	x16, #SYS_write
	svc	#0x80
	ret

/* run the program at x0 with the DYLD_PRINT_* environment, wait for it, print its status */
run:
	stp	x29, x30, [sp, #-48]!
	mov	x29, sp
	str	x20, [sp, #16]
	mov	x20, x0

	adr	x0, running
	bl	say
	mov	x0, x20
	bl	say
	adr	x0, newline
	bl	say

	mov	x16, #SYS_fork
	svc	#0x80
	cbnz	x1, child		/* x1 = 1 in the child, 0 in the parent */

	/* parent: x0 = child pid; wait4(pid, &status, 0, NULL) */
	str	wzr, [sp, #32]
	add	x1, sp, #32
	mov	x2, #0
	mov	x3, #0
	mov	x16, #SYS_wait4
	svc	#0x80
	adr	x0, status_msg
	bl	say
	ldr	w21, [sp, #32]
	bl	hex32
	adr	x0, newline
	bl	say
	ldr	x20, [sp, #16]
	ldp	x29, x30, [sp], #48
	ret

child:
	sub	sp, sp, #64
	/* argv at sp: { path, NULL }; envp at sp+16: { 4 strings, NULL } */
	str	x20, [sp, #0]
	str	xzr, [sp, #8]
	adr	x2, env0
	adr	x3, env1
	stp	x2, x3, [sp, #16]
	adr	x2, env2
	adr	x3, env3
	stp	x2, x3, [sp, #32]
	str	xzr, [sp, #48]
	mov	x0, x20
	mov	x1, sp
	add	x2, sp, #16
	mov	x16, #SYS_execve
	svc	#0x80
	adr	x0, exec_failed
	bl	say
	mov	x0, #127
	mov	x16, #SYS_exit
	svc	#0x80
	b	hang

/* print w21 as 0x%08x */
hex32:
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp
	mov	w4, #'0'
	strb	w4, [sp, #16]
	mov	w4, #'x'
	strb	w4, [sp, #17]
	mov	x5, #0
1:	lsr	w6, w21, #28
	and	w6, w6, #15
	cmp	w6, #10
	b.lt	2f
	add	w6, w6, #('a' - 10 - '0')
2:	add	w6, w6, #'0'
	add	x7, sp, #18
	strb	w6, [x7, x5]
	lsl	w21, w21, #4
	add	x5, x5, #1
	cmp	x5, #8
	b.lt	1b
	add	x1, sp, #16
	mov	x2, #10
	mov	x0, x19
	mov	x16, #SYS_write
	svc	#0x80
	ldp	x29, x30, [sp], #32
	ret

	.p2align 3
five_sec:
	.quad	5
	.quad	0
console:
	.asciz	"/dev/console"
smoke:
	.asciz	"/usr/bin/smoke"
real_init:
	.asciz	"/sbin/init-real"
env0:
	.asciz	"DYLD_PRINT_LIBRARIES=1"
env1:
	.asciz	"DYLD_PRINT_INITIALIZERS=1"
env2:
	.asciz	"DYLD_PRINT_SEGMENTS=1"
env3:
	.asciz	"OS_LOG_STDERR=1"
running:
	.asciz	"\nspawn: running "
status_msg:
	.asciz	"spawn: child ended, wait4 status "
exec_failed:
	.asciz	"spawn: execve failed\n"
newline:
	.asciz	"\n"
done:
	.asciz	"spawn: all children done\n"
