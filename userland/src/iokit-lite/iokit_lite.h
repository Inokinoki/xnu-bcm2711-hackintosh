/*
 * Minimal IOKit client calls for this system: no IOKit.framework, no CoreFoundation. Thin wrappers around the
 * MIG stubs generated from xnu's osfmk/device/device.defs (userland/build-fbtest.sh).
 */
#ifndef IOKIT_LITE_H
#define IOKIT_LITE_H
#include <mach/mach.h>
#include <stdint.h>
#include <stddef.h>

/* First registered service whose class is (or inherits from) class_name. */
kern_return_t iol_find_service(const char *class_name, mach_port_t *service);
kern_return_t iol_open(mach_port_t service, uint32_t type, mach_port_t *connect);
kern_return_t iol_close(mach_port_t connect);
/* Map the connection's memory of the given type anywhere in this task. */
kern_return_t iol_map(mach_port_t connect, uint32_t type, void **addr, size_t *size);
/* Scalar-only external method. *out_cnt is the capacity on entry and the count on return. */
kern_return_t iol_call(mach_port_t connect, uint32_t selector, const uint64_t *in, uint32_t in_cnt,
    uint64_t *out, uint32_t *out_cnt);
#endif
