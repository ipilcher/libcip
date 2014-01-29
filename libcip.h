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

#ifndef CIP_LIBCIP_H
#define CIP_LIBCIP_H

#include <stdlib.h>
#include <stdio.h>

#pragma GCC visibility push(default)

/*
 * API Types
 */

typedef struct cip_err_ctx cip_err_ctx;
typedef struct cip_opt_type cip_opt_type;
typedef struct cip_opt_info cip_opt_info;
typedef struct cip_sect_info cip_sect_info;
typedef struct cip_opt_schema cip_opt_schema;
typedef struct cip_sect_schema cip_sect_schema;
typedef struct cip_file_schema cip_file_schema;
typedef struct cip_ini_value cip_ini_value;
typedef struct cip_ini_sect cip_ini_sect;
typedef struct cip_ini_file cip_ini_file;

/*
 * Error reporting
 */

struct cip_err_ctx {
	char *err_buf;
	size_t buf_size;
	int static_err;
};

__attribute__((format(printf, 2, 3)))
void cip_err(cip_err_ctx *ctx, const char *format, ...);

cip_err_ctx *cip_err_ctx_init(cip_err_ctx *ctx);
void cip_err_ctx_fini(cip_err_ctx *ctx);
const char *cip_last_err(const cip_err_ctx *ctx);

/*
 * Schema stuff
 */

#define CIP_OPT_REQUIRED	0x01
#define CIP_OPT_DEFAULT		0x02

#define CIP_SECT_REQUIRED	0x01
#define CIP_SECT_MULTIPLE	0x02
#define CIP_SECT_NOT_EMPTY	0x04

struct cip_opt_type {
	const char *name;
	char *(*parse_fn)(cip_err_ctx *ctx, void *value, char *s);
	int (*format_fn)(cip_err_ctx *ctx, char *buf, size_t size,
			 const void *value);
	void (*free_fn)(void *value);
	size_t size;
};

struct cip_opt_info {
	char *name;
	const cip_opt_type *type;
	int (*post_parse_fn)(cip_err_ctx *ctx, const cip_ini_value *value,
			     const cip_ini_sect *sect, const cip_ini_file *file,
			     void *post_parse_data);
	void *post_parse_data;
	const void *default_value;
	unsigned char flags;
};

struct cip_sect_info {
	char *name;
	const cip_opt_info *options;
	unsigned char flags;
};

void cip_file_schema_free(cip_file_schema *file_schema);

cip_file_schema *cip_file_schema_new1(cip_err_ctx *ctx);

cip_file_schema *cip_file_schema_new2(cip_err_ctx *ctx,
				      const cip_sect_info *sections);

cip_sect_schema *cip_sect_schema_new1(cip_err_ctx *ctx,
				      cip_file_schema *file_schema, char *name,
				      unsigned char flags);

cip_sect_schema *cip_sect_schema_new2(cip_err_ctx *ctx,
				      cip_file_schema *file_schema,
				      const cip_sect_info *section);

int cip_sect_schema_new3(cip_err_ctx *ctx, cip_file_schema *file_schema,
			 const cip_sect_info *sections);

int cip_opt_schema_new1(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			char *name, const cip_opt_type *type,
		        int (*post_parse_fn)(cip_err_ctx *ctx,
					     const cip_ini_value *value,
					     const cip_ini_sect *sect,
					     const cip_ini_file *file,
			                     void *post_parse_data),
		        void *post_parse_data, unsigned char flags,
		        const void *default_value);

int cip_opt_schema_new2(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			const cip_opt_info *option);

int cip_opt_schema_new3(cip_err_ctx *ctx, cip_sect_schema *sect_schema,
			const cip_opt_info *options);

/*
 * Built-in option types
 */

typedef struct cip_int_list cip_int_list;
typedef struct cip_short_list cip_short_list;
typedef struct cip_float_list cip_float_list;
typedef struct cip_str_list cip_str_list;
typedef struct cip_bool_list cip_bool_list;

struct cip_int_list {
	int *values;
	unsigned count;
};

struct cip_short_list {
	short *values;
	unsigned count;
};

struct cip_float_list {
	float *values;
	unsigned count;
};

struct cip_str_list {
	char **values;
	unsigned count;
};

struct cip_bool_list {
	_Bool *values;
	unsigned count;
};

extern const cip_opt_type cip_opt_type_int;
extern const cip_opt_type cip_opt_type_int_list;
extern const cip_opt_type cip_opt_type_float;
extern const cip_opt_type cip_opt_type_float_list;
extern const cip_opt_type cip_opt_type_string;
extern const cip_opt_type cip_opt_type_str_list;
extern const cip_opt_type cip_opt_type_short;
extern const cip_opt_type cip_opt_type_short_list;
extern const cip_opt_type cip_opt_type_bool;
extern const cip_opt_type cip_opt_type_bool_list;

#define CIP_OPT_TYPE_INT	(&cip_opt_type_int)
#define CIP_OPT_TYPE_INT_LIST	(&cip_opt_type_int_list)
#define CIP_OPT_TYPE_FLOAT	(&cip_opt_type_float)
#define CIP_OPT_TYPE_FLOAT_LIST	(&cip_opt_type_float_list)
#define CIP_OPT_TYPE_STRING	(&cip_opt_type_string)
#define CIP_OPT_TYPE_STR_LIST	(&cip_opt_type_str_list)
#define CIP_OPT_TYPE_SHORT	(&cip_opt_type_short)
#define CIP_OPT_TYPE_SHORT_LIST	(&cip_opt_type_short_list)
#define CIP_OPT_TYPE_BOOL	(&cip_opt_type_bool)
#define CIP_OPT_TYPE_BOOL_LIST	(&cip_opt_type_bool_list)

/*
 * AVL tree
 */

struct cip_avl_node {
	char *name;
	struct cip_avl_node *left;
	struct cip_avl_node *right;
	int skew;
};

struct cip_avl_node *cip_avl_get(struct cip_avl_node *tree, const char *name);

/*
 * Parsed values
 */

struct cip_ini_value {
	struct cip_avl_node node;
	const cip_opt_schema *schema;
	char post_parse_done;
	unsigned char value[] __attribute__((aligned));
};

struct cip_ini_sect {
	struct cip_avl_node node;
	const cip_sect_schema *schema;
	union {
		cip_ini_value *values;
		cip_ini_sect *instances;
	};
};

struct cip_ini_file {
	const cip_file_schema *schema;
	cip_ini_sect *sections;
};

__attribute__((always_inline))
static inline const cip_ini_value *cip_ini_value_get(const cip_ini_sect *sect,
						     const char *name)
{
	return (cip_ini_value *)
		cip_avl_get((struct cip_avl_node *)sect->values, name);
}

__attribute__((always_inline))
static inline const cip_ini_sect *cip_ini_inst_get(const cip_ini_sect *sect,
						   const char *name)
{
	return (cip_ini_sect *)
		cip_avl_get((struct cip_avl_node *)sect->instances, name);
}

__attribute__((always_inline))
static inline const cip_ini_sect *cip_ini_sect_get(const cip_ini_file *file,
						   const char *name)
{
	return (cip_ini_sect *)
		cip_avl_get((struct cip_avl_node *)file->sections, name);
}

void cip_ini_file_free(cip_ini_file *file);

/*
 * Parsing
 */

cip_ini_file *cip_parse_stream(cip_err_ctx *err_ctx, FILE *stream,
			       const char *name, cip_file_schema *schema,
			       int (*warning_fn)(const char *warn_msg));

cip_ini_file *cip_parse_file(cip_err_ctx *err_ctx, const char *file_name,
			     cip_file_schema *schema,
			     int (*warning_fn)(const char *warn_msg));

/*
 * Type helpers
 */

void *cip_list_parse(char **remainder, unsigned *count, cip_err_ctx *ctx,
		     char *s, const cip_opt_type *type);

int cip_list_format(cip_err_ctx *ctx, char *buf, size_t size, void *values,
		    size_t count, const cip_opt_type *type);

#pragma GCC visibility pop

#endif		/* CIP_LIBCIP_H */
