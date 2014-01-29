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
 * Type-safe AVL tree putters (const/non-const getters are in header files)
 */

static inline int cip_ini_value_put(cip_ini_sect *sect, cip_ini_value *value)
{
	struct cip_avl_node *tree;
	int ret;

	tree = (struct cip_avl_node *)(sect->values);
	ret = cip_avl_add(&tree, (struct cip_avl_node *)value);
	sect->values = (cip_ini_value *)tree;
	return ret;
}

static inline int cip_ini_inst_put(cip_ini_sect *sect, cip_ini_sect *inst)
{
	struct cip_avl_node *tree;
	int ret;

	tree = (struct cip_avl_node *)(sect->instances);
	ret = cip_avl_add(&tree, (struct cip_avl_node *)inst);
	sect->instances = (cip_ini_sect *)tree;
	return ret;
}

static inline int cip_ini_sect_put(cip_ini_file *file, cip_ini_sect *sect)
{
	struct cip_avl_node *tree;
	int ret;

	tree = (struct cip_avl_node *)(file->sections);
	ret = cip_avl_add(&tree, (struct cip_avl_node *)sect);
	file->sections = (cip_ini_sect *)tree;
	return ret;
}

/*
 * Internal API
 */

cip_ini_file *cip_ini_file_new(cip_err_ctx *ctx, const cip_file_schema *schema)
{
	cip_ini_file *new;

	new = malloc(sizeof *new);
	if (new == NULL)
		return cip_err_ptr(ctx, strerror(ENOMEM));

	new->schema = schema;
	new->sections = NULL;

	return new;
}

cip_ini_sect *cip_ini_sect_new(cip_err_ctx *ctx, cip_ini_file *file,
			       const cip_sect_schema *schema)
{
	cip_ini_sect *new;

	new = malloc(sizeof *new);
	if (new == NULL)
		return cip_err_ptr(ctx, strerror(ENOMEM));

	new->node.name = schema->node.name;
	new->schema = schema;

	if (schema->flags & CIP_SECT_MULTIPLE)
		new->instances = NULL;
	else
		new->values = NULL;

	if (cip_ini_sect_put(file, new) == -1) {
		free(new);
		cip_err(ctx, "Duplicate section [%s]", schema->node.name);
		return NULL;
	}

	return new;
}

cip_ini_sect *cip_ini_inst_new(cip_err_ctx *ctx, cip_ini_sect *sect,
			       const cip_sect_schema *schema, char *id)
{
	cip_ini_sect *new;

	new = malloc(sizeof *new);
	if (new == NULL)
		return cip_err_ptr(ctx, strerror(ENOMEM));

	new->node.name = id;
	new->schema = schema;
	new->values = NULL;

	if (cip_ini_inst_put(sect, new) == -1) {
		free(new);
		return cip_err_ptr(ctx, "Duplicate section [%s:%s]",
				   schema->node.name, id);
	}

	return new;
}

int cip_ini_value_new(cip_err_ctx *ctx, cip_ini_sect *sect,
		      const cip_opt_schema *schema, const void *value)
{
	cip_ini_value *new;

	new = malloc(sizeof *new + schema->type->size);
	if (new == NULL)
		return cip_err_int(ctx, strerror(ENOMEM));

	new->node.name = schema->node.name;
	new->schema = schema;
	new->post_parse_done = 0;
	memcpy(new->value, value, schema->type->size);

	if (cip_ini_value_put(sect, new) == -1) {

		free(new);
		if (sect->schema->flags & CIP_SECT_MULTIPLE) {
			cip_err(ctx, "Duplicate value [%s:%s]:%s",
				sect->schema->node.name, sect->node.name,
				schema->node.name);
		}
		else {
			cip_err(ctx, "Duplicate value [%s]:%s",
				sect->node.name, schema->node.name);
		}

		return -1;
	}

	return 0;
}

static void cip_ini_value_free(struct cip_avl_node *node)
{
	const void *default_value;
	cip_ini_value *value;
	size_t size;

	value = (cip_ini_value *)node;

	if (value->schema->type->free_fn != 0) {

		default_value = value->schema->default_value;
		size = value->schema->type->size;

		if (memcmp(value->value, default_value, size) != 0)
			value->schema->type->free_fn(value->value);
	}
}

static void cip_ini_inst_free(struct cip_avl_node *node)
{
	cip_ini_sect *inst;

	inst = (cip_ini_sect *)node;
	free(inst->node.name);

	if (inst->values != NULL) {
		cip_avl_free((struct cip_avl_node *)inst->values,
			     cip_ini_value_free);
	}
}

static void cip_ini_sect_free(struct cip_avl_node *node)
{
	cip_ini_sect *sect;

	sect = (cip_ini_sect *)node;

	if (sect->schema->flags & CIP_SECT_MULTIPLE) {
		/* Must have parsed an instance to create the section */
		cip_avl_free((struct cip_avl_node *)sect->instances,
			     cip_ini_inst_free);
	}
	else if (sect->values != NULL) {
		cip_avl_free((struct cip_avl_node *)sect->values,
			     cip_ini_value_free);
	}
}

/*
 * Public API
 */

void cip_ini_file_free(cip_ini_file *file)
{
	if (file->sections != NULL) {
		cip_avl_free((struct cip_avl_node *)file->sections,
			     cip_ini_sect_free);
	}

	free(file);
}
