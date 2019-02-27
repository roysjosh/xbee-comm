/*
 * ehx2srec: decrypt and decode .ehx "encrypted hex" files to S-record
 * Copyright (C) 2013  Joshua Roys
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

#include <err.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include <openssl/evp.h>

ssize_t
read_line(char *out, size_t outmax) {
	ssize_t ret;

	if ( (ret = read(STDIN_FILENO, out, outmax)) < 0) {
		warn("read");
	}
	return ret;
}

/*
 * set stdin echo on or off
 */
int
set_echo(int echo) {
	struct termios localios;

	if (tcgetattr(STDIN_FILENO, &localios)) {
		warn("tcgetattr");
		return -1;
	}
	if (echo) {
		localios.c_lflag |= ECHO;
	}
	else {
		localios.c_lflag &= ~ECHO;
	}
	if (tcsetattr(STDIN_FILENO, TCSANOW, &localios)) {
		warn("tcsetattr");
		return -1;
	}
	return 0;
}

ssize_t
read_pass(char *out, size_t outmax) {
	ssize_t ret;

	if (set_echo(0)) {
		return -1;
	}
	printf("Password: ");
	fflush(stdout);
	ret = read_line(out, outmax);
	printf("\n");
	if (set_echo(1)) {
		return -1;
	}
	return ret;
}

int
kdf_derive(const uint8_t *pass, size_t passlen, uint8_t *keybuffer, uint8_t *ivbuffer) {
	return EVP_BytesToKey(EVP_des_ede3_cbc(), EVP_sha1(),
			NULL, pass, passlen, 1,
			keybuffer, ivbuffer);
}

int
kdf_simple(uint8_t *keybuffer, uint8_t *ivbuffer) {
	char pass[64];
	int i, ret;
	uint16_t utfpass[64];

	/* read pass from stdin */
	if ( (ret = read_pass(pass, sizeof(pass))) <= 0) {
		return -1;
	}
	/* trim any trailing carriage returns or newlines */
	while (pass[ret - 1] == '\r' || pass[ret - 1] == '\n') {
		pass[--ret] = '\0';
	}
	/* convert pass to UTF-16LE */
	for (i = 0; i < ret; i++) {
		utfpass[i] = (uint16_t)pass[i]; /* HACK */
	}
	/* generate key/iv */
	if ( (ret = kdf_derive((uint8_t *)utfpass, ret * 2, keybuffer, ivbuffer)) != 24) {
		warnx("key derivation failed: %i/24 bytes returned", ret);
		return -1;
	}
	return 0;
}

void
untwist(unsigned char *buf, int len) {
	int i;
	static const int sub[16] = { 2, 10, 13, 1, 11, 6, 3, 15, 5, 12, 8, 0, 14, 4, 7, 9 };

	for (i = 0; i < len; i++) {
		buf[i] = ((sub[buf[i] >> 4]) << 4) | (sub[buf[i] & 0x0F]);
	}
}

int
main(int argc, char *argv[]) {
	int declen, infd, outfd, ret;
	ssize_t sret;
	uint8_t keybuffer[24], ivbuffer[8];
	unsigned char buf[4096], decbuf[4096];
	EVP_CIPHER_CTX *ectx;

	if (argc < 3) {
		errx(EXIT_FAILURE, "<xb24_15_4_ABCD.ehx> <out.hex>");
	}
	if ( (infd = open(argv[1], O_RDONLY)) < 0) {
		err(EXIT_FAILURE, "open");
	}
	if ( (outfd = creat(argv[2], S_IRUSR|S_IWUSR)) < 0) {
		err(EXIT_FAILURE, "creat");
	}

	/* generate key/iv from password */
	if ( (ret = kdf_simple(keybuffer, ivbuffer)) < 0) {
		return ret;
	}

	/* read off the initial length */
	if ( (sret = read(infd, buf, 4)) < 0) {
		err(EXIT_FAILURE, "read");
	}

	/* decrypt & decode */
	if ( (ectx = EVP_CIPHER_CTX_new()) == NULL) {
		err(EXIT_FAILURE, "EVP_CIPHER_CTX_new");
	}
	EVP_CIPHER_CTX_init(ectx);
	EVP_DecryptInit_ex(ectx, EVP_des_ede3_cbc(), NULL, keybuffer, ivbuffer);

	while (infd > 0) {
		if ( (sret = read(infd, buf, sizeof(buf))) < 0) {
			err(EXIT_FAILURE, "read");
		}
		if (!sret) { /* EOF */
			close(infd);
			infd = -1;
			if (!EVP_DecryptFinal_ex(ectx, decbuf, &declen)) {
				errx(EXIT_FAILURE, "EVP_DecryptFinal_ex");
			}
		} else {
			if (!EVP_DecryptUpdate(ectx, decbuf, &declen, buf, sret)) {
				errx(EXIT_FAILURE, "EVP_DecryptUpdate");
			}
		}

		/* pass decrypted data through a S-box? */
		untwist(decbuf, declen);

		if ( (sret = write(outfd, decbuf, declen)) < 0) {
			err(EXIT_FAILURE, "write");
		}
	}

	EVP_CIPHER_CTX_free(ectx);

	return EXIT_SUCCESS;
}

// vim: cindent
