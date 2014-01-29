/*
 * Copyright 2014 Ian Pilcher <arequipeno@gmail.com>
 *
 * This program is free software.  You can redistribute it or modify it under
 * the terms of version 2 of the GNU General Public License (GPL), as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY -- without even the implied warranty of MERCHANTIBILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the text of the GPL for more details.
 *
 * Version 2 of the GNU General Public License is available at:
 *
 *   http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include "libcip.h"
#include "libcip_p.h"

#include <stdarg.h>
#include <stdio.h>

static const char *const cip_static_errors[] = {
	[0] = "Failed to allocate memory for error message",
	[1] = "Error formatting error message",
};

cip_err_ctx *cip_err_ctx_init(cip_err_ctx *ctx)
{
	if (ctx == NULL) {
		ctx = malloc(sizeof *ctx);
		if (ctx == NULL)
			return NULL;
	}

	ctx->err_buf = NULL;
	ctx->buf_size = 0;
	ctx->static_err = -1;

	return ctx;
}

void cip_err_ctx_fini(cip_err_ctx *ctx)
{
	free(ctx->err_buf);
}

const char *cip_last_err(const cip_err_ctx *ctx)
{
	if (ctx->static_err >= 0)
		return cip_static_errors[ctx->static_err];
	else
		return ctx->err_buf;
}

static void cip_err_fmt(cip_err_ctx *ctx, const char *format, va_list ap)
{
	va_list ap_copy;
	char *new_buf;
	int len;

	while (1) {

		va_copy(ap_copy, ap);
		len = vsnprintf(ctx->err_buf, ctx->buf_size, format, ap_copy);
		va_end(ap_copy);

		if (len < 0) {
			ctx->static_err = 1;
			return;
		}

		if ((unsigned)len < ctx->buf_size) {
			ctx->static_err = -1;
			return;
		}

		new_buf = realloc(ctx->err_buf, len + 1);
		if (new_buf == NULL) {
			ctx->static_err = 0;
			return;
		}

		ctx->err_buf = new_buf;
		ctx->buf_size = len + 1;
	}
}

void cip_err(cip_err_ctx *ctx, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	cip_err_fmt(ctx, format, ap);
	va_end(ap);
}

void *cip_err_ptr(cip_err_ctx *ctx, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	cip_err_fmt(ctx, format, ap);
	va_end(ap);

	return NULL;
}

int cip_err_int(cip_err_ctx *ctx, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	cip_err_fmt(ctx, format, ap);
	va_end(ap);

	return -1;
}

void cip_err_use(cip_err_ctx *ctx, const char *format, ...)
{
	char *old_buf;
	va_list ap;

	old_buf = ctx->err_buf;
	ctx->err_buf = NULL;
	ctx->buf_size = 0;

	va_start(ap, format);
	cip_err_fmt(ctx, format, ap);
	va_end(ap);

	free(old_buf);
}
