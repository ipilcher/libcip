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

#include <string.h>
#include <errno.h>

/*
 * Type-safe AVL tree putters (getters are in libcip_p.h)
 */

static inline int cip_opt_schema_put(cip_sect_schema *sect, cip_opt_schema *opt)
{
	struct cip_avl_node *tree;
	int ret;

	tree = (struct cip_avl_node *)(sect->options);
	ret = cip_avl_add(&tree, (struct cip_avl_node *)opt);
	sect->options = (cip_opt_schema *)tree;
	return ret;
}

static inline int cip_sect_schema_put(cip_file_schema *file,
				      cip_sect_schema *sect)
{
	struct cip_avl_node *tree;
	int ret;

	tree = (struct cip_avl_node *)(file->sections);
	ret = cip_avl_add(&tree, (struct cip_avl_node *)sect);
	file->sections = (cip_sect_schema *)tree;
	return ret;
}

/*
 * Public schema API
 */

cip_file_schema *cip_file_schema_new1(cip_err_ctx *ctx)
{
	cip_file_schema *new;

	new = malloc(sizeof *new);
	if (new == NULL)
		return cip_err_ptr(ctx, "%s", strerror(ENOMEM));

	new->sections = NULL;
	return new;
}

cip_file_schema *cip_file_schema_new2(cip_err_ctx *ctx,
				      const cip_sect_info *sections)
{
	cip_file_schema *new;

	new = cip_file_schema_new1(ctx);
	if (new == NULL)
		return NULL;

	if (cip_sect_schema_new3(ctx, new, sections) == -1) {
		cip_file_schema_free(new);
		return NULL;
	}

	return new;
}

cip_sect_schema *cip_sect_schema_new1(cip_err_ctx *ctx,
				      cip_file_schema *file_schema, char *name,
				      unsigned char flags)
{
	cip_sect_schema *new;

	new = malloc(sizeof *new);
	if (new == NULL)
		return cip_err_ptr(ctx, "%s", strerror(ENOMEM));

	new->node.name = name;
	new->flags = flags;
	new->options = NULL;

	if (cip_sect_schema_put(file_schema, new) == -1) {
		cip_err(ctx, "Schema section '%s' already exists", name);
		free(new);
		return NULL;
	}

	return new;
}

cip_sect_schema *cip_sect_schema_new2(cip_err_ctx *ctx,
				      cip_file_schema *file_schema,
				      const cip_sect_info *section)
{
	cip_sect_schema *sect_schema;

	sect_schema = cip_sect_schema_new1(ctx, file_schema, section->name,
					   section->flags);
	if (sect_schema == NULL)
		return NULL;

	if (cip_opt_schema_new3(ctx, sect_schema, section->options) == -1)
		return NULL;

	return sect_schema;
}

int cip_sect_schema_new3(cip_err_ctx *ctx, cip_file_schema *file_schema,
			 const cip_sect_info *sections)
{
	while (sections->name != NULL) {

		if (cip_sect_schema_new2(ctx, file_schema, sections) == NULL)
			return -1;
		++sections;
	}

	return 0;
}

int cip_opt_schema_new1(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			char *name, const cip_opt_type *type,
		        int (*post_parse_fn)(cip_err_ctx *ctx,
					     const cip_ini_value *value,
					     const cip_ini_sect *sect,
					     const cip_ini_file *file,
			                     void *post_parse_data),
		        void *post_parse_data, unsigned char flags,
		        const void *default_value)
{
	cip_opt_schema *new;
	int has_default;

	has_default = flags & CIP_OPT_DEFAULT;

	new = malloc(sizeof *new + (has_default ? type->size : 0));
	if (new == NULL)
		return cip_err_int(ctx, "%m");

	new->node.name = name;
	new->type = type;
	new->post_parse_fn = post_parse_fn;
	new->post_parse_data = post_parse_data;
	new->flags = flags;

	if (has_default)
		memcpy(new->default_value, default_value, type->size);

	if (cip_opt_schema_put(sect_schema, new) == -1) {
		cip_err(ctx, "Schema option '[%s]:%s' already exists",
			sect_schema->node.name, new->node.name);
		free(new);
		return -1;
	}

	return 0;
}

int cip_opt_schema_new2(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			const cip_opt_info *option)
{
	return cip_opt_schema_new1(ctx, sect_schema, option->name, option->type,
				   option->post_parse_fn,
				   option->post_parse_data, option->flags,
				   option->default_value);
}

int cip_opt_schema_new3(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			const cip_opt_info *options)
{
	while (options->name != NULL) {

		if (cip_opt_schema_new2(ctx, sect_schema, options) == -1)
			return -1;
		++options;
	}

	return 0;
}

static void cip_sect_schema_free(struct cip_avl_node *node)
{
	cip_sect_schema *sect_schema;

	sect_schema = (cip_sect_schema *)node;

	if (sect_schema->options != NULL)
		cip_avl_free((struct cip_avl_node *)sect_schema->options, 0);
}

void cip_file_schema_free(cip_file_schema *file_schema)
{
	if (file_schema->sections != NULL) {
		cip_avl_free((struct cip_avl_node *)file_schema->sections,
			     cip_sect_schema_free);
	}

	free(file_schema);
}
