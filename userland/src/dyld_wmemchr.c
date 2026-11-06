/*
 * dyld links the cut-down libc_dyld.a, which has no wmemchr. Xcode 16's libc++ headers make dyld4's ProcessAtlas.o
 * reference it (std::wstring code), so give the dyld executable its own copy.
 */
#include <stddef.h>
#include <wchar.h>

__attribute__((visibility("hidden"))) wchar_t *
wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
	for (; n > 0; n--, s++) {
		if (*s == c) {
			return (wchar_t *)s;
		}
	}
	return NULL;
}
