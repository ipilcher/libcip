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

#include "../libcip.h"

#include <string.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

static char *cip_short_parse(cip_err_ctx *ctx, void *value, char *s)
{
	char *endptr;
	long val;

	errno = 0;
	val = strtol(s, &endptr, 0);

	if (errno != 0) {
		cip_err(ctx, "Failed to parse '%.10s' as a short integer: %m",
			s);
		return NULL;
	}

	if (endptr == s) {
		cip_err(ctx, "Failed to parse '%.10s' as a short integer", s);
		return NULL;
	}

	if (val < SHRT_MIN || val > SHRT_MAX) {
		cip_err(ctx, "Value (%ld) outside short integer range (%d-%d)",
			val, SHRT_MIN, SHRT_MAX);
		return NULL;
	}

	*(short *)value = (short)val;
	return endptr;
}

static int cip_short_format(cip_err_ctx *ctx, char *buf, size_t size,
			    const void *value)
{
	int ret;

	ret = snprintf(buf, size, "%hd", *(short *)value);
	if (ret < 0) {
		cip_err(ctx, "%m");
		return -1;
	}

	return ret;
}

const cip_opt_type cip_opt_type_short = {
	.name		= "short integer",
	.parse_fn	= cip_short_parse,
	.format_fn	= cip_short_format,
	.free_fn	= 0,
	.size		= sizeof(short),
};

static char *cip_short_list_parse(cip_err_ctx *ctx, void *value, char *s)
{
	cip_short_list *list;
	char *remainder;

	list = value;

	list->values = cip_list_parse(&remainder, &list->count, ctx, s,
				      &cip_opt_type_short);
	if (list->values == NULL)
		return NULL;
	else
		return remainder;
}

static int cip_short_list_format(cip_err_ctx *ctx, char *buf, size_t size,
				 const void *value)
{
	const cip_short_list *list;

	list = value;
	return cip_list_format(ctx, buf, size, list->values, list->count,
			       &cip_opt_type_short);
}

static void cip_short_list_free(void *value)
{
	cip_short_list *list;

	list = value;
	free(list->values);
}

const cip_opt_type cip_opt_type_short_list = {
	.name		= "list of short integers",
	.parse_fn	= cip_short_list_parse,
	.format_fn	= cip_short_list_format,
	.free_fn	= cip_short_list_free,
	.size		= sizeof(cip_short_list),
};
