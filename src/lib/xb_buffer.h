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

#ifndef XB_BUFFER_H
#define XB_BUFFER_H

#include <stdint.h>

#include "buffer.h"

struct xb_buffer;

struct xb_buffer *xb_buffer_new();
void xb_buffer_free(struct xb_buffer *);

uint8_t xb_buffer_get_frame_id(struct xb_buffer *);
void xb_buffer_set_frame_id(struct xb_buffer *, uint8_t);

int xb_buffer_get_data(struct xb_buffer *, const char *, uint16_t);
int xb_buffer_put_data(struct xb_buffer *, const char *, uint16_t);

int xb_buffer_get_uint8(struct xb_buffer *, uint8_t *);
int xb_buffer_put_uint8(struct xb_buffer *, uint8_t);

int xb_buffer_get_uint16(struct xb_buffer *, uint16_t *);
int xb_buffer_put_uint16(struct xb_buffer *, uint16_t);

int xb_buffer_get_uint32(struct xb_buffer *, uint32_t *);
int xb_buffer_put_uint32(struct xb_buffer *, uint32_t);

int xb_buffer_get_uint64(struct xb_buffer *, uint64_t *);
int xb_buffer_put_uint64(struct xb_buffer *, uint64_t);

int xb_buffer_put_at_cmd(struct xb_buffer *, char[2]);

struct buffer *xb_buffer_as_api(struct xb_buffer *);
struct buffer *xb_buffer_as_at(struct xb_buffer *);

#endif
