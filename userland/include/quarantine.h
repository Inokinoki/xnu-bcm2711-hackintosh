/*
 * quarantine.h belongs to libquarantine (closed). No file is ever quarantined here: copyfile.c gets the same
 * no-op functions it has for iOS (its TARGET_OS_IPHONE branch), plus the constant it reads from the real header.
 */
#ifndef __QUARANTINE_STANDIN_H__
#define __QUARANTINE_STANDIN_H__
#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

#define qtn_file_t void *
#define QTN_SERIALIZED_DATA_MAX 0
#define QTN_FLAG_DO_NOT_TRANSLOCATE 0
#define qtn_xattr_name "com.apple.quarantine"
static void *qtn_file_alloc(void) { return NULL; }
static int qtn_file_init_with_fd(void *x, int y) { (void)x; (void)y; return -1; }
static int qtn_file_init_with_path(void *x, const char *path) { (void)x; (void)path; return -1; }
static int qtn_file_init_with_data(void *x, const void *data, size_t len) { (void)x; (void)data; (void)len; return -1; }
static void qtn_file_free(void *x) { (void)x; }
static int qtn_file_apply_to_fd(void *x, int y) { (void)x; (void)y; return 0; }
static char *qtn_error(int x) { (void)x; return NULL; }
static int qtn_file_to_data(void *x, char *y, size_t *z) { (void)x; (void)y; (void)z; return -1; }
static void *qtn_file_clone(void *x) { (void)x; return NULL; }
static uint32_t qtn_file_get_flags(void *x) { (void)x; return 0; }
static int qtn_file_set_flags(void *x, uint32_t flags) { (void)x; (void)flags; return 0; }
#endif
