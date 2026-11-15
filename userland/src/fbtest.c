/*
 * fbtest: draw on the screen through the BCM2837Framebuffer IOKit service.
 *
 *   fbtest [hold]
 *
 * Maps the framebuffer, turns the kernel's console mirror off and draws a test picture: eight colour bars
 * across the top half, a horizontal grey gradient below, and a white square with a red centre at (64, 448).
 * Pixels are written as 0x00RRGGBB words. With `hold` it keeps the screen until killed (the boot test takes
 * its screendump then) and draws an orange 12x12 pointer, starting at the centre, that follows the reports of the
 * BCM2837Pointer service; without, it waits three seconds and gives the screen back to the console.
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include "iokit-lite/iokit_lite.h"

static const uint32_t bars[8] = {
	0x00ffffff, 0x00ffff00, 0x0000ffff, 0x0000ff00, 0x00ff00ff, 0x00ff0000, 0x000000ff, 0x00000000,
};

/* The test picture at (x, y). */
static uint32_t
picture(uint32_t x, uint32_t y, uint32_t w, uint32_t h)
{
	if (x >= 64 && x < 128 && y >= 448 && y < 512) {
		return (x >= 88 && x < 104 && y >= 472 && y < 488) ? 0x00ff0000 : 0x00ffffff;
	}
	if (y < h / 2) {
		return bars[x * 8 / w];
	}
	uint32_t g = x * 255 / (w - 1);
	return (g << 16) | (g << 8) | g;
}

#define POINTER_SIZE    12
#define POINTER_COLOUR  0x00ff8000

static void
pointer_paint(void *base, uint32_t w, uint32_t h, uint32_t pitch, int px, int py, int show)
{
	for (int y = py; y < py + POINTER_SIZE; y++) {
		for (int x = px; x < px + POINTER_SIZE; x++) {
			if (x >= 0 && y >= 0 && (uint32_t)x < w && (uint32_t)y < h) {
				uint32_t *row = (uint32_t *)((uint8_t *)base + (size_t)y * pitch);
				row[x] = show ? POINTER_COLOUR : picture((uint32_t)x, (uint32_t)y, w, h);
			}
		}
	}
}

/* Layout of the kernel's IOSharedDataQueue memory and of the entries BCM2837Pointer puts in it. */
struct queue_memory {
	uint32_t queue_size;
	volatile uint32_t head;
	volatile uint32_t tail;
	uint8_t queue[];
};
struct pointer_event {
	int32_t dx, dy;
	uint32_t buttons;
	uint32_t reserved;
	uint64_t timestamp;
};

/* Take one entry off the queue (same rules as IOKit's IODataQueueDequeue). */
static int
queue_next(struct queue_memory *q, struct pointer_event *ev)
{
	uint32_t head = q->head, tail = q->tail;
	if (head == tail) {
		return 0;
	}
	uint32_t size, at = head;
	if (head + 4 <= q->queue_size) {
		memcpy(&size, q->queue + head, 4);
	}
	if (head + 4 > q->queue_size || head + 4 + size > q->queue_size) {
		at = 0;                 /* the entry wrapped to the start */
		memcpy(&size, q->queue, 4);
	}
	if (size == sizeof(*ev)) {
		memcpy(ev, q->queue + at + 4, sizeof(*ev));
	}
	q->head = at + 4 + size;
	return size == sizeof(*ev);
}

/* Keep the screen; follow the pointer device if there is one. */
static void
hold(void *base, uint32_t w, uint32_t h, uint32_t pitch)
{
	mach_port_t service, connect;
	struct queue_memory *q = NULL;
	size_t qsize = 0;
	void *qaddr = NULL;
	if (iol_find_service("BCM2837Pointer", &service) == KERN_SUCCESS &&
	    iol_open(service, 0, &connect) == KERN_SUCCESS &&
	    iol_map(connect, 0, &qaddr, &qsize) == KERN_SUCCESS) {
		q = qaddr;
		printf("fbtest: pointer queue mapped, %u bytes\n", q->queue_size);
	} else {
		printf("fbtest: no pointer device\n");
	}
	fflush(stdout);
	int px = (int)w / 2, py = (int)h / 2;
	pointer_paint(base, w, h, pitch, px, py, 1);
	for (;;) {
		struct pointer_event ev;
		while (q != NULL && queue_next(q, &ev)) {
			pointer_paint(base, w, h, pitch, px, py, 0);
			px += ev.dx;
			py += ev.dy;
			px = px < 0 ? 0 : (px > (int)w - POINTER_SIZE ? (int)w - POINTER_SIZE : px);
			py = py < 0 ? 0 : (py > (int)h - POINTER_SIZE ? (int)h - POINTER_SIZE : py);
			pointer_paint(base, w, h, pitch, px, py, 1);
			printf("fbtest: pointer at %d %d buttons %u\n", px, py, ev.buttons);
			fflush(stdout);
		}
		usleep(16000);
	}
}

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
			row[x] = picture(x, y, w, h);
		}
	}
	printf("fbtest: drew bars, gradient and marker\n");
	fflush(stdout);

	if (argc > 1 && strcmp(argv[1], "hold") == 0) {
		hold(base, w, h, pitch);
	}
	sleep(3);
	iol_close(connect);     /* the kernel turns the console mirror back on */
	printf("fbtest: done\n");
	return 0;
}
