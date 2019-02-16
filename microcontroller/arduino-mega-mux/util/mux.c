#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <fcntl.h>
#include <termios.h>
#include <linux/gsmmux.h>
#include <linux/tty.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#define DEFAULT_SPEED	B115200
#define SERIAL_PORT	"/dev/ttyACM0"
#define GSM_PREFIX "/dev/ttyGSM"
#define NUM_MUX 4

int verbose = 0;
int daemonize = 0;

static struct option options[] = {
	{"tty", required_argument, NULL, 0},
	{"baud", required_argument, NULL, 0},
	{"gsm", required_argument, NULL, 0},
	{"num", required_argument, NULL, 0},
	{"daemon", no_argument, &daemonize, 1},
	{"verbose", no_argument, &verbose, 1},
	{0, 0, 0, 0},
};

static const char *args = "t:b:g:n:dvh?";

void sig_handler(int signum) {}

speed_t get_baud(unsigned int baud) {
	switch (baud) {
		case 1200:
			return B1200;
		case 1800:
			return B1800;
		case 2400:
			return B2400;
		case 4800:
			return B4800;
		case 9600:
			return B9600;
		case 19200:
			return B19200;
		case 38400:
			return B38400;
		case 57600:
			return B57600;
		case 115200:
			return B115200;
		case 230400:
			return B230400;
		default:
			return 0;
	}
}

int get_major() {
	int major = -1;
	size_t len = 0;
	char *line = NULL;
	char device[20];

	FILE *fp = fopen("/proc/devices", "r");

	if (!fp) return -1;

	while (major == -1 && getline(&line, &len, fp) != -1) {
		if (strstr(line, "gsmtty") != NULL) {
			if (sscanf(line, "%d %s\n", &major, device) != 2)
				major = -1;
		}

		if (line) {
			free(line);
			line = NULL;
		}
	}
	return major;
}

int exit_error(int error, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vdprintf(2, fmt, ap);
	va_end(ap);
	return error;
}

#define LOGV(fmt, ...) if (verbose) dprintf(2, fmt __VA_OPT__(,) __VA_ARGS__);

int main(int argc, char **argv) {
	speed_t baud = B115200;
	char *tty = SERIAL_PORT;
	char *gsm_prefix = GSM_PREFIX;
	unsigned int num_mux = NUM_MUX;

	{
		int c;
		int option_index = 0;
		while (1) {
			c = getopt_long(argc, argv, args, options, &option_index);
			if (c == -1) break;
			switch(c) {
				case 0:
					if (options[option_index].flag)
						break;
					break;
				case 't':
					tty = optarg;
					break;
				case 'b': {
					speed_t newbaud = get_baud(atoi(optarg));
					if (newbaud == 0)
						return exit_error(9, "Baud rate %s unsupported\n", optarg);
					baud = newbaud;
					break;
				}
				case 'g':
					gsm_prefix = optarg;
					break;
				case 'n':
					num_mux = atoi(optarg);
					if (num_mux > 8)
						return exit_error(9, "Maximum number of mux is 8\n");
					break;
				case 'd':
					daemonize = 1;
					break;
				case 'v':
					verbose = 1;
					break;
				case 'h':
				case '?':
				default:
					printf(
						"Usage: %s "
						"[(-t|--tty) parent_tty] "
						"[(-b|--baud) baud] "
						"[(-g|--gsm) gsm_prefix] "
						"[(-n|--num) num_mux] "
						"[--daemon] "
						"[--verbose]\n", argv[0]);
					return 1;
			}
		}
	}
	// {
	// 	struct stat dirstat;
	// 	stat(dirname(gsm_prefix), &dirstat);
	// 	if (
	// 		(getuid() != dirstat.st_uid & (dirstat.st_mode & S_IWUSR)) ||
	// 		(getgid() != dirstat.st_uid & (dirstat.st_mode & S_IWGRP))
	// 	) {
	// 		return exit_error(99, "This program should be run as root\n");
	// 	}
	// }

	int ldisc = N_GSM0710;
	struct gsm_config c;

	int major = get_major();

	if (major == -1)
		return exit_error(
			3,
			"Could not find gsmmux in /proc/devices, "
			"is n_gsm kmod loaded?\n");

	LOGV("gsmmux major %d\n", major);

	/* open the serial port connected to the modem */
	int fd = open(tty, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
		return exit_error(
			2,
			"Could not open parent tty %s\n",
			tty);

	LOGV("%s open\n", tty);

	struct termios termios_p;
	tcgetattr(fd, &termios_p);
	cfmakeraw(&termios_p);
	cfsetspeed(&termios_p, baud);
	tcsetattr(fd, TCSANOW, &termios_p);

	struct stat parent_tty_stat;
	fstat(fd, &parent_tty_stat);

	/* use n_gsm line discipline */
	if (ioctl(fd, TIOCSETD, &ldisc))
		return exit_error(
			1,
			"Could not set n_gsm line discipline\n");

	LOGV("N_GSM0710 ldisc set\n");

	/* get n_gsm configuration */
	if(ioctl(fd, GSMIOC_GETCONF, &c))
		return exit_error(
			1,
			"Could not get GSMIOC config\n");

	/* we are initiator and need encoding 0 (basic) */
	c.initiator = 1;
	c.encapsulation = 0;
	/* our modem defaults to a maximum size of 127 bytes */
	c.mru = 127;
	c.mtu = 127;
	c.i = 1;
	c.adaption = 1;
	/* set the new configuration */
	if(ioctl(fd, GSMIOC_SETCONF, &c))
		return exit_error(
			1,
			"Could not set GSMIOC config\n");

	LOGV("line configured\n");

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	for (int minor = 0; minor < num_mux; minor++) {
		char devname[32];
		snprintf(devname, 32, "%s%d", gsm_prefix, minor);
		dev_t device = makedev(major, minor + 1);
		if (mknod(devname, S_IFCHR | 0666, device)) {
			dprintf(2, "%s creating dev %s\n", strerror(errno), devname);
		} else {
			chown(devname, parent_tty_stat.st_uid, parent_tty_stat.st_gid);
			chmod(devname, parent_tty_stat.st_mode);
		}
	}

	if (daemonize) {
		LOGV("Daemonize\n");
		if (daemon(0, verbose)) {
			dprintf(2, "%s calling daemon\n", strerror(errno));
		}
	}

	LOGV("Pausing\n");

	/* and wait for ever to keep the line discipline enabled */
	pause();

	LOGV("Shutdown\n");

	for (int minor = 0; minor < num_mux; minor++) {
		char devname[32];
		snprintf(devname, 32, "/dev/ttyGSM%d", minor);
		if (unlink(devname))
			dprintf(2, "%s unlinking dev %s\n", strerror(errno), devname);
	}
	ldisc = N_TTY;
	if (ioctl(fd, TIOCSETD, &ldisc))
		return exit_error(
			1,
			"Could not set TTY line discipline\n");

	close(fd);
	return 0;
}
