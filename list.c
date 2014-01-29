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

#include <string.h>
#include <errno.h>
#include <ctype.h>

struct cip_vlist_node {
	struct cip_vlist_node *next;
	unsigned char value[] __attribute__((aligned));
};

static void cip_vlist_free(struct cip_vlist_node *list,
			   void (*free_fn)(void *value))
{
	struct cip_vlist_node *next;

	while (list != NULL) {

		if (free_fn != 0)
			free_fn(list->value);

		next = list->next;
		free(list);
		list = next;
	}
}

void *cip_list_parse(char **remainder, size_t *count, cip_err_ctx *ctx,
		     char *s, const cip_opt_type *type)
{
	struct cip_vlist_node *n, *list, **list_end;
	unsigned char *values;
	size_t i;

	list = NULL;
	list_end = &list;
	i = 0;

	while (1) {

		*list_end = malloc(sizeof **list_end + type->size);
		if (*list_end == NULL) {
			cip_err(ctx, "%m");
			cip_vlist_free(list, type->free_fn);
			return NULL;
		}

		(*list_end)->next = NULL;

		s = type->parse_fn(ctx, (*list_end)->value, s);
		if (s == NULL) {
			cip_vlist_free(list, type->free_fn);
			return NULL;
		}

		list_end = &(*list_end)->next;
		++i;

		while (*s != 0 && isspace(*s))
			++s;

		if (*s != ',')
			break;

		++s;

		while (*s != 0 && isspace(*s))
			++s;
	}

	/* i > 0 (type->parse_fn will error on empty list) */

	values = malloc(i * type->size);
	if (values == NULL) {
		cip_err(ctx, "%m");
		cip_vlist_free(list, type->free_fn);
		return NULL;
	}

	*count = i;
	*remainder = s;

	for (n = list, i = 0; n != NULL; n = n->next, i += type->size)
		memcpy(values + i, n->value, type->size);

	cip_vlist_free(list, 0);

	return values;
}

int cip_list_format(cip_err_ctx *ctx, char *buf, size_t size, void *values,
		    size_t count, const cip_opt_type *type)
{
	unsigned char *v;
	int ret, total;
	size_t i;

	v = values;
	total = 0;
	count *= type->size;
	i = 0;

	while (1) {

		ret = type->format_fn(ctx, buf, size, v + i);
		if (ret < 0)
			return -1;

		total += ret;

		if ((unsigned)ret >= size)
			ret = size - 1;

		buf += ret;
		size -= ret;

		i += type->size;
		if (i == count)
			break;

		ret = snprintf(buf, size, ", ");
		if (ret < 0) {
			cip_err(ctx, "%m");
			return -1;
		}

		total += ret;

		if ((unsigned)ret >= size)
			ret = size - 1;

		buf += ret;
		size -= ret;
	}

	return total;
}
