/*
 * Process 1, until a real launchd replaces it (docs/userland-plan.md section 4, option 1).
 *
 * Installed as /sbin/launchd, which is what the kernel executes. It
 *   - makes /dev/console the standard descriptors and says hello;
 *   - is the bootstrap server: it makes a port, installs it as its own bootstrap port (every process forked from
 *     it inherits that), and answers bootstrap_check_in / register / look_up (include/bootstrap_proto.h) between
 *     the polls of its child-reaping loop;
 *   - runs /etc/rc with /bin/sh when there is one;
 *   - keeps an interactive /bin/sh on the console, starting a new one when it exits;
 *   - reaps every child that is orphaned onto it.
 * It never exits: the kernel panics when process 1 does. SIGUSR1 reboots, SIGUSR2 halts.
 */
#include <errno.h>
#include <stdint.h>
#include <mach/mach.h>
#include "bootstrap_proto.h"
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static volatile sig_atomic_t want_reboot, want_halt;

/* Marks on the console before anything else, so a crash during start-up can be placed: libSystem has been
 * initialized once this runs, and main has been reached once the next one prints. */
static void
mark(const char *msg)
{
	int fd = open("/dev/console", O_WRONLY);

	if (fd >= 0) {
		write(fd, msg, strlen(msg));
		close(fd);
	}
}

__attribute__((constructor)) static void
early(void)
{
	mark("init: constructor (libSystem is initialized)\n");
}

static void
on_usr1(int sig)
{
	(void)sig;
	want_reboot = 1;
}

static void
on_usr2(int sig)
{
	(void)sig;
	want_halt = 1;
}

/* The kernel mounts the RAM disk root read-only; like launchd on macOS, remount it read-write (mount -uw /). */
static void
remount_root_rw(void)
{
	struct {
		char *fspec;
		uid_t hfs_uid;
		gid_t hfs_gid;
		mode_t hfs_mask;
		uint32_t hfs_encoding;
		struct timezone hfs_timezone;
		int flags;
		int journal_tbuffer_size;
		int journal_flags;
		int journal_disable;
	} args;

	memset(&args, 0, sizeof(args));
	/* the root is the RAM disk (rd=md0) or the SD card (rd=sdroot0); devfs has a node only for the one in use */
	args.fspec = access("/dev/sdroot0", F_OK) == 0 ? "/dev/sdroot0" : "/dev/md0";
	args.hfs_uid = (uid_t)-1;
	args.hfs_gid = (gid_t)-1;
	args.hfs_mask = (mode_t)-1;
	args.hfs_encoding = (uint32_t)-1;
	if (mount("hfs", "/", MNT_UPDATE, &args) == 0) {
		printf("init: / is read-write (%s)\n", args.fspec);
	} else {
		printf("init: remounting / read-write failed: %s\n", strerror(errno));
	}
}

static void
open_console(void)
{
	int fd = open("/dev/console", O_RDWR);

	if (fd < 0) {
		return;
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	if (fd > 2) {
		close(fd);
	}
}

static pid_t
spawn(const char *path, char *const argv[])
{
	pid_t pid = fork();

	if (pid == 0) {
		setsid();
		open_console();
		ioctl(0, TIOCSCTTY, 0);
		signal(SIGUSR1, SIG_DFL);
		signal(SIGUSR2, SIG_DFL);
		execv(path, argv);
		printf("init: cannot execute %s: %s\n", path, strerror(errno));
		_exit(127);
	}
	return pid;
}

/* a daemon: its own session, console output, but not the controlling terminal (the shell that follows must get it) */
static pid_t
spawn_daemon(const char *path, char *const argv[])
{
	pid_t pid = fork();

	if (pid == 0) {
		int fd;

		setsid();
		fd = open("/dev/console", O_RDWR | O_NOCTTY);
		if (fd >= 0) {
			dup2(fd, 0);
			dup2(fd, 1);
			dup2(fd, 2);
			if (fd > 2) {
				close(fd);
			}
		}
		signal(SIGUSR1, SIG_DFL);
		signal(SIGUSR2, SIG_DFL);
		execv(path, argv);
		printf("init: cannot execute %s: %s\n", path, strerror(errno));
		_exit(127);
	}
	return pid;
}

/* ---- bootstrap server ---- */

#define BS_MAX_SERVICES 64

static mach_port_t bootstrap_receive;
static struct {
	char name[BS_NAME_MAX];
	mach_port_t send;    /* MACH_PORT_NULL: the slot is free */
} services[BS_MAX_SERVICES];

/* slot of a live service called NAME, or -1; a service whose port has died is forgotten */
static int
bs_find(const char *name)
{
	for (int i = 0; i < BS_MAX_SERVICES; i++) {
		mach_port_type_t type;

		if (services[i].send == MACH_PORT_NULL || strcmp(services[i].name, name) != 0) {
			continue;
		}
		if (mach_port_type(mach_task_self(), services[i].send, &type) == KERN_SUCCESS &&
		    !(type & MACH_PORT_TYPE_DEAD_NAME)) {
			return i;
		}
		mach_port_deallocate(mach_task_self(), services[i].send);
		services[i].send = MACH_PORT_NULL;
		return -1;
	}
	return -1;
}

static int
bs_free_slot(void)
{
	for (int i = 0; i < BS_MAX_SERVICES; i++) {
		if (services[i].send == MACH_PORT_NULL) {
			return i;
		}
	}
	return -1;
}

/* answers one request; the right in REPLY (if any) goes to the sender, a right that arrived with it is kept
 * (BS_REGISTER) or dropped */
static void
bs_serve(bs_request_t *req, size_t size)
{
	bs_reply_t rep;
	mach_port_t reply_to = req->head.msgh_remote_port;
	mach_port_t carried = MACH_PORT_NULL;
	mach_port_t give = MACH_PORT_NULL;
	mach_msg_type_name_t how = MACH_MSG_TYPE_COPY_SEND;
	kern_return_t ret = KERN_SUCCESS;
	int slot = -1;
	bool well_formed = size == sizeof(*req) && (req->head.msgh_bits & MACH_MSGH_BITS_COMPLEX) &&
	    req->body.msgh_descriptor_count == 1;

	if (well_formed) {
		carried = req->port.name;
		req->name[BS_NAME_MAX - 1] = '\0';
	}
	if (!well_formed) {
		ret = KERN_INVALID_ARGUMENT;
	} else if (req->head.msgh_id == BS_LOOK_UP) {
		slot = bs_find(req->name);
		if (slot < 0) {
			ret = BS_UNKNOWN_SERVICE;
		} else {
			give = services[slot].send;
		}
	} else if (req->head.msgh_id == BS_CHECK_IN) {
		mach_port_t port;

		if (bs_find(req->name) >= 0) {
			ret = BS_SERVICE_ACTIVE;
		} else if ((slot = bs_free_slot()) < 0) {
			ret = KERN_RESOURCE_SHORTAGE;
		} else if (mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port) != KERN_SUCCESS) {
			ret = KERN_RESOURCE_SHORTAGE;
		} else if (mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND) != KERN_SUCCESS) {
			mach_port_mod_refs(mach_task_self(), port, MACH_PORT_RIGHT_RECEIVE, -1);
			ret = KERN_RESOURCE_SHORTAGE;
		} else {
			strlcpy(services[slot].name, req->name, sizeof(services[slot].name));
			services[slot].send = port;    /* the receive right goes to the caller below */
			give = port;
			how = MACH_MSG_TYPE_MOVE_RECEIVE;
		}
	} else if (req->head.msgh_id == BS_REGISTER) {
		if (carried == MACH_PORT_NULL || carried == MACH_PORT_DEAD) {
			ret = KERN_INVALID_ARGUMENT;
		} else if (bs_find(req->name) >= 0) {
			ret = BS_SERVICE_ACTIVE;
		} else if ((slot = bs_free_slot()) < 0) {
			ret = KERN_RESOURCE_SHORTAGE;
		} else {
			strlcpy(services[slot].name, req->name, sizeof(services[slot].name));
			services[slot].send = carried;
			carried = MACH_PORT_NULL;    /* kept */
		}
	} else {
		ret = KERN_INVALID_ARGUMENT;
	}
	if (carried != MACH_PORT_NULL && carried != MACH_PORT_DEAD) {
		mach_port_deallocate(mach_task_self(), carried);
	}
	if (reply_to == MACH_PORT_NULL) {
		return;
	}

	memset(&rep, 0, sizeof(rep));
	rep.head.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_MOVE_SEND_ONCE, 0) | MACH_MSGH_BITS_COMPLEX;
	rep.head.msgh_size = sizeof(rep);
	rep.head.msgh_remote_port = reply_to;
	rep.head.msgh_local_port = MACH_PORT_NULL;
	rep.head.msgh_id = req->head.msgh_id + BS_REPLY_ID;
	rep.body.msgh_descriptor_count = 1;
	rep.port.name = give;
	rep.port.disposition = how;
	rep.port.type = MACH_MSG_PORT_DESCRIPTOR;
	rep.ret = ret;
	if (mach_msg(&rep.head, MACH_SEND_MSG, sizeof(rep), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE,
	    MACH_PORT_NULL) != KERN_SUCCESS && how == MACH_MSG_TYPE_MOVE_RECEIVE) {
		/* the sender is gone and took the new port down with the message */
		mach_port_deallocate(mach_task_self(), services[slot].send);
		services[slot].send = MACH_PORT_NULL;
	}
}

/* serves at most one request, waiting at most MS milliseconds for it */
static void
bs_poll(mach_msg_timeout_t ms)
{
	bs_request_rcv_t buf;
	kern_return_t kr;

	if (bootstrap_receive == MACH_PORT_NULL) {
		usleep(ms * 1000);
		return;
	}
	kr = mach_msg(&buf.msg.head, MACH_RCV_MSG | MACH_RCV_TIMEOUT, 0, sizeof(buf), bootstrap_receive, ms, MACH_PORT_NULL);
	if (kr == KERN_SUCCESS) {
		bs_serve(&buf.msg, buf.msg.head.msgh_size);
	} else if (kr != MACH_RCV_TIMED_OUT && kr != MACH_RCV_INTERRUPTED) {
		/* too large and the like: the message is gone; do not spin if the port itself is broken */
		usleep(ms * 1000);
	}
}

static void
start_bootstrap_server(void)
{
	mach_port_t port;
	kern_return_t kr;

	kr = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port);
	if (kr == KERN_SUCCESS) {
		kr = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND);
	}
	if (kr == KERN_SUCCESS) {
		kr = task_set_bootstrap_port(mach_task_self(), port);
	}
	if (kr != KERN_SUCCESS) {
		printf("init: no bootstrap server: %s\n", mach_error_string(kr));
		return;
	}
	bootstrap_receive = port;
	printf("init: bootstrap server is up\n");
}

/* wait for PID, reaping whatever else exits meanwhile and serving bootstrap requests; returns the status of PID,
 * -1 if a reboot was asked for */
static int
wait_for(pid_t pid)
{
	for (;;) {
		int status;
		pid_t r = waitpid(-1, &status, WNOHANG);

		if (want_reboot || want_halt) {
			return -1;
		}
		if (r == pid) {
			return status;
		}
		if (r <= 0) {
			bs_poll(100);    /* also what paces this loop */
		}
	}
}

int
main(void)
{
	struct stat st;
	char *const rc_argv[] = { "sh", "/etc/rc", NULL };
	char *const sh_argv[] = { "-sh", NULL };

	mark("init: main reached\n");
	open_console();
	setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
	signal(SIGUSR1, on_usr1);
	signal(SIGUSR2, on_usr2);
	signal(SIGHUP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);

	printf("\ninit: process 1 (pid %d) is up\n", getpid());

	remount_root_rw();
	start_bootstrap_server();
	if (stat("/usr/sbin/notifyd", &st) == 0) {
		char *const nd_argv[] = { "notifyd", NULL };

		setenv("OS_LOG_STDERR", "1", 1);
		printf("init: starting notifyd (pid %d)\n", spawn_daemon("/usr/sbin/notifyd", nd_argv));
	}
	if (stat("/etc/rc", &st) == 0 && stat("/bin/sh", &st) == 0) {
		printf("init: running /etc/rc\n");
		wait_for(spawn("/bin/sh", rc_argv));
	}

	for (;;) {
		int status;

		if (want_reboot || want_halt) {
			break;
		}
		if (stat("/bin/sh", &st) != 0) {
			printf("init: no /bin/sh\n");
			sleep(5);
			continue;
		}
		status = wait_for(spawn("/bin/sh", sh_argv));
		if (status >= 0) {
			printf("init: shell exited (status 0x%x), restarting\n", status);
			sleep(1);
		}
	}
	sync();
	reboot(want_halt ? RB_HALT : RB_AUTOBOOT);
	for (;;) {
		sleep(60);
	}
}
