/*
 * Stage A smoke test: exercises what plain arm64 for the Cortex-A72 must not get wrong
 * (atomics, a function pointer call, a C++-free libSystem import) so the gate has
 * something to look at. Not part of the root image.
 */
#include <stdatomic.h>
#include <stdio.h>

static _Atomic int counter;

static int bump(int n)
{
	return atomic_fetch_add(&counter, n) + 1;
}

int main(void)
{
	int (*f)(int) = bump;
	int old = 0;

	atomic_compare_exchange_strong(&counter, &old, 5);
	printf("smoke %d\n", f(2));
	return 0;
}
