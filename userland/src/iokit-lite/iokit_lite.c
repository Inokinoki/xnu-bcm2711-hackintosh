#include "iokit_lite.h"
#include "iokitmig.h"
#include <stdio.h>
#include <string.h>
#include <mach/mach_host.h>

static mach_port_t
iol_main_port(void)
{
	static mach_port_t port = MACH_PORT_NULL;
	if (port == MACH_PORT_NULL) {
		host_get_io_main(mach_host_self(), &port);
	}
	return port;
}

kern_return_t
iol_find_service(const char *class_name, mach_port_t *service)
{
	/* the kernel parses this with OSUnserializeXML */
	char matching[512];
	int n = snprintf(matching, sizeof(matching),
	    "<dict><key>IOProviderClass</key><string>%s</string></dict>", class_name);
	if (n < 0 || (size_t)n >= sizeof(matching)) {
		return KERN_INVALID_ARGUMENT;
	}
	*service = MACH_PORT_NULL;
	kern_return_t kr = io_service_get_matching_service_bin(iol_main_port(), matching, (mach_msg_type_number_t)n + 1, service);
	if (kr == KERN_SUCCESS && *service == MACH_PORT_NULL) {
		kr = KERN_FAILURE;      /* no such service */
	}
	return kr;
}

kern_return_t
iol_open(mach_port_t service, uint32_t type, mach_port_t *connect)
{
	kern_return_t result = KERN_FAILURE;
	kern_return_t kr = io_service_open_extended(service, mach_task_self(), type, NDR_record, NULL, 0, &result, connect);
	return kr != KERN_SUCCESS ? kr : result;
}

kern_return_t
iol_close(mach_port_t connect)
{
	kern_return_t kr = io_service_close(connect);
	mach_port_deallocate(mach_task_self(), connect);
	return kr;
}

kern_return_t
iol_map(mach_port_t connect, uint32_t type, void **addr, size_t *size)
{
	mach_vm_address_t a = 0;
	mach_vm_size_t s = 0;
	kern_return_t kr = io_connect_map_memory_into_task(connect, type, mach_task_self(), &a, &s, 1 /* kIOMapAnywhere */);
	if (kr == KERN_SUCCESS) {
		*addr = (void *)(uintptr_t)a;
		*size = (size_t)s;
	}
	return kr;
}

kern_return_t
iol_call(mach_port_t connect, uint32_t selector, const uint64_t *in, uint32_t in_cnt, uint64_t *out, uint32_t *out_cnt)
{
	char inband_out[1];
	mach_msg_type_number_t inband_out_cnt = 0, scalar_out_cnt = out_cnt ? *out_cnt : 0;
	mach_vm_size_t ool_out_size = 0;
	uint64_t none[1] = { 0 };
	kern_return_t kr = io_connect_method(connect, selector, (uint64_t *)(in ? in : none), in_cnt, NULL, 0, 0, 0,
	    inband_out, &inband_out_cnt, out ? out : none, &scalar_out_cnt, 0, &ool_out_size);
	if (out_cnt) {
		*out_cnt = scalar_out_cnt;
	}
	return kr;
}
