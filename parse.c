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

#if 0
#define SUBSTR_DEBUG
#endif

#define _GNU_SOURCE	/* for rawmemchr */

#include "libcip.h"
#include "libcip_p.h"

#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>

/*
 * String splitting and whitespace trimming
 */

struct cip_substr {
	char *start;
	char *end;
	char restore;
};

static inline void cip_str_restore(const struct cip_substr *substr)
{
	*(substr->end) = substr->restore;
}

static void cip_str_trim(struct cip_substr *substr, char *s)
{
	char *end;

	/* It isn't 100% clear that isspace(0) ALWAYS returns 0 */

	while (1) {

		if (*s == 0) {
			substr->start = s;
			substr->end = s;
			substr->restore = 0;
			return;
		}

		if (!isspace(*s))
			break;

		++s;
	}

	/* s now points to the first non-whitespace (and non-0) character */

	end = rawmemchr(s, 0) - 1;

	while (isspace(*end))
		--end;

	++end;

	/* end now points to the char AFTER the last non-whitespace char */

	substr->start = s;
	substr->end = end;
	substr->restore = *end;

	*end = 0;
}

static char *cip_str_split(struct cip_substr *before, char *s, char delim)
{
	char *d;

	d = strchr(s, delim);
	if (d == NULL)
		return NULL;

	*d = 0;
	cip_str_trim(before, s);

	if (before->restore == 0)
		before->restore = delim;
	else
		*d = delim;

	return d + 1;
}

static int cip_str_split_trim(struct cip_substr *before,
			      struct cip_substr *after, char *s, char delim)
{
	char *aftr;

	aftr = cip_str_split(before, s, delim);
	if (aftr == NULL)
		return -1;

	cip_str_trim(after, aftr);

	return 0;
}

/*
 * Actual parsing stuff
 */

struct cip_parse_ctx {
	cip_err_ctx *err;
	cip_file_schema *file_schema;
	cip_ini_file *file;
	cip_ini_sect *sect;
	const char *file_name;
	int (*warning_fn)(const char *warn_msg);
	union {
		int line_num;
		struct {
			char succeeded;
			char deferred;
		};
	};
};

static cip_ini_sect *cip_line_sect_single(struct cip_parse_ctx *ctx,
					  const char *title)
{
	cip_sect_schema *sect_schema;
	cip_ini_sect *sect;

	sect_schema = cip_sect_schema_get(ctx->file_schema, title);
	if (sect_schema == NULL) {
		cip_err(ctx->err, "%s:%d: Unknown section title [%s]",
			ctx->file_name, ctx->line_num, title);
		return NULL;
	}

	if (sect_schema->flags & CIP_SECT_MULTIPLE) {
		cip_err(ctx->err, "%s:%d: Missing ID for section [%s]",
			ctx->file_name, ctx->line_num, title);
		return NULL;
	}

	sect = cip_ini_sect_new(ctx->err, ctx->file, sect_schema);
	if (sect == NULL) {
		cip_err_use(ctx->err, "%s:%d: %s", ctx->file_name,
			    ctx->line_num, cip_last_err(ctx->err));
		return NULL;
	}

	return sect;
}

static cip_ini_sect *cip_line_sect_multi(struct cip_parse_ctx *ctx,
					 const char *title, const char *id)
{
	cip_sect_schema *sect_schema;
	cip_ini_sect *sect, *inst;
	char *id_copy;

	sect_schema = cip_sect_schema_get(ctx->file_schema, title);
	if (sect_schema == NULL) {
		cip_err(ctx->err, "%s:%d: Unknown section title [%s:*]",
			ctx->file_name, ctx->line_num, title);
		return NULL;
	}

	if (!(sect_schema->flags & CIP_SECT_MULTIPLE)) {
		cip_err(ctx->err, "%s:%d: Unexpected ID for section [%s:%s]",
			ctx->file_name, ctx->line_num, title, id);
		return NULL;
	}

	sect = cip_ini_sect_get_p(ctx->file, title);
	if (sect == NULL) {
		sect = cip_ini_sect_new(ctx->err, ctx->file, sect_schema);
		if (sect == NULL) {
			cip_err_use(ctx->err, "%s:%d: %s", ctx->file_name,
				    ctx->line_num, cip_last_err(ctx->err));
			return NULL;
		}
	}

	id_copy = strdup(id);
	if (id_copy == NULL)
		return cip_err_ptr(ctx->err, strerror(ENOMEM));

	inst = cip_ini_inst_new(ctx->err, sect, sect_schema, id_copy);
	if (inst == NULL) {
		cip_err_use(ctx->err, "%s:%d: %s", ctx->file_name,
			    ctx->line_num, cip_last_err(ctx->err));
		free(id_copy);
		return NULL;
	}

	return inst;
}

static int cip_check_remainder(struct cip_parse_ctx *ctx, char *remainder)
{
	struct cip_substr rem;
	int ret;

	if (ctx->warning_fn == 0)
		return 0;

	cip_str_trim(&rem, remainder);

	if (*rem.start == 0 || *rem.start == ';' || *rem.start == '#') {
		ret = 0;
	}
	else {
		cip_err(ctx->err, "%s:%d: Unexpected extra characters",
			ctx->file_name, ctx->line_num);
		ret = ctx->warning_fn(cip_last_err(ctx->err));
	}

	cip_str_restore(&rem);
	return ret;
}

static int cip_parse_sect_cb(struct cip_avl_node *node, void *context)
{
	const cip_opt_schema *opt_schema;
	const cip_ini_value *value;
	struct cip_parse_ctx *ctx;

	opt_schema = (cip_opt_schema *)node;

	if (!(opt_schema->flags & (CIP_OPT_REQUIRED | CIP_OPT_DEFAULT)))
		return 1;

	ctx = context;

	value = cip_ini_value_get(ctx->sect, opt_schema->node.name);
	if (value != NULL)
		return 1;

	if (opt_schema->flags & CIP_OPT_DEFAULT) {

		if (cip_ini_value_def(ctx->err, ctx->sect, opt_schema) == -1)
			return 0;
		else
			return 1;
	}
	else {

		if (ctx->sect->schema->flags & CIP_SECT_MULTIPLE) {

			cip_err(ctx->err, "%s:%d: Section [%s:%s] missing "
				"required option: %s", ctx->file_name,
				ctx->line_num, ctx->sect->schema->node.name,
				ctx->sect->node.name, opt_schema->node.name);
		}
		else {
			cip_err(ctx->err, "%s:%d: Section [%s] missing "
				"required option: %s", ctx->file_name,
				ctx->line_num, ctx->sect->node.name,
				opt_schema->node.name);
		}

		return 0;
	}
}

static int cip_parse_file_cb(struct cip_avl_node *node, void *context)
{
	struct cip_parse_ctx *ctx;
	cip_sect_schema *schema;
	const char *format;

	schema = (cip_sect_schema *)node;
	if (!(schema->flags & (CIP_SECT_REQUIRED | CIP_SECT_CREATE)))
		return 1;

	ctx = context;

	if (cip_ini_sect_get(ctx->file, schema->node.name) != NULL)
		return 1;

	if (schema->flags & CIP_SECT_CREATE) {

		ctx->sect = cip_ini_sect_new(ctx->err, ctx->file, schema);
		if (ctx->sect == NULL)
			return 0;

		if (schema->options != NULL) {

			node = (struct cip_avl_node *)schema->options;
			if (cip_avl_foreach(node, cip_parse_sect_cb, ctx) == 0)
				return 0;
		}
	}
	else {

		if (schema->flags & CIP_SECT_MULTIPLE)
			format = "%s: Missing section [%s:*]";
		else
			format = "%s: Missing section [%s]";

		cip_err(ctx->err, format, ctx->file_name, schema->node.name);
		return 0;
	}

	return 1;
}

static int cip_check_prev_sect(struct cip_parse_ctx *ctx)
{
	const cip_sect_schema *schema;
	struct cip_avl_node *tree;
	cip_ini_sect *sect;

	sect = ctx->sect;
	if (sect == NULL)
		return 0;

	schema = sect->schema;

	if ((schema->flags & CIP_SECT_NOT_EMPTY) && sect->values == NULL) {

		if (schema->flags & CIP_SECT_MULTIPLE) {
			cip_err(ctx->err,
				"%s:%d: Invalid empty section: [%s:%s]",
				ctx->file_name, ctx->line_num,
				schema->node.name, sect->node.name);
		}
		else {
			cip_err(ctx->err, "%s:%d: Invalid empty section: [%s]",
				ctx->file_name, ctx->line_num, sect->node.name);
		}

		return -1;
	}

	tree = (struct cip_avl_node *)schema->options;

	if (tree != NULL) {

		if (cip_avl_foreach(tree, cip_parse_sect_cb, ctx) == 0)
			return -1;
	}

	return 0;
}

static int cip_parse_sect_line(struct cip_parse_ctx *ctx, char *line)
{
	struct cip_substr sect_all, sect_title, sect_id;
	char *remainder, *id;
	cip_ini_sect *sect;

	++line;		/* skip opening bracket */

	remainder = cip_str_split(&sect_all, line, ']');
	if (remainder == NULL) {
		cip_err(ctx->err, "%s:%d: Missing closing bracket (']')",
			ctx->file_name, ctx->line_num);
		return -1;
	}

	id = cip_str_split(&sect_title, sect_all.start, ':');
	if (id == NULL) {
		sect = cip_line_sect_single(ctx, sect_all.start);
		cip_str_restore(&sect_all);
	}
	else {
		cip_str_trim(&sect_id, id);
		sect = cip_line_sect_multi(ctx, sect_title.start,
					   sect_id.start);
		cip_str_restore(&sect_id);
		cip_str_restore(&sect_title);
		cip_str_restore(&sect_all);
	}

	if (sect == NULL)
		return -1;

	if (cip_check_prev_sect(ctx) == -1)
		return -1;

	ctx->sect = sect;

	return cip_check_remainder(ctx, remainder);
}

static int cip_parse_opt_value(struct cip_parse_ctx *ctx,
			       cip_opt_schema *schema, char *value)
{
	unsigned char buf[schema->type->size];
	cip_err_ctx err_ctx;
	const char *err_msg;
	char *remainder;

	cip_err_ctx_init(&err_ctx);

	remainder = schema->type->parse_fn(&err_ctx, buf, value);
	err_msg = cip_last_err(&err_ctx);

	if (remainder == NULL) {
		if (err_msg == NULL)
			err_msg = "Unknown parse error";
		cip_err(ctx->err, "%s:%d: Failed to parse %s: %s",
			ctx->file_name, ctx->line_num, schema->type->name,
			err_msg);
		cip_err_ctx_fini(&err_ctx);
		return -1;
	}

	if (err_msg != NULL && ctx->warning_fn != 0) {
		cip_err(ctx->err, "%s:%d: %s", ctx->file_name, ctx->line_num,
			err_msg);
		if (ctx->warning_fn(cip_last_err(ctx->err)) == -1) {
			cip_err_ctx_fini(&err_ctx);
			return -1;
		}
	}

	cip_err_ctx_fini(&err_ctx);

	if (cip_ini_value_new(ctx->err, ctx->sect, schema, buf) == -1) {
		cip_err_use(ctx->err, "%s:%d: %s", ctx->file_name,
			    ctx->line_num, cip_last_err(ctx->err));
		return -1;
	}

	return cip_check_remainder(ctx, remainder);
}

static int cip_parse_opt_line(struct cip_parse_ctx *ctx, char *line)
{
	struct cip_substr name, value;
	cip_opt_schema *schema;
	int ret;

	if (ctx->sect == NULL) {
		cip_err(ctx->err, "%s:%d: Value outside any section",
			ctx->file_name, ctx->line_num);
		return -1;
	}

	if (cip_str_split_trim(&name, &value, line, '=') == -1) {
		cip_err(ctx->err, "%s:%d: Expected equal sign ('=')",
			ctx->file_name, ctx->line_num);
		return -1;
	}

	schema = cip_opt_schema_get(ctx->sect->schema, name.start);
	if (schema == NULL) {
		cip_err(ctx->err, "%s:%d: Unknown option [%s]:%s",
			ctx->file_name, ctx->line_num, ctx->sect->node.name,
			name.start);
		ret = -1;
	}
	else {
		ret = cip_parse_opt_value(ctx, schema, value.start);
	}

	cip_str_restore(&value);
	cip_str_restore(&name);

	return ret;
}

int cip_parse_line(struct cip_parse_ctx *ctx, char *line)
{
	struct cip_substr trimmed;
	int ret;
#ifdef SUBSTR_DEBUG
	char *copy;

	copy = strdup(line);
	if (copy == NULL)
		abort();
#endif
	cip_str_trim(&trimmed, line);

	switch (*trimmed.start) {

		case 0:
		case '\n':
		case ';':
		case '#':	ret = 0;
				break;

		case '[':	ret = cip_parse_sect_line(ctx, trimmed.start);
				break;

		default:	ret = cip_parse_opt_line(ctx, trimmed.start);
	}

	cip_str_restore(&trimmed);
#ifdef SUBSTR_DEBUG
	if (strcmp(line, copy) != 0)
		abort();

	free(copy);
#endif
	return ret;
}

static void cip_post_parse_err(struct cip_parse_ctx *ctx, cip_ini_value *value,
			       const char *err_msg)
{
	if (ctx->sect->schema->flags & CIP_SECT_MULTIPLE) {

		cip_err(ctx->err, "%s: [%s:%s]:%s: %s", ctx->file_name,
			ctx->sect->schema->node.name, ctx->sect->node.name,
			value->node.name, err_msg);
	}
	else {
		cip_err(ctx->err, "%s: [%s]:%s: %s", ctx->file_name,
			ctx->sect->node.name, value->node.name, err_msg);
	}
}

static int cip_post_value_cb(struct cip_avl_node *node, void *context)
{
	struct cip_parse_ctx *parse_ctx;
	cip_ini_value *value;
	cip_err_ctx err_ctx;
	const char *err_msg;
	int ret;

	value = (cip_ini_value *)node;

	if (value->post_parse_done || value->schema->post_parse_fn == 0)
		return 1;

	parse_ctx = context;
	cip_err_ctx_init(&err_ctx);

	ret = value->schema->post_parse_fn(&err_ctx, value, parse_ctx->sect,
					   parse_ctx->file,
					   value->schema->post_parse_data);

	err_msg = cip_last_err(&err_ctx);

	if (ret < 0) {
		if (err_msg == NULL)
			err_msg = "Unknown post-parse error";
		cip_post_parse_err(parse_ctx, value, err_msg);
		cip_err_ctx_fini(&err_ctx);
		return 0;
	}

	if (err_msg != NULL && parse_ctx->warning_fn != 0) {
		cip_post_parse_err(parse_ctx, value, err_msg);
		if (parse_ctx->warning_fn(cip_last_err(parse_ctx->err)) == -1) {
			cip_err_ctx_fini(&err_ctx);
			return 0;
		}
	}

	cip_err_ctx_fini(&err_ctx);

	if (ret == 0) {
		value->post_parse_done = 1;
		parse_ctx->succeeded = 1;
	}
	else {
		parse_ctx->deferred = 1;
	}

	return 1;
}

static int cip_post_inst_cb(struct cip_avl_node *node, void *context)
{
	struct cip_avl_node *values;
	struct cip_parse_ctx *ctx;
	cip_ini_sect *instance;

	instance = (cip_ini_sect *)node;
	values = (struct cip_avl_node *)instance->values;

	if (values != NULL) {

		ctx = context;
		ctx->sect = instance;

		return cip_avl_foreach(values, cip_post_value_cb, ctx);
	}
	else {
		return 1;
	}
}

static int cip_post_sect_cb(struct cip_avl_node *node, void *context)
{
	struct cip_avl_node *tree;
	struct cip_parse_ctx *ctx;
	cip_ini_sect *section;

	section = (cip_ini_sect *)node;

	if (section->schema->flags & CIP_SECT_MULTIPLE) {

		tree = (struct cip_avl_node *)section->instances;
		/* tree != NULL (must have parsed instance to create section) */
		return cip_avl_foreach(tree, cip_post_inst_cb, context);
	}
	else {
		tree = (struct cip_avl_node *)section->values;

		if (tree != NULL) {

			ctx = context;
			ctx->sect = section;

			return cip_avl_foreach(tree, cip_post_value_cb, ctx);
		}
		else {
			return 1;
		}
	}
}

static int cip_post_parse(struct cip_parse_ctx *ctx)
{
	struct cip_avl_node *sections;

	sections = (struct cip_avl_node *)ctx->file->sections;
	if (sections == NULL)
		return 0;

	do {
		ctx->succeeded = 0;
		ctx->deferred = 0;

		if (cip_avl_foreach(sections, cip_post_sect_cb, ctx) == 0)
			return -1;
	}
	while (ctx->deferred && ctx->succeeded);

	if (ctx->deferred) {
		cip_err(ctx->err, "Infinite loop processing file: %s",
			ctx->file_name);
		return -1;
	}

	return 0;
}

cip_ini_file *cip_parse_stream(cip_err_ctx *err_ctx, FILE *stream,
			       const char *name, cip_file_schema *schema,
			       int (*warning_fn)(const char *warn_msg))
{
	struct cip_avl_node *tree;
	struct cip_parse_ctx ctx;
	char *lineptr;
	size_t n;

	ctx.err = err_ctx;
	ctx.file_schema = schema;
	ctx.file_name = name;
	ctx.warning_fn = warning_fn;

	ctx.file = cip_ini_file_new(ctx.err, ctx.file_schema);
	if (ctx.file == NULL)
		return NULL;

	lineptr = NULL;
	ctx.line_num = 0;
	ctx.sect = NULL;

	if (stream != NULL) {

		while (getline(&lineptr, &n, stream) > 0) {

			++(ctx.line_num);

			if (cip_parse_line(&ctx, lineptr) == -1) {
				free(lineptr);
				cip_ini_file_free(ctx.file);
				return NULL;
			}
		}

		if (ferror(stream)) {
			cip_err(ctx.err, "%s: %m", ctx.file_name);
			free(lineptr);
			cip_ini_file_free(ctx.file);
			return NULL;
		}

		free(lineptr);

		if (cip_check_prev_sect(&ctx) == -1) {
			cip_ini_file_free(ctx.file);
			return NULL;
		}
	}

	tree = (struct cip_avl_node *)schema->sections;
	if (tree != NULL) {
		if (cip_avl_foreach(tree, cip_parse_file_cb, &ctx) == 0) {
			cip_ini_file_free(ctx.file);
			return NULL;
		}
	}

	if (cip_post_parse(&ctx) == -1) {
		cip_ini_file_free(ctx.file);
		return NULL;
	}

	return ctx.file;
}

cip_ini_file *cip_parse_file(cip_err_ctx *err_ctx, const char *file_name,
			     cip_file_schema *schema,
			     int (*warning_fn)(const char *warn_msg))
{
	cip_ini_file *file;
	FILE *stream;

	stream = fopen(file_name, "re");
	if (stream == NULL)
		return cip_err_ptr(err_ctx, "%s: %m", file_name);

	file = cip_parse_stream(err_ctx, stream, file_name, schema, warning_fn);
	if (file == NULL) {
		fclose(stream);		/* don't overwrite error message */
		return NULL;
	}

	if (fclose(stream) == EOF) {
		cip_err(err_ctx, "%s: %m", file_name);
		cip_ini_file_free(file);
		return NULL;
	}

	return file;
}

/*
 * Temporary testing stuff
 */
#if 0
static int cip_dump_value(struct cip_avl_node *node,
			  void *context __attribute__((unused)))
{
	cip_ini_value *value;
	static char buf[500];

	value = (cip_ini_value *)node;
	value->schema->type->format_fn(NULL, buf, sizeof buf, &value->value);
	printf("%s (%s) = %s\n", value->node.name, value->schema->type->name,
	       buf);
	return 1;
}

static int cip_dump_instance(struct cip_avl_node *node,
			     void *context __attribute__((unused)))
{
	cip_ini_sect *inst;

	inst = (cip_ini_sect *)node;
	printf("[ %s : %s ]\n", inst->schema->node.name, inst->node.name);
	cip_avl_foreach(&inst->values->node, cip_dump_value, NULL);
	putchar('\n');
	return 1;
}

static int cip_dump_section(struct cip_avl_node *node,
			    void *context __attribute__((unused)))
{
	cip_ini_sect *sect;

	sect = (cip_ini_sect *)node;

	if (sect->schema->flags & CIP_SECT_MULTIPLE) {
		cip_avl_foreach(&sect->instances->node, cip_dump_instance,
				NULL);
	}
	else {
		printf("[ %s ]\n", sect->node.name);
		cip_avl_foreach(&sect->values->node, cip_dump_value, NULL);
		putchar('\n');
	}

	return 1;
}

void cip_dump_file(cip_ini_file *file)
{
	cip_avl_foreach(&file->sections->node, cip_dump_section, NULL);
}
#endif
