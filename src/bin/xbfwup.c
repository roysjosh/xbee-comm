/*
 * xbfwup: program new firmware to your xbee
 * Copyright (C) 2011  Joshua Roys
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define _BSD_SOURCE

#ifdef __APPLE__
#   include <machine/endian.h>
#else
#   include <endian.h>
#endif

#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "xb_ctx.h"

extern char *optarg;
extern int optind;

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

ssize_t
xb_read(int fd, char *buf, size_t count) {
	ssize_t pos = 0, ret;
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	do {
		ret = read(fd, buf + pos, count - pos);

		if (ret <= 0) {
			return ret;
		}

		pos += ret;

		ret = poll(fds, 1, 100);
	} while (ret > 0 && pos < count);

	return pos;
}

ssize_t
xb_write(int fd, const char *buf, size_t count) {
	ssize_t ret;

	while (count > 0) {
		ret = write(fd, buf, count);

		if (ret <= 0) {
			return -1;
		}

		count -= ret;
		buf += ret;
	}

	return 0;
}

/* http://en.wikipedia.org/wiki/Computation_of_CRC */
uint16_t
xmodem_crc(const char *data) {
	int i, j;
	uint16_t rem = 0;

	/* assume 128 byte blocks */
	for(i = 0; i < 128; i++) {
		rem ^= data[i] << 8;
		for(j = 0; j < 8; j++) {
			if (rem & 0x8000) {
				rem = (rem << 1) ^ 0x1021;
			}
			else {
				rem <<= 1;
			}
		}
	}

//	printf("%0hx\n", rem);

	return rem;
}

int
xb_firmware_update(int xbfd, int fwfd) {
	char *fwbuf;
	char buf[128], header[3], reply[1];
	size_t len, off;
	ssize_t ret;
	struct stat stbuf;
	uint8_t block;
	uint16_t crc;
	unsigned int i;

	/* at the menu: "1. upload ebl" */
	if (xb_write(xbfd, "1", 1)) {
		warnx("failed to enter programming mode");
		return -1;
	}

	/* read reply: "\r\nbegin upload\r\nC" */
	if ( (ret = xb_read(xbfd, buf, sizeof(buf))) <= 0) {
		warnx("failed to read programming go-ahead");
		return -1;
	}
	if (buf[ret - 1] != 'C') {
		warnx("unknown transfer type");
		return -1;
	}

	/* read entire firmware image into memory */
	if (fstat(fwfd, &stbuf) < 0) {
		warn("failed to stat firmware file");
		return -1;
	}
	if (stbuf.st_size == 0) {
		warnx("empty firmware file!");
		return -1;
	}

	/* round up to nearest 128 byte block */
	len = stbuf.st_size;
	len += (len % 128 ? 128 - (len % 128) : 0);

	if ( (fwbuf = malloc(len)) == NULL) {
		warn("failed to allocate memory");
		return -1;
	}

	/* set "empty" bytes to 0xff */
	memset(fwbuf + stbuf.st_size, 0xff, len - stbuf.st_size);

	off = 0;
	do {
		ret = read(fwfd, fwbuf + off, len - off);

		if (ret < 0) {
			return -1;
		}

		off += ret;
	} while (ret);

	printf("Read %i byte firmware file (%i blocks).\n",
			(int)stbuf.st_size, (int)len / 128);

	/* send it */
	for(block = 1, i = 0; i < len / 128; block++, i++) {
		/* display progress */
		printf(".");
		if ((i + 1) % 50 == 0) {
			printf(" %4i\n", (i + 1));
		}
		fflush(stdout);

		header[0] = (uint8_t)'\x01'; /* SOH */
		header[1] = (uint8_t)block;
		header[2] = (uint8_t)(255 - block);

		if (xb_write(xbfd, header, 3)) {
			warn("failed to write XMODEM header, block %i", i);
			return -1;
		}

		if (xb_write(xbfd, fwbuf + (i * 128), 128)) {
			warn("failed to write XMODEM data, block %i", i);
			return -1;
		}

		/* CRC-16 */
		crc = xmodem_crc(fwbuf + (i * 128));
		crc = htobe16(crc);
		if (xb_write(xbfd, (const char *)&crc, 2)) {
			warn("failed to write XMODEM CRC, block %i", i);
			return -1;
		}

		if ((i + 1) == len / 128) {
			printf("\nWaiting for upload confirmation...\n");
			fflush(stdout);
		}

		/* read ACK (0x06); use read for speed! */
		if (read(xbfd, reply, 1) <= 0 || *reply != '\x06') {
			warnx("failed to transfer block %i: %02x", i, *reply);
			return -1;
		}
	}

	/* write EOT (0x04) */
	if (xb_write(xbfd, "\x04", 1)) {
		warn("failed to write XMODEM EOT");
		return -1;
	}

	/* read reply: "\x06\r\nSerial upload complete\r\n" */
	if (xb_read(xbfd, buf, sizeof(buf)) <= 0 || *buf != '\x06') {
		warnx("failed to read programming confirmation");
		return -1;
	}

	return 0;
}

void
serial_setup(struct xb_ctx *xctx, struct termios *serial) {
	/*
	 * 9600 is the default baudrate for the XBee in API/AT mode.
	 * The serial paramters should be 8-N-1.
	 */

	bzero(serial, sizeof(*serial));
	serial->c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	/* blocking reads */
	serial->c_cc[VMIN] = 1;
	serial->c_cc[VTIME] = 0;
	if (xctx->api_mode == XB_AT) {
		/* this makes reading much easier on AT mode */
		serial->c_lflag = ICANON;
		serial->c_iflag = ICRNL;
	}
	if (tcsetattr(xctx->xbfd, TCSANOW, serial)) {
		err(EXIT_FAILURE, "error setting baudrate 9600 & 8N1");
	}
}

void
usage(const char *argv0, int status) {
	fprintf(stderr, "Usage: %s [-A api_mode] [-d /dev/ttyX] firmware.ebl\n", argv0);
	exit(status);
}

int
program_local(int fwfd, struct xb_ctx *xctx) {
	char tmpbuf[512];
	int i;
	ssize_t ret;
	struct buffer *buf;
	struct termios serial;
	uint8_t frame_id;

	if (tcgetattr(xctx->xbfd, &serial)) {
		err(EXIT_FAILURE, "failed to get terminal attributes");
	}

	serial_setup(xctx, &serial);

//	if (!recovery_mode)

	/* enter command mode */
	if (!xctx->api_mode) {
		printf("Entering AT command mode...\n");
		sleep(1);
		write(xctx->xbfd, "+++", 3);
		sleep(1);
		wait_for_ok(xctx->xbfd);
	}

	printf("Entering bootloader...\n");

	/* start the power cycle */
	if (xb_send_at_cmd(xctx, "FR", &frame_id) < 0) {
		err(EXIT_FAILURE, "xb_send_at_cmd");
	}

	buf = xb_wait_for_reply(xctx, frame_id);
	if (!buf) {
		err(EXIT_FAILURE, "xb_wait_for_reply");
	}
	buffer_free(buf);

	/* assert DTR, clear RTS */
	i = TIOCM_DTR | TIOCM_CTS;
	ioctl(xctx->xbfd, TIOCMSET, &i);

	/* send a serial break */
	ioctl(xctx->xbfd, TIOCSBRK);

	/* wait for the power cycle to hit */
	sleep(2);

	/* clear the serial break */
	ioctl(xctx->xbfd, TIOCCBRK);

	/* RTS/CTS have an annoying habit of toggling... */
	i = TIOCM_DTR | TIOCM_CTS;
	ioctl(xctx->xbfd, TIOCMSET, &i);

	/* send a carriage return at 115200bps */
	cfsetspeed(&serial, B115200);
	/* don't wait more than 1/10s for input */
	serial.c_cc[VMIN] = 0;
	serial.c_cc[VTIME] = 1;
	/* clear canonical input mode */
	serial.c_lflag = 0;
	serial.c_iflag = 0;
	if (tcsetattr(xctx->xbfd, TCSANOW, &serial)) {
		err(EXIT_FAILURE, "failed to set 115200bps, VMIN/VTIME");
	}

	for(i = 0; i < 100; i++) {
		if (i % 5 == 0) {
			printf(".");
			fflush(stdout);
		}
		xb_write(xctx->xbfd, "\r", 1);
		if ( (ret = xb_read(xctx->xbfd, tmpbuf, sizeof(tmpbuf))) > 0) {
			break;
		}
	}
	printf("\n");

	/* check for "BL >" prompt */
	if (ret < 6 || strncmp(tmpbuf + ret - 6, "BL > \0", 6)) {
		errx(EXIT_FAILURE, "failed to read bootloader prompt");
	}

	/* restore "wait forever" settings */
	serial.c_cc[VMIN] = 1;
	serial.c_cc[VTIME] = 0;
	if (tcsetattr(xctx->xbfd, TCSANOW, &serial)) {
		err(EXIT_FAILURE, "failed to reset VMIN/VTIME");
	}

	printf("Beginning programming...\n");

	/* update! */
	if (xb_firmware_update(xctx->xbfd, fwfd)) {
		errx(EXIT_FAILURE, "failed to flash firmware!");
	}

	/* verify */

	printf("Programming complete, running uploaded firmware...\n");

	/* run the firmware */
	xb_write(xctx->xbfd, "2", 1);

	/* cleanup */
	cfsetspeed(&serial, B9600);
	if (tcsetattr(xctx->xbfd, TCSANOW, &serial)) {
		err(EXIT_FAILURE, "failed to set 9600bps");
	}

	return EXIT_SUCCESS;
}

int
main(int argc, char *argv[]) {
	const char *fwfile, *ttydev;
	enum xb_api_mode api_mode = XB_AT;
	int fwfd, i;
	struct xb_ctx *xctx;

	ttydev = "/dev/ttyUSB0";

	while ( (i = getopt(argc, argv, "A:d:")) != -1) {
		switch (i) {
		case 'A':
			api_mode = atoi(optarg);

			if (api_mode > 2) {
				usage(argv[0], EXIT_FAILURE);
			}

			break;

		case 'd':
			ttydev = optarg;
			break;

		default:
			usage(argv[0], EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		usage(argv[0], EXIT_FAILURE);
	}

	fwfile = argv[optind];

	if ( (fwfd = open(fwfile, O_RDONLY)) < 0) {
		err(EXIT_FAILURE, "failed to open firmware file: %s", fwfile);
	}

	xctx = xb_open(ttydev, api_mode);
	if (!xctx) {
		err(EXIT_FAILURE, "failed to open serial console");
	}

	program_local(fwfd, xctx);

	close(fwfd);
	//xb_close(xctx);

	return EXIT_SUCCESS;
}
