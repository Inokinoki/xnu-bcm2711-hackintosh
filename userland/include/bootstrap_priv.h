/*
 * bootstrap_priv.h is launchd-private. Libinfo and Libnotify only use bootstrap_look_up2 with the privileged flag;
 * the implementation is in the libxpc stand-in (src/stubs/libxpc.c).
 */
#ifndef __BOOTSTRAP_PRIV_STANDIN_H__
#define __BOOTSTRAP_PRIV_STANDIN_H__
#include <mach/mach.h>
#include <servers/bootstrap.h>

#define BOOTSTRAP_PRIVILEGED_SERVER 0x1

__BEGIN_DECLS
kern_return_t bootstrap_look_up2(mach_port_t bp, const name_t service_name, mach_port_t *sp, pid_t target_pid, uint64_t flags);
__END_DECLS
#endif
