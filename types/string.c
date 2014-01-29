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

#define _GNU_SOURCE	/* for rawmemchr */

#include "../libcip.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

static char *cip_str_parse(cip_err_ctx *ctx, void *value, char *s,
			   const char *delims)
{
	char *end, *remainder;
	char **val;
	size_t len;

	val = value;

	if (*s == '"' || *s == '\'') {

		end = strchr(s + 1, *s);
		if (end == NULL) {
			cip_err(ctx, "Closing quotation mark (%c) not found",
				*s);
			return NULL;
		}

		++s;
		len = end - s;
		remainder = end + 1;
	}
	else {
		remainder = strpbrk(s, delims);
		if (remainder == NULL)
			remainder = rawmemchr(s, 0);

		end = remainder;

		do {
			if (end == s) {
				cip_err(ctx, "Unquoted empty string");
				return NULL;
			}

			--end;
		}
		while (isspace(*end));

		len = end - s + 1;
	}

	*val = malloc(len + 1);
	if (*val == NULL) {
		cip_err(ctx, "%m");
		return NULL;
	}

	memcpy(*val, s, len);
	(*val)[len] = 0;

	return remainder;
}

static char *cip_string_parse(cip_err_ctx *ctx, void *value, char *s)
{
	return cip_str_parse(ctx, value, s, ";#");
}

static int cip_string_format(cip_err_ctx *ctx, char *buf, size_t size,
			     const void *value)
{
	int ret;

	ret = snprintf(buf, size, "%s", *(char **)value);
	if (ret < 0) {
		cip_err(ctx, "%m");
		return -1;
	}

	return ret;
}

static void cip_string_free(void *value)
{
	free(*(char **)value);
}

const cip_opt_type cip_opt_type_string = {
	.name		= "string",
	.parse_fn	= cip_string_parse,
	.format_fn	= cip_string_format,
	.free_fn	= cip_string_free,
	.size		= sizeof(char *),
};

static char *cip_str_mem_parse(cip_err_ctx *ctx, void *value, char *s)
{
	return cip_str_parse(ctx, value, s, ",;#");
}

static const cip_opt_type cip_opt_type_str_mem = {
	.name		= "string list member",
	.parse_fn	= cip_str_mem_parse,
	.format_fn	= cip_string_format,
	.free_fn	= cip_string_free,
	.size		= sizeof(char *),
};

static char *cip_str_list_parse(cip_err_ctx *ctx, void *value, char *s)
{
	cip_str_list *list;
	char *remainder;

	list = value;

	list->values = cip_list_parse(&remainder, &list->count, ctx, s,
				      &cip_opt_type_str_mem);
	if (list->values == NULL)
		return NULL;
	else
		return remainder;
}

static int cip_str_list_format(cip_err_ctx *ctx, char *buf, size_t size,
			       const void *value)
{
	const cip_str_list *list;

	list = value;
	return cip_list_format(ctx, buf, size, list->values, list->count,
			       &cip_opt_type_str_mem);
}

static void cip_str_list_free(void *value)
{
	cip_str_list *list;
	size_t i;

	list = value;

	for (i = 0; i < list->count; ++i)
		free(list->values[i]);

	free(list->values);
}

const cip_opt_type cip_opt_type_str_list = {
	.name		= "list of strings",
	.parse_fn	= cip_str_list_parse,
	.format_fn	= cip_str_list_format,
	.free_fn	= cip_str_list_free,
	.size		= sizeof(cip_str_list),
};
