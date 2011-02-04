#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

void
wait_for_ok(int fd) {
	char buf[1];

	do {
		if (read(fd, buf, 1) > 0 && *buf == 'O')
			break;
		usleep(100000);
	} while (1);
	read(fd, buf, 1);
	read(fd, buf, 1);
}

int
main() {
	char buf[1024];
	int fd, i, ret;
	ssize_t pos, sret;
	struct pollfd fds[1];
	struct termios serial;

	if ( (fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY)) < 0) {
		err(EXIT_FAILURE, "failed to open serial console");
	}

	if (tcgetattr(fd, &serial)) {
		err(EXIT_FAILURE, "failed to get terminal attributes");
	}

	/* enter command mode */
	sleep(1);
	write(fd, "+++", 3);
	sleep(1);
	wait_for_ok(fd);

	/* start the power cycle */
	write(fd, "ATFR\r", 5);
	wait_for_ok(fd);

	/* assert DTR, clear RTS */
	i = TIOCM_DTR | TIOCM_CTS;
	ioctl(fd, TIOCMSET, &i);

	/* send a serial break */
	ioctl(fd, TIOCSBRK);

	/* wait for the power cycle to hit */
	sleep(2);

	/* clear the serial break */
	ioctl(fd, TIOCCBRK);

	/* RTS/CTS have an annoying habit of toggling... */
	i = TIOCM_DTR | TIOCM_CTS;
	ioctl(fd, TIOCMSET, &i);

	/* send a carriage return at 115200bps */
	cfsetspeed(&serial, B115200);
	if (tcsetattr(fd, TCSANOW, &serial)) {
		err(EXIT_FAILURE, "failed to set 115200bps");
	}

	for(i = 0; i < 10; i++) {
		write(fd, "\r", 1);
		sret = read(fd, buf, sizeof(buf));
		if (sret > 0)
			break;
		usleep(100000);
	}
	if (sret == 0) {
		errx(EXIT_FAILURE, "timeout waiting for bootloader prompt");
	}

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	pos = 0;
	do {
		sret = read(fd, buf + pos, sizeof(buf) - pos);
		if (sret > 0)
			pos += sret;
		ret = poll(fds, 1, 100);
	} while (ret > 0 && pos < sizeof(buf));

	/* run the firmware */
	write(fd, "2", 1);

	/* cleanup */
	cfsetspeed(&serial, B9600);
	if (tcsetattr(fd, TCSANOW, &serial)) {
		err(EXIT_FAILURE, "failed to set 9600bps");
	}

	close(fd);

	return EXIT_SUCCESS;
}
