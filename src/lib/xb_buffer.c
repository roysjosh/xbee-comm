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

#include <stdlib.h>
#include <string.h>

#include "xb_buffer.h"

enum xb_buffer_value_type {
	XB_BUFFER_TYPE_DATA,
	XB_BUFFER_TYPE_U8,
	XB_BUFFER_TYPE_U16,
	XB_BUFFER_TYPE_U32,
	XB_BUFFER_TYPE_U64,
	XB_BUFFER_TYPE_AT_COMMAND,
	XB_BUFFER_TYPE_NULL
};

struct xb_buffer_value {
	struct xb_buffer_value *next;

	enum xb_buffer_value_type type;
	uint16_t len;
	union {
		char *data;
		uint8_t u8;
		uint16_t u16;
		uint32_t u32;
		uint64_t u64;
		char at_cmd[2];
	} value;
};

struct xb_buffer {
	struct xb_buffer_value *head, *tail;

	uint8_t frame_id;
};

struct xb_buffer *
xb_buffer_new() {
	struct xb_buffer *buf;

	buf = (struct xb_buffer *)malloc(sizeof(struct xb_buffer));
	if (!buf) {
		return NULL;
	}

	buf->head = buf->tail = NULL;
	buf->frame_id = 0;

	return buf;
}

void
xb_buffer_free(struct xb_buffer *buf) {
	struct xb_buffer_value *valptr, *nextval;

	for(valptr = buf->head; valptr; valptr = nextval) {
		//if (valptr->type == XB_BUFFER_TYPE_DATA)
		nextval = valptr->next;
		free(valptr);
	}

	free(buf);
}

uint8_t
xb_buffer_get_frame_id(struct xb_buffer *xbuf) {
	return xbuf->frame_id;
}

void
xb_buffer_set_frame_id(struct xb_buffer *xbuf, uint8_t frame_id) {
	xbuf->frame_id = frame_id;
}

struct xb_buffer_value *
xb_buffer_new_value(struct xb_buffer *buf, enum xb_buffer_value_type type) {
	struct xb_buffer_value *val;

	val = (struct xb_buffer_value *)malloc(sizeof(struct xb_buffer_value));
	if (!val) {
		return NULL;
	}

	val->next = NULL;
	val->type = type;

	if (!buf->head) {
		buf->head = buf->tail = val;
	}
	else {
		buf->tail->next = val;
		buf->tail = val;
	}

	return val;
}

/*
int xb_buffer_get_data(struct xb_buffer *, const char *, uint16_t);
int xb_buffer_put_data(struct xb_buffer *, const char *, uint16_t);
*/

int
xb_buffer_get_uint8(struct xb_buffer *buf, uint8_t *u8) {
	return 0;
}

int
xb_buffer_put_uint8(struct xb_buffer *buf, uint8_t u8) {
	struct xb_buffer_value *val;

	val = xb_buffer_new_value(buf, XB_BUFFER_TYPE_U8);
	if (!val) {
		return -1;
	}

	val->len = sizeof(uint8_t);
	val->value.u8 = u8;

	return sizeof(uint8_t);
}

int
xb_buffer_get_uint16(struct xb_buffer *buf, uint16_t *u16) {
	return 0;
}

int
xb_buffer_put_uint16(struct xb_buffer *buf, uint16_t u16) {
	struct xb_buffer_value *val;

	val = xb_buffer_new_value(buf, XB_BUFFER_TYPE_U16);
	if (!val) {
		return -1;
	}

	val->len = sizeof(uint16_t);
	val->value.u16 = u16;

	return sizeof(uint16_t);
}

int
xb_buffer_get_uint32(struct xb_buffer *buf, uint32_t *u32) {
	return 0;
}

int
xb_buffer_put_uint32(struct xb_buffer *buf, uint32_t u32) {
	struct xb_buffer_value *val;

	val = xb_buffer_new_value(buf, XB_BUFFER_TYPE_U32);
	if (!val) {
		return -1;
	}

	val->len = sizeof(uint32_t);
	val->value.u32 = u32;

	return sizeof(uint32_t);
}

int
xb_buffer_get_uint64(struct xb_buffer *buf, uint64_t *u64) {
	return 0;
}

int
xb_buffer_put_uint64(struct xb_buffer *buf, uint64_t u64) {
	struct xb_buffer_value *val;

	val = xb_buffer_new_value(buf, XB_BUFFER_TYPE_U64);
	if (!val) {
		return -1;
	}

	val->len = sizeof(uint64_t);
	val->value.u64 = u64;

	return sizeof(uint64_t);
}

int
xb_buffer_put_at_cmd(struct xb_buffer *buf, char at_cmd[2]) {
	struct xb_buffer_value *val;

	val = xb_buffer_new_value(buf, XB_BUFFER_TYPE_AT_COMMAND);
	if (!val) {
		return -1;
	}

	val->len = 2;
	val->value.at_cmd[0] = at_cmd[0];
	val->value.at_cmd[1] = at_cmd[1];

	return 2;
}

struct buffer *
xb_buffer_as_api(struct xb_buffer *xbuf) {
	struct buffer *buf;
	struct xb_buffer_value *valptr;
	uint8_t csum;
	uint16_t len;
	uint64_t i;

	buf = buffer_new(512);
	if (!buf) {
		return NULL;
	}

	buffer_put_uint8(buf, 0x7e);
	buf->writepos = 3;

	for(valptr = xbuf->head; valptr; valptr = valptr->next) {
		switch(valptr->type) {
		case XB_BUFFER_TYPE_DATA:
			//buffer_put_data(buf, valptr->value.data, valptr->len);
			break;
		case XB_BUFFER_TYPE_U8:
			buffer_put_uint8(buf, valptr->value.u8);
			break;
		case XB_BUFFER_TYPE_U16:
			buffer_put_uint16(buf, valptr->value.u16);
			break;
		case XB_BUFFER_TYPE_U32:
			buffer_put_uint32(buf, valptr->value.u32);
			break;
		case XB_BUFFER_TYPE_U64:
			buffer_put_uint64(buf, valptr->value.u64);
			break;
		case XB_BUFFER_TYPE_AT_COMMAND:
			buffer_sprintf(buf, "%c%c", valptr->value.at_cmd[0],
					valptr->value.at_cmd[1]);
			break;
		case XB_BUFFER_TYPE_NULL:
			break;
		}
	}

	/* XXX check writepos vs size here */

	len = htobe16((uint16_t)buf->writepos - 3);
	memcpy(buf->data + 1, &len, 2);

	for(csum = 0, i = 3; i < buf->writepos; i++) {
		csum += buf->data[i];
	}
	buf->data[buf->writepos++] = (char)(0xff - csum);

	return buf;
}

struct buffer *
xb_buffer_as_at(struct xb_buffer *xbuf) {
	struct buffer *buf;
	struct xb_buffer_value *valptr;

	buf = buffer_new(512);
	if (!buf) {
		return NULL;
	}

	for(valptr = xbuf->head; valptr; valptr = valptr->next) {
		switch(valptr->type) {
		case XB_BUFFER_TYPE_DATA:
			//buffer_put_data(buf, valptr->value.data, valptr->len);
			break;
		case XB_BUFFER_TYPE_U8:
			buffer_sprintf(buf, "%02hhX", valptr->value.u8);
			break;
		case XB_BUFFER_TYPE_U16:
			buffer_sprintf(buf, "%04hX", valptr->value.u16);
			break;
		case XB_BUFFER_TYPE_U32:
			buffer_sprintf(buf, "%08X", valptr->value.u32);
			break;
		case XB_BUFFER_TYPE_U64:
			buffer_sprintf(buf, "%016lX", valptr->value.u64);
			break;
		case XB_BUFFER_TYPE_AT_COMMAND:
			buffer_sprintf(buf, "AT%c%c", valptr->value.at_cmd[0],
					valptr->value.at_cmd[1]);
			break;
		case XB_BUFFER_TYPE_NULL:
			break;
		}
	}


	/* XXX check writepos vs size here */

	buf->data[buf->writepos++] = '\r';

	return buf;
}
