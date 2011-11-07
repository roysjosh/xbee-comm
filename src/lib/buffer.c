/*
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

struct buffer *
buffer_new(uint64_t starting_size) {
	struct buffer *buf;

	buf = (struct buffer *)malloc(sizeof(struct buffer));
	if (!buf) {
		return NULL;
	}

	buf->data = (char *)malloc(starting_size);
	if (!buf) {
		free(buf);
		return NULL;
	}

	buf->size = starting_size;
	buf->readpos = 0;
	buf->writepos = 0;

	return buf;
}

void
buffer_free(struct buffer *buf) {
	free(buf->data);
	free(buf);
}

int
buffer_sprintf(struct buffer *buf, const char *format, ...) {
	int ret;
	size_t size;
	va_list ap;

	size = buf->size - buf->writepos;

	va_start(ap, format);
	ret = vsnprintf(buf->data + buf->writepos, size, format, ap);
	va_end(ap);

	if (ret >= 0 && (size_t)ret < size) {
		buf->writepos += ret;
		return ret;
	}

	return -1;
}

/*
int buffer_get_data(struct buffer *, const char *, uint64_t);
int buffer_put_data(struct buffer *, const char *, uint64_t);
*/

int
buffer_get_uint8(struct buffer *buf, uint8_t *u8) {
	if (buf->readpos + sizeof(uint8_t) > buf->size) {
		return -1;
	}

	memcpy(u8, buf->data + buf->readpos, sizeof(uint8_t));
	buf->readpos += sizeof(uint8_t);

	return 0;
}

int
buffer_put_uint8(struct buffer *buf, uint8_t u8) {
	if (buf->writepos + sizeof(uint8_t) > buf->size) {
		return -1;
	}

	memcpy(buf->data + buf->writepos, &u8, sizeof(uint8_t));
	buf->writepos += sizeof(uint8_t);

	return 0;
}

int
buffer_get_uint16(struct buffer *buf, uint16_t *u16) {
	if (buf->readpos + sizeof(uint16_t) > buf->size) {
		return -1;
	}

	memcpy(u16, buf->data + buf->readpos, sizeof(uint16_t));
	buf->readpos += sizeof(uint16_t);

	return 0;
}

int
buffer_put_uint16(struct buffer *buf, uint16_t u16) {
	if (buf->writepos + sizeof(uint16_t) > buf->size) {
		return -1;
	}

	memcpy(buf->data + buf->writepos, &u16, sizeof(uint16_t));
	buf->writepos += sizeof(uint16_t);

	return 0;
}

int
buffer_get_uint32(struct buffer *buf, uint32_t *u32) {
	if (buf->readpos + sizeof(uint32_t) > buf->size) {
		return -1;
	}

	memcpy(u32, buf->data + buf->readpos, sizeof(uint32_t));
	buf->readpos += sizeof(uint32_t);

	return 0;
}

int
buffer_put_uint32(struct buffer *buf, uint32_t u32) {
	if (buf->writepos + sizeof(uint32_t) > buf->size) {
		return -1;
	}

	memcpy(buf->data + buf->writepos, &u32, sizeof(uint32_t));
	buf->writepos += sizeof(uint32_t);

	return 0;
}

int
buffer_get_uint64(struct buffer *buf, uint64_t *u64) {
	if (buf->readpos + sizeof(uint64_t) > buf->size) {
		return -1;
	}

	memcpy(u64, buf->data + buf->readpos, sizeof(uint64_t));
	buf->readpos += sizeof(uint64_t);

	return 0;
}

int
buffer_put_uint64(struct buffer *buf, uint64_t u64) {
	if (buf->writepos + sizeof(uint64_t) > buf->size) {
		return -1;
	}

	memcpy(buf->data + buf->writepos, &u64, sizeof(uint64_t));
	buf->writepos += sizeof(uint64_t);

	return 0;
}
