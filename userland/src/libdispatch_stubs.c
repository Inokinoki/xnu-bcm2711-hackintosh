/*
 * Symbols libdispatch gets from the private parts of macOS that the headers and libraries we build against do not
 * have: pthread_priority_t helpers (inline in xnu's pthread/priority_private.h, which Apple's SDK has and ours lacks)
 * and the work interval instance calls (libsystem_kernel). Workgroup intervals are Apple-silicon scheduling hints:
 * the instance calls succeed without doing anything.
 */
#include <stdbool.h>
#include <stdlib.h>

typedef unsigned long pthread_priority_t;
extern unsigned int _pthread_qos_class_decode(pthread_priority_t pp, int *relpri, unsigned long *flags);

bool
_pthread_priority_has_qos(pthread_priority_t pp)
{
	return _pthread_qos_class_decode(pp, NULL, NULL) != 0; /* QOS_CLASS_UNSPECIFIED */
}

int
_pthread_priority_relpri(pthread_priority_t pp)
{
	int relpri = 0;
	(void)_pthread_qos_class_decode(pp, &relpri, NULL);
	return relpri;
}

void *
work_interval_instance_alloc(void *wi)
{
	(void)wi;
	return malloc(1);
}

void
work_interval_instance_free(void *wii)
{
	free(wii);
}

int work_interval_instance_clear(void *wii) { (void)wii; return 0; }
int work_interval_instance_set_start(void *wii, unsigned long long t) { (void)wii; (void)t; return 0; }
int work_interval_instance_set_finish(void *wii, unsigned long long t) { (void)wii; (void)t; return 0; }
int work_interval_instance_set_deadline(void *wii, unsigned long long t) { (void)wii; (void)t; return 0; }
int work_interval_instance_set_complexity(void *wii, unsigned int c) { (void)wii; (void)c; return 0; }
int work_interval_instance_start(void *wii) { (void)wii; return 0; }
int work_interval_instance_update(void *wii) { (void)wii; return 0; }
int work_interval_instance_finish(void *wii) { (void)wii; return 0; }
