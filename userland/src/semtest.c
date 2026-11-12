/*
 * Mach semaphore probe (text_cmds sort keeps its temp-file list under one): create, signal, wait, destroy,
 * with every kern_return_t printed.
 */
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <mach/vm_map.h>
#include <servers/bootstrap.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/mman.h>
#include <locale.h>
#include <malloc/malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <notify.h>
#include <errno.h>

/* malloc alignment probe, run first so the heap is as fresh as sort's: sort's item blocks come back 12 mod 16 */
static void *probe[64];
static size_t probe_sz[64];

static void alloc_probe(void)
{
	static const size_t pattern[] = { 11, 16, 24, 16, 32, 16, 61, 16, 16, 16, 48, 16, 8, 16, 100, 16 };
	for (int i = 0; i < 64; i++) {
		probe_sz[i] = pattern[i % 16];
		probe[i] = malloc(probe_sz[i]);
	}
	int misaligned = 0;
	for (int i = 0; i < 64; i++) {
		if ((unsigned long)probe[i] % 16) misaligned++;
		if (i < 24)
			printf("semtest: malloc(%zu) = %p (%% 16 = %lu) size %zu\n", probe_sz[i], probe[i],
			    (unsigned long)probe[i] % 16, malloc_size(probe[i]));
	}
	printf("semtest: %d of 64 pointers not 16-byte aligned\n", misaligned);
}

/* Addresses past the user pmap (39 bits here) must fail with an error, not take the kernel down. */
static void vm_range_probe(void)
{
	void *hi = (void *)0x600000000000ULL;	/* libmalloc nano v2's region */
	size_t len = 0x10000000;
	void *r = mmap(hi, len, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE | MAP_FIXED, -1, 0);
	printf("semtest: mmap(%p) -> %p\n", hi, r);
	int rc = munmap(hi, len);
	printf("semtest: munmap(%p) -> %d\n", hi, rc);
	vm_address_t a = (vm_address_t)hi;
	kern_return_t kr = vm_deallocate(mach_task_self(), a, len);
	printf("semtest: vm_deallocate -> %d\n", kr);
	vm_size_t sz = 0x4000;
	a = (vm_address_t)hi;
	kr = vm_allocate(mach_task_self(), &a, sz, VM_FLAGS_FIXED);
	printf("semtest: vm_allocate(fixed) -> %d\n", kr);
	printf("semtest: vm range probe done\n");
}

/* the bootstrap server in /sbin/launchd: check in, look up from this process and from a forked child, a message
 * from the child to the checked-in port, the failure cases */
static void
bootstrap_probe(void)
{
	struct { mach_msg_header_t h; char text[16]; mach_msg_trailer_t t; } m;
	mach_port_t bp = MACH_PORT_NULL, svc = MACH_PORT_NULL, again = MACH_PORT_NULL, found = MACH_PORT_NULL;
	kern_return_t kr;
	pid_t pid;

	kr = task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bp);
	printf("semtest: bootstrap port -> %d (%s)\n", kr, bp != MACH_PORT_NULL ? "set" : "null");
	kr = bootstrap_check_in(bp, "org.giskard.semtest", &svc);
	printf("semtest: check_in -> %d (%s)\n", kr, svc != MACH_PORT_NULL ? "port" : "no port");
	kr = bootstrap_check_in(bp, "org.giskard.semtest", &again);
	printf("semtest: check_in again -> %d (expect 1103)\n", kr);
	kr = bootstrap_look_up(bp, "org.giskard.semtest", &found);
	printf("semtest: look_up -> %d (%s)\n", kr, found != MACH_PORT_NULL ? "port" : "no port");
	kr = bootstrap_look_up(bp, "org.giskard.nothing", &found);
	printf("semtest: look_up unknown -> %d (expect 1102)\n", kr);

	pid = fork();
	if (pid == 0) {
		mach_port_t to = MACH_PORT_NULL;
		mach_msg_header_t h = { 0 };

		if (bootstrap_look_up(bootstrap_port, "org.giskard.semtest", &to) != KERN_SUCCESS) {
			_exit(2);
		}
		h.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
		h.msgh_size = sizeof(h);
		h.msgh_remote_port = to;
		h.msgh_id = 4242;
		_exit(mach_msg(&h, MACH_SEND_MSG, sizeof(h), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL) ? 3 : 0);
	}
	if (pid > 0 && svc != MACH_PORT_NULL) {
		int status = -1;

		kr = mach_msg(&m.h, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0, sizeof(m), svc, 10000, MACH_PORT_NULL);
		waitpid(pid, &status, 0);
		printf("semtest: message from child -> %d id %d (child status %d)\n", kr, kr ? 0 : m.h.msgh_id, status);
	}
	printf("semtest: bootstrap probe done\n");
}

/* notifyd: wait for its service, then post from a child and watch the token change here */
static void
notify_probe(void)
{
	mach_port_t np = MACH_PORT_NULL;
	int token = -1, check = -1, changed = -1;
	uint32_t st;
	pid_t pid;

	for (int i = 0; i < 100 && np == MACH_PORT_NULL; i++) {
		if (bootstrap_look_up(bootstrap_port, "com.apple.system.notification_center", &np) != KERN_SUCCESS) {
			usleep(100000);
		}
	}
	printf("semtest: notifyd service %s\n", np != MACH_PORT_NULL ? "found" : "NOT found");
	st = notify_register_check("org.giskard.notifytest", &token);
	printf("semtest: notify_register_check -> %u\n", st);
	if (st == NOTIFY_STATUS_OK) {
		st = notify_check(token, &check);     /* the first check always reports a change */
		printf("semtest: notify_check first -> %u changed %d\n", st, check);
		notify_check(token, &check);
		pid = fork();
		if (pid == 0) {
			uint32_t r = notify_post("org.giskard.notifytest");

			printf("semtest: child notify_post -> %u (errno %d)\n", r, errno);
			_exit(r == NOTIFY_STATUS_OK ? 0 : 1);
		}
		if (pid > 0) {
			int status = -1;

			waitpid(pid, &status, 0);
			printf("semtest: notify_post in child -> status %d\n", status);
		}
		st = notify_check(token, &changed);
		printf("semtest: notify_check after post -> %u changed %d\n", st, changed);
	}
	printf("semtest: notify probe done\n");
}

int main(int argc, char **argv)
{
	if (argc > 1 && !strcmp(argv[1], "bootstrap")) {
		bootstrap_probe();
		return 0;
	}
	if (argc > 1 && !strcmp(argv[1], "notify")) {
		notify_probe();
		return 0;
	}
	vm_range_probe();
	if (argc > 1 && !strcmp(argv[1], "vm"))
		return 0;
	alloc_probe();
	semaphore_t s = 0;
	kern_return_t kr;

	kr = semaphore_create(mach_task_self(), &s, SYNC_POLICY_FIFO, 1);
	printf("semtest: create kr=%d sem=0x%x\n", kr, s);
	kr = semaphore_wait(s);
	printf("semtest: wait kr=%d\n", kr);
	kr = semaphore_signal(s);
	printf("semtest: signal kr=%d\n", kr);
	kr = semaphore_destroy(mach_task_self(), s);
	printf("semtest: destroy kr=%d\n", kr);

	/* what sort relies on: the locale state and a heap that does not hand out overlapping blocks */
	printf("semtest: MB_CUR_MAX before setlocale %d\n", (int)MB_CUR_MAX);
	printf("semtest: setlocale -> %s\n", setlocale(LC_ALL, ""));
	printf("semtest: MB_CUR_MAX after %d, sizeof(wchar_t) %d\n", (int)MB_CUR_MAX, (int)sizeof(wchar_t));

	enum { N = 2000 };
	static unsigned char *p[N];
	static size_t sz[N];
	int bad = 0;
	unsigned seed = 1;
	for (int i = 0; i < N; i++) {
		seed = seed * 1103515245 + 12345;
		sz[i] = 1 + (seed >> 16) % 300;
		p[i] = malloc(sz[i]);
		if (!p[i]) { bad++; continue; }
		memset(p[i], i & 0xff, sz[i]);
		if (i % 3 == 0 && i) {
			size_t n = sz[i / 2] + 64; /* grow some earlier block */
			unsigned char *q = realloc(p[i / 2], n);
			if (q) { memset(q, (i / 2) & 0xff, n); p[i / 2] = q; sz[i / 2] = n; }
		}
	}
	for (int i = 0; i < N; i++)
		for (size_t j = 0; p[i] && j < sz[i]; j++)
			if (p[i][j] != (i & 0xff)) { bad++; break; }
	printf("semtest: heap check: %d bad blocks of %d\n", bad, N);

	wchar_t w[8];
	mbstowcs(w, "# User", 8);
	printf("semtest: mbstowcs -> %x %x %x\n", (unsigned)w[0], (unsigned)w[1], (unsigned)w[2]);
	return 0;
}
