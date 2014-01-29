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
#include <errno.h>
#include <stdio.h>

static char *cip_float_parse(cip_err_ctx *ctx, void *value, char *s)
{
	char *endptr;

	errno = 0;
	*(float *)value = strtof(s, &endptr);

	if (errno != 0) {
		cip_err(ctx, "Failed to parse '%.10s' as a "
			"floating-point number: %m", s);
		return NULL;
	}

	if (endptr == s) {
		cip_err(ctx,
			"Failed to parse '%.10s' as a floating-point number",
			s);
		return NULL;
	}

	return endptr;
}

static int cip_float_format(cip_err_ctx *ctx, char *buf, size_t size,
			    const void *value)
{
	int ret;

	ret = snprintf(buf, size, "%f", (double)*(float *)value);
	if (ret < 0) {
		cip_err(ctx, "%m");
		return -1;
	}

	return ret;
}

const cip_opt_type cip_opt_type_float = {
	.name		= "floating-point number",
	.parse_fn	= cip_float_parse,
	.format_fn	= cip_float_format,
	.free_fn	= 0,
	.size		= sizeof(float),
};

static char *cip_float_list_parse(cip_err_ctx *ctx, void *value, char *s)
{
	cip_float_list *list;
	char *remainder;

	list = value;

	list->values = cip_list_parse(&remainder, &list->count, ctx, s,
				      &cip_opt_type_float);
	if (list->values == NULL)
		return NULL;
	else
		return remainder;
}

static int cip_float_list_format(cip_err_ctx *ctx, char *buf, size_t size,
				 const void *value)
{
	const cip_float_list *list;

	list = value;
	return cip_list_format(ctx, buf, size, list->values, list->count,
			       &cip_opt_type_float);
}

static void cip_float_list_free(void *value)
{
	cip_float_list *list;

	list = value;
	free(list->values);
}

const cip_opt_type cip_opt_type_float_list = {
	.name		= "list of floating-point numbers",
	.parse_fn	= cip_float_list_parse,
	.format_fn	= cip_float_list_format,
	.free_fn	= cip_float_list_free,
	.size		= sizeof(cip_float_list),
};
