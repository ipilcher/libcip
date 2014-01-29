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

/*
 * Error reporting - err.c
 */

__attribute__((format(printf, 2, 3)))
void *cip_err_ptr(cip_err_ctx *ctx, const char *format, ...);

__attribute__((format(printf, 2, 3)))
int cip_err_int(cip_err_ctx *ctx, const char *format, ...);

__attribute__((format(printf, 2, 3)))
void cip_err_use(cip_err_ctx *ctx, const char *format, ...);

/*
 * AVL tree stuff - avl.c
 */

void cip_avl_free(struct cip_avl_node *tree,
		  void (*free_fn)(struct cip_avl_node *node));

int cip_avl_add(struct cip_avl_node **tree, struct cip_avl_node *new);

int cip_avl_foreach(struct cip_avl_node *tree,
			   int (*callback_fn)(struct cip_avl_node *node,
					      void *context),
			   void *context);

/*
 * Schema stuff - schema.c
 */

struct cip_opt_schema {
	struct cip_avl_node node;
	const struct cip_opt_type *type;
	int (*post_parse_fn)(cip_err_ctx *ctx, const cip_ini_value *value,
			     const cip_ini_sect *sect, const cip_ini_file *file,
			     void *post_parse_data);
	void *post_parse_data;
	unsigned char flags;
	unsigned char default_value[]
			__attribute__((aligned(__BIGGEST_ALIGNMENT__)));
};

struct cip_sect_schema {
	struct cip_avl_node node;
	struct cip_opt_schema *options;
	unsigned char flags;
};

struct cip_file_schema {
	struct cip_sect_schema *sections;
};

__attribute__((always_inline))
static inline cip_opt_schema *cip_opt_schema_get(const cip_sect_schema *sect,
						 const char *name)
{
	return (cip_opt_schema *)
		cip_avl_get((struct cip_avl_node *)sect->options, name);
}

__attribute__((always_inline))
static inline cip_sect_schema *cip_sect_schema_get(const cip_file_schema *file,
						   const char *name)
{
	return (cip_sect_schema *)
		cip_avl_get((struct cip_avl_node *)file->sections, name);
}

/*
 * Parsed stuff - values.c
 */

__attribute__((always_inline))
static inline cip_ini_value *cip_ini_value_get_p(const cip_ini_sect *sect,
						 const char *name)
{
	return (cip_ini_value *)
		cip_avl_get((struct cip_avl_node *)sect->values, name);
}

__attribute__((always_inline))
static inline cip_ini_sect *cip_ini_inst_get_p(const cip_ini_sect *sect,
					       const char *name)
{
	return (cip_ini_sect *)
		cip_avl_get((struct cip_avl_node *)sect->instances, name);
}

__attribute__((always_inline))
static inline cip_ini_sect *cip_ini_sect_get_p(const cip_ini_file *file,
					       const char *name)
{
	return (cip_ini_sect *)
		cip_avl_get((struct cip_avl_node *)file->sections, name);
}

cip_ini_file *cip_ini_file_new(cip_err_ctx *ctx, const cip_file_schema *schema);

cip_ini_sect *cip_ini_sect_new(cip_err_ctx *ctx, cip_ini_file *file,
			       const cip_sect_schema *schema);

cip_ini_sect *cip_ini_inst_new(cip_err_ctx *ctx, cip_ini_sect *sect,
			       const cip_sect_schema *schema, char *id);

int cip_ini_value_new(cip_err_ctx *ctx, cip_ini_sect *sect,
		      const cip_opt_schema *schema, const void *value);

__attribute__((always_inline))
static inline int cip_ini_value_def(cip_err_ctx *ctx, cip_ini_sect *sect,
				    const cip_opt_schema *schema)
{
	return cip_ini_value_new(ctx, sect, schema, schema->default_value);
}
