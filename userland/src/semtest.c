/*
 * Mach semaphore probe (text_cmds sort keeps its temp-file list under one): create, signal, wait, destroy,
 * with every kern_return_t printed.
 */
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

int main(void)
{
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
