/*
 * Mach semaphore probe (text_cmds sort keeps its temp-file list under one): create, signal, wait, destroy,
 * with every kern_return_t printed.
 */
#include <mach/mach.h>
#include <mach/semaphore.h>
#include <mach/task.h>
#include <stdio.h>

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
	return 0;
}
