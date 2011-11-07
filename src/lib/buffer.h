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

#ifndef BUFFER_H
#define BUFFER_H

#include <stdint.h>

struct buffer {
	char *data;
	uint64_t size, readpos, writepos;
};

struct buffer *buffer_new(uint64_t);
void buffer_free(struct buffer *);

int buffer_sprintf(struct buffer *, const char *, ...);

int buffer_get_data(struct buffer *, const char *, uint64_t);
int buffer_put_data(struct buffer *, const char *, uint64_t);

int buffer_get_uint8(struct buffer *, uint8_t *);
int buffer_put_uint8(struct buffer *, uint8_t);

int buffer_get_uint16(struct buffer *, uint16_t *);
int buffer_put_uint16(struct buffer *, uint16_t);

int buffer_get_uint32(struct buffer *, uint32_t *);
int buffer_put_uint32(struct buffer *, uint32_t);

int buffer_get_uint64(struct buffer *, uint64_t *);
int buffer_put_uint64(struct buffer *, uint64_t);

#endif
