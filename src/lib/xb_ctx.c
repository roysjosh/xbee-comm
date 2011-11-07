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

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "xb_buffer.h"
#include "xb_ctx.h"

struct xb_ctx *
xb_open(const char *device, enum xb_api_mode api_mode) {
	int xbfd;
	struct xb_ctx *xctx;

	if ( (xbfd = open(device, O_RDWR | O_NOCTTY)) < 0) {
		return NULL;
	}

	xctx = (struct xb_ctx *)malloc(sizeof(struct xb_ctx));
	if (!xctx) {
		close(xbfd);
		return NULL;
	}

	xctx->api_mode = api_mode;
//	xctx->device = strdup(device);
	xctx->xbfd = xbfd;
	xctx->frame_id = 1;

	return xctx;
}

int
xb_write_fully(int fd, const char *buf, size_t count) {
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

int
xb_send(struct xb_ctx *xctx, struct xb_buffer *xbuf) {
	int ret;
	struct buffer *packet;

	if (xctx->api_mode == XB_API || xctx->api_mode == XB_API_ESC) {
		packet = xb_buffer_as_api(xbuf);
	}
	else {
		packet = xb_buffer_as_at(xbuf);
	}

	ret = xb_write_fully(xctx->xbfd, packet->data, packet->writepos);
	buffer_free(packet);

	return ret;
}

struct buffer *
xb_wait_for_reply(struct xb_ctx *xctx, uint8_t frame_id) {
	ssize_t ret;
	struct buffer *buf;

	buf = buffer_new(512);
	if (!buf) {
		return NULL;
	}

	/* XXX actually look for frame_id (if API) or OK\r (if AT) here */
	ret = read(xctx->xbfd, buf->data, buf->size);
	if (ret < 0) {
		free(buf);
		return NULL;
	}

	buf->writepos = (uint64_t)ret;

	return buf;
}

struct xb_buffer *
xb_create_at_cmd(struct xb_ctx *xctx, char at_cmd[2], int flags) {
	struct xb_buffer *xbuf;
	uint8_t frame_id;

	xbuf = xb_buffer_new();
	if (!xbuf) {
		return NULL;
	}

	if (xctx->api_mode == XB_API || xctx->api_mode == XB_API_ESC) {
		if (xb_buffer_put_uint8(xbuf, XB_FRAME_TYPE_AT_CMD) < 0) {
			goto error;
		}

		if (flags & API_REQUEST_ACK) {
			frame_id = xctx->frame_id++;
			if (xctx->frame_id == 0) {
				/* when it wraps around, make sure it doesn't stay on 0 */
				xctx->frame_id++;
			}

			xb_buffer_set_frame_id(xbuf, frame_id);
			if (xb_buffer_put_uint8(xbuf, frame_id) < 0) {
				goto error;
			}
		}
	}

	if (xb_buffer_put_at_cmd(xbuf, at_cmd) < 0) {
		goto error;
	}

	return xbuf;

error:
	xb_buffer_free(xbuf);
	return NULL;
}

int
xb_send_at_cmd(struct xb_ctx *xctx, char at_cmd[2], uint8_t *frame_id) {
	struct xb_buffer *xbuf;

	xbuf = xb_create_at_cmd(xctx, at_cmd, API_REQUEST_ACK);
	if (!xbuf) {
		return -1;
	}
	if (xb_send(xctx, xbuf) < 0) {
		xb_buffer_free(xbuf);
		return -1;
	}
	*frame_id = xb_buffer_get_frame_id(xbuf);
	xb_buffer_free(xbuf);

	return 0;
}
