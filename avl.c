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

void cip_avl_free(struct cip_avl_node *tree,
		  void (*free_fn)(struct cip_avl_node *node))
{
	if (tree->left)
		cip_avl_free(tree->left, free_fn);
	if (tree->right)
		cip_avl_free(tree->right, free_fn);

	if (free_fn != 0)
		free_fn(tree);

	free(tree);
}

static struct cip_avl_node *cip_avl_promote_right(struct cip_avl_node *left)
{
	struct cip_avl_node *right;
	int new_left_skew;

	right = left->right;
	left->right = right->left;
	right->left = left;

	new_left_skew = -1 + left->skew - (right->skew > 0 ? right->skew : 0);
	if (new_left_skew < 0) {
		right->skew = -2 + left->skew +
				(right->skew < 0 ? right->skew : 0);
	}
	else {
		--right->skew;
	}
	left->skew = new_left_skew;

	return right;
}

static struct cip_avl_node *cip_avl_promote_left(struct cip_avl_node *right)
{
	struct cip_avl_node *left;
	int new_right_skew;

	left = right->left;
	right->left = left->right;
	left->right = right;

	new_right_skew = 1 + right->skew - (left->skew < 0 ? left->skew : 0);
	if (new_right_skew > 0) {
		left->skew = 2 + right->skew +
				(left->skew > 0 ? left->skew : 0);
	}
	else {
		++left->skew;
	}
	right->skew = new_right_skew;

	return left;
}

int cip_avl_add(struct cip_avl_node **tree_ptr, struct cip_avl_node *new)
{
	struct cip_avl_node *tree;
	int ret;

	new->left = NULL;
	new->right = NULL;
	new->skew = 0;

	tree = *tree_ptr;

	if (tree == NULL) {
		*tree_ptr = new;
		return 1;
	}

	ret = strcmp(new->name, tree->name);
	if (ret < 0) {

		ret = cip_avl_add(&tree->left, new);
		if (ret < 1)
			return ret;

		if (--tree->skew > -2)
			return -tree->skew;

		if (tree->left->skew == 1)
			tree->left = cip_avl_promote_right(tree->left);

		*tree_ptr = cip_avl_promote_left(tree);
		return 0;
	}
	else if (ret > 0) {
		ret = cip_avl_add(&tree->right, new);
		if (ret < 1)
			return ret;

		if (++tree->skew < 2)
			return tree->skew;

		if (tree->right->skew == -1)
			tree->right = cip_avl_promote_left(tree->right);

		*tree_ptr = cip_avl_promote_right(tree);
		return 0;
	}
	else {
		return -1;
	}
}

struct cip_avl_node *cip_avl_get(struct cip_avl_node *tree, const char *name)
{
	int ret;

	while (tree != NULL) {
		ret = strcmp(name, tree->name);
		if (ret < 0)
			tree = tree->left;
		else if (ret > 0)
			tree = tree->right;
		else
			return tree;
	}

	return NULL;
}

static int cip_avl_foreach_int(struct cip_avl_node *tree,
			       int (*callback_fn)(struct cip_avl_node *node,
						  void *context),
			       void *context)
{
	if (tree->left) {
		if (!cip_avl_foreach(tree->left, callback_fn, context))
			return 0;
	}

	if (!callback_fn(tree, context))
		return 0;

	if (tree->right) {
		if (!cip_avl_foreach(tree->right, callback_fn, context))
			return 0;
	}

	return 1;
}

int cip_avl_foreach(struct cip_avl_node *tree,
		    int (*callback_fn)(struct cip_avl_node *node,
				       void *context),
		    void *context)
{
	if (tree == NULL)
		return 1;

	return cip_avl_foreach_int(tree, callback_fn, context);
}
