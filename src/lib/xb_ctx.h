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

#ifndef XB_CTX_H
#define XB_CTX_H

#include "xb_buffer.h"

#define XB_FRAME_TYPE_AT_CMD			0x08
#define XB_FRAME_TYPE_EXPLICIT_TX		0x11
#define XB_FRAME_TYPE_AT_CMD_RESPONSE		0x88

/* xb_create_at_cmd flags */
#define API_REQUEST_ACK				(1 << 0)

enum xb_api_mode {
	XB_AT = 0,
	XB_API = 1,
	XB_API_ESC = 2,
};

struct xb_ctx {
	enum xb_api_mode api_mode;
	//char *device;
	int xbfd;
	// baud/stop/parity
	// debug
	uint8_t frame_id;
};

struct xb_ctx *xb_open(const char *, enum xb_api_mode);

int xb_send(struct xb_ctx *, struct xb_buffer *);
struct buffer *xb_wait_for_reply(struct xb_ctx *, uint8_t);

struct xb_buffer *xb_create_at_cmd(struct xb_ctx *, char[2], int);
int xb_send_at_cmd(struct xb_ctx *, char[2], uint8_t *);

#endif
