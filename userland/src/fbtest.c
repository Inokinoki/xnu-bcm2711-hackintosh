/*
 * fbtest: draw on the screen through the BCM2837Framebuffer IOKit service.
 *
 *   fbtest [hold]
 *
 * Maps the framebuffer, turns the kernel's console mirror off and draws a test picture: eight colour bars
 * across the top half, a horizontal grey gradient below, and a white square with a red centre at (64, 448).
 * Pixels are written as 0x00RRGGBB words. With `hold` it keeps the screen until killed (the boot test takes
 * its screendump then); without, it waits three seconds and gives the screen back to the console.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "iokit-lite/iokit_lite.h"

static const uint32_t bars[8] = {
	0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00, 0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00000000,
};

int
main(int argc, char **argv)
{
	mach_port_t service, connect;
	kern_return_t kr = iol_find_service("BCM2837Framebuffer", &service);
	if (kr != KERN_SUCCESS) {
		printf("fbtest: no BCM2837Framebuffer service (0x%x)\n", kr);
		return 1;
	}
	if ((kr = iol_open(service, 0, &connect)) != KERN_SUCCESS) {
		printf("fbtest: open failed (0x%x)\n", kr);
		return 1;
	}
	uint64_t info[4] = { 0 };
	uint32_t n = 4;
	if ((kr = iol_call(connect, 0, NULL, 0, info, &n)) != KERN_SUCCESS || n != 4) {
		printf("fbtest: GetInfo failed (0x%x, %u values)\n", kr, n);
		return 1;
	}
	uint32_t w = (uint32_t)info[0], h = (uint32_t)info[1], pitch = (uint32_t)info[2], depth = (uint32_t)info[3];
	void *base = NULL;
	size_t size = 0;
	if ((kr = iol_map(connect, 0, &base, &size)) != KERN_SUCCESS) {
		printf("fbtest: map failed (0x%x)\n", kr);
		return 1;
	}
	printf("fbtest: mapped %ux%u pitch %u depth %u, %zu bytes at %p\n", w, h, pitch, depth, size, base);
	if (depth != 32 || (size_t)pitch * h > size) {
		printf("fbtest: unsupported geometry\n");
		return 1;
	}
	uint64_t off = 0;
	iol_call(connect, 1, &off, 1, NULL, NULL);

	for (uint32_t y = 0; y < h; y++) {
		uint32_t *row = (uint32_t *)((uint8_t *)base + (size_t)y * pitch);
		for (uint32_t x = 0; x < w; x++) {
			if (y < h / 2) {
				row[x] = bars[x * 8 / w];
			} else {
				uint32_t g = x * 255 / (w - 1);
				row[x] = (g << 16) | (g << 8) | g;
			}
		}
	}
	for (uint32_t y = 448; y < 512 && y < h; y++) {
		uint32_t *row = (uint32_t *)((uint8_t *)base + (size_t)y * pitch);
		for (uint32_t x = 64; x < 128 && x < w; x++) {
			int centre = x >= 88 && x < 104 && y >= 472 && y < 488;
			row[x] = centre ? 0x00ff0000 : 0x00ffffff;
		}
	}
	printf("fbtest: drew bars, gradient and marker\n");
	fflush(stdout);

	if (argc > 1 && strcmp(argv[1], "hold") == 0) {
		for (;;) {
			sleep(3600);
		}
	}
	sleep(3);
	iol_close(connect);     /* the kernel turns the console mirror back on */
	printf("fbtest: done\n");
	return 0;
}
