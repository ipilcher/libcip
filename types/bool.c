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

#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

struct cip_bool_term {
	const char *term;
	int len;
	bool value;
};

static const struct cip_bool_term cip_bool_terms[] = {
	{ "true",	sizeof("true") - 1,	true	},
	{ "false",	sizeof("false") - 1, 	false	},
	{ "yes",	sizeof("yes") - 1,	true	},
	{ "no",		sizeof("no") - 1,	false	},
	{ "1",		sizeof("1") - 1,	true	},
	{ "0",		sizeof("0") - 1,	false	},
	{ "on",		sizeof("on") - 1,	true	},
	{ "off",	sizeof("off") - 1,	false	},
	{ NULL }
};

static char *cip_bool_parse(cip_err_ctx *ctx, void *value, char *s)
{
	const struct cip_bool_term *term;
	int len;

	for (len = 0; isalnum(s[len]); ++len);
		/* empty loop */

	for (term = cip_bool_terms; term->term != NULL; ++term) {

		if (len == term->len && strncasecmp(term->term, s, len) == 0) {

			*(bool *)value = term->value;
			return s + len;
		}
	}

	cip_err(ctx, "Failed to parse '%.10s' as a boolean", s);
	return NULL;
}

static int cip_bool_format(cip_err_ctx *ctx, char *buf, size_t size,
			   const void *value)
{
	int ret;

	ret = snprintf(buf, size, "%s", *(bool *)value ? "true" : "false");
	if (ret < 0) {
		cip_err(ctx, "%m");
		return -1;
	}

	return ret;
}

const cip_opt_type cip_opt_type_bool = {
	.name		= "boolean",
	.parse_fn	= cip_bool_parse,
	.format_fn	= cip_bool_format,
	.free_fn	= 0,
	.size		= sizeof(bool),
};

static char *cip_bool_list_parse(cip_err_ctx *ctx, void *value, char *s)
{
	cip_bool_list *list;
	char *remainder;

	list = value;

	list->values = cip_list_parse(&remainder, &list->count, ctx, s,
				      &cip_opt_type_bool);
	if (list->values == NULL)
		return NULL;
	else
		return remainder;
}

static int cip_bool_list_format(cip_err_ctx *ctx, char *buf, size_t size,
				const void *value)
{
	const cip_bool_list *list;

	list = value;
	return cip_list_format(ctx, buf, size, list->values, list->count,
			       &cip_opt_type_bool);
}

static void cip_bool_list_free(void *value)
{
	cip_bool_list *list;

	list = value;
	free(list->values);
}

const cip_opt_type cip_opt_type_bool_list = {
	.name		= "list of booleans",
	.parse_fn	= cip_bool_list_parse,
	.format_fn	= cip_bool_list_format,
	.free_fn	= cip_bool_list_free,
	.size		= sizeof(cip_bool_list),
};
