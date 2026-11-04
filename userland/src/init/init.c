/*
 * Process 1, until a real launchd replaces it (docs/userland-plan.md section 4, option 1).
 *
 * Installed as /sbin/launchd, which is what the kernel executes. It
 *   - makes /dev/console the standard descriptors and says hello;
 *   - runs /etc/rc with /bin/sh when there is one;
 *   - keeps an interactive /bin/sh on the console, starting a new one when it exits;
 *   - reaps every child that is orphaned onto it.
 * It never exits: the kernel panics when process 1 does. SIGUSR1 reboots, SIGUSR2 halts.
 */
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
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
	args.fspec = "/dev/md0";
	args.hfs_uid = (uid_t)-1;
	args.hfs_gid = (gid_t)-1;
	args.hfs_mask = (mode_t)-1;
	args.hfs_encoding = (uint32_t)-1;
	if (mount("hfs", "/", MNT_UPDATE, &args) == 0) {
		printf("init: / is read-write\n");
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

/* wait for PID, reaping whatever else exits meanwhile; returns the status of PID, -1 if a reboot was asked for */
static int
wait_for(pid_t pid)
{
	for (;;) {
		int status;
		pid_t r = wait(&status);

		if (want_reboot || want_halt) {
			return -1;
		}
		if (r == pid) {
			return status;
		}
		if (r < 0 && errno != EINTR) {
			sleep(1);
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
