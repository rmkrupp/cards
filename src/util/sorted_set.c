/* File: src/util/sorted_set.h
 * Part of cards <github.com/rmkrupp/cards>
 *
 * Copyright (C) 2024 Noah Santer <n.ed.santer@gmail.com>
 * Copyright (C) 2024 Rebecca Krupp <beka.krupp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "util/sorted_set.h"

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

/* a sorted set */
struct sorted_set {
    struct node * root;
    size_t depth_hint;
};

/* a node in the BST */
struct node {
    struct node * left,
                * right;
    char * key;
    size_t length;
    void * data;
};

/* do a post-order traversal (e.g. for freeing), applying fn to each node
 *
 * preallocate a traversal stack of size depth_hint
 *
 * returns the largest stack required
 */
static size_t sorted_set_postorder_traversal(
        struct node * root,
        size_t depth_hint,
        void (*fn)(struct node * node)
    ) [[gnu::nonnull(1, 3)]]
{
    struct node ** stack = malloc(sizeof(*stack) * (depth_hint + 1));
    size_t stack_capacity = depth_hint + 1;
    size_t stack_size = 0;
    size_t stack_max = 0;

    struct node * last = NULL;
    struct node * node = root;

    while (node || stack_size) {
        if (node) {
            if (stack_size == stack_capacity) {
                stack = realloc(stack, sizeof(*stack) * (stack_capacity + 1));
                stack_capacity++;
            }
            stack[stack_size] = node;
            if (stack_size == stack_max) {
                stack_max++;
            }
            stack_size++;

            node = node->left;
        } else {
            struct node * peek = stack[stack_size - 1];
            if (peek->right && last != peek->right) {
                node = peek->right;
            } else {
                fn(peek);
                last = stack[--stack_size];
            }
        }
    }

    free(stack);
    return stack_max;
}

/* create an empty sorted set */
[[nodiscard]] struct sorted_set * sorted_set_create()
{
    struct sorted_set * sorted_set = malloc(sizeof(*sorted_set));
    *sorted_set = (struct sorted_set) { };
    return sorted_set;
}

static void free_node(struct node * node)
{
    free(node->key);
    free(node);
}

/* destroy this sorted set */
void sorted_set_destroy(struct sorted_set * sorted_set) [[gnu::nonnull(1)]]
{
    sorted_set_postorder_traversal(
            sorted_set->root, sorted_set->depth_hint, &free_node);
    free(sorted_set);
}

/* returns 0 when equal, negative when a < b, positive when a > b */
static int key_compare(const struct node * a, const struct node * b)
{
    size_t length = a->length > b->length ? a->length : b->length;
    for (size_t i = 0; i < length; i++) {
        if ((unsigned char)a->key[i] != (unsigned char)b->key[i]) {
            return (int)(unsigned char)a->key[i] -
                (int)(unsigned char)b->key[i];
        }
    }
    return 0;
}

/* add this key of length to the sorted set, associating it with data
 *
 * returns SORTED_SET_ADD_KEY_UNIQUE if the key was not already in the set,
 * or SORTED_SET_ADD_KEY_DUPLICATE otherwise
 */
enum sorted_set_add_key_result sorted_set_add_key(
        struct sorted_set * sorted_set,
        const char * key,
        size_t length,
        void * data
    ) [[gnu::nonnull(1, 2)]]
{
    // TODO make a non-copying version
    // TODO this is an unbalanced BST

    char * key_copy = malloc(length + 1);
    for (size_t i = 0; i < length; i++) {
        key_copy[i] = key[i];
    }
    key_copy[length] = '\0';

    struct node * z = malloc(sizeof(*z));
    *z = (struct node) {
        .key = key_copy,
        .length = length,
        .data = data
    };

    struct node * y = NULL;
    struct node * x = sorted_set->root;
    while (x) {
        y = x;

        int compare = key_compare(z, x);

        if (compare == 0) {
            free(z->key);
            free(z);
            return SORTED_SET_ADD_KEY_DUPLICATE;
        }
        
        if (compare < 0) { // z < x
            x = x->left;
        } else { // z > x
            x = x->right;
        }
    }

    if (!y) {
        sorted_set->root = z;
        return SORTED_SET_ADD_KEY_UNIQUE;
    }

    int compare = key_compare(z, y);

    if (compare == 0) {
        free(z->key);
        free(z);
        return SORTED_SET_ADD_KEY_UNIQUE;
    }

    if (compare < 0) {
        y->left = z;
    } else {
        y->right = z;
    }

    return SORTED_SET_ADD_KEY_UNIQUE;
}

/* do a post-order traversal (e.g. for freeing), applying fn to each node
 *
 * preallocate a traversal stack of size depth_hint
 *
 * returns the largest stack required
 */

/* apply this function to every key in sorted order
 *
 * the ptr passed to sorted_set_apply is passed to the callback as well
 *
 * this does an in-order traversal. we preallocate a stack of size
 * sorted_set->depth_hint + 1 and also update that depth_hint after traversal
 * to the confirmed value.
 *
 * TODO could add a "stop traversal" return from fn
 */
void sorted_set_apply(
        struct sorted_set * sorted_set,
        void (*fn)(const char * key, size_t length, void * data, void * ptr),
        void * ptr
    )
{
    if (!sorted_set->root) {
        return;
    }

    struct node ** stack =
        malloc(sizeof(*stack) * (sorted_set->depth_hint + 1));
    size_t stack_capacity = sorted_set->depth_hint + 1;
    size_t stack_size = 0;
    size_t stack_max = 0;

    struct node * node = sorted_set->root;

    while (stack_size || node) {
        if (node) {
            if (stack_size == stack_capacity) {
                stack = realloc(stack, sizeof(*stack) * (stack_capacity + 1));
                stack_capacity++;
            }
            stack[stack_size] = node;
            if (stack_size == stack_max) {
                stack_max++;
            }
            stack_size++;

            node = node->left;
        } else {
            node = stack[--stack_size];
            fn(node->key, node->length, node->data, ptr);
            node = node->right;
        }
    }

    free(stack);
}
