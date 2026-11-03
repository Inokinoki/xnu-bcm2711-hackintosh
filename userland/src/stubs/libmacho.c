/*
 * libmacho (cctools, closed here): the section lookups libSystem and libpthread use to find data in an image's own
 * Mach-O header. Written from <mach-o/loader.h>; no dyld calls, the slide follows from the header's address.
 */
#include <mach-o/loader.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

static intptr_t
slide_of(const struct mach_header_64 *mh)
{
	const struct load_command *lc = (const void *)(mh + 1);

	for (uint32_t i = 0; i < mh->ncmds; i++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sg = (const void *)lc;

			if (strcmp(sg->segname, "__TEXT") == 0) {
				return (intptr_t)mh - (intptr_t)sg->vmaddr;
			}
		}
	}
	return 0;
}

const struct section_64 *
getsectbynamefromheader_64(const struct mach_header_64 *mh, const char *segname, const char *sectname)
{
	const struct load_command *lc = (const void *)(mh + 1);

	for (uint32_t i = 0; i < mh->ncmds; i++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sg = (const void *)lc;
			const struct section_64 *s = (const void *)(sg + 1);

			for (uint32_t j = 0; j < sg->nsects; j++, s++) {
				if (strncmp(s->segname, segname, 16) == 0 && strncmp(s->sectname, sectname, 16) == 0) {
					return s;
				}
			}
		}
	}
	return NULL;
}

uint8_t *
getsectiondata(const struct mach_header_64 *mh, const char *segname, const char *sectname, unsigned long *size)
{
	const struct section_64 *s = getsectbynamefromheader_64(mh, segname, sectname);

	if (!s) {
		return NULL;
	}
	if (size) {
		*size = (unsigned long)s->size;
	}
	return (uint8_t *)(uintptr_t)(s->addr + slide_of(mh));
}

uint8_t *
getsegmentdata(const struct mach_header_64 *mh, const char *segname, unsigned long *size)
{
	const struct load_command *lc = (const void *)(mh + 1);

	for (uint32_t i = 0; i < mh->ncmds; i++, lc = (const void *)((const char *)lc + lc->cmdsize)) {
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sg = (const void *)lc;

			if (strncmp(sg->segname, segname, 16) == 0) {
				if (size) {
					*size = (unsigned long)sg->vmsize;
				}
				return (uint8_t *)(uintptr_t)(sg->vmaddr + slide_of(mh));
			}
		}
	}
	return NULL;
}
