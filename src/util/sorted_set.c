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

#include <stdint.h>
#include <stdlib.h>
#include <assert.h>

/*
 * TODO: do we need some sort of apply_and_destroy function or similar to allow
 *       a zero-copy move of key from sorted_set to hash_inputs (would also
 *       have to add a non-copying add to hash_inputs in the hash lib)?
 *
 *       or a dedicated sorted_set->hash function that does this? is there a
 *       way to be smart about handingling hash_create failing? (is that even
 *       needed? can that just be a slow path where we recreate the set?)
 */

/* a sorted set */
struct sorted_set {
    struct node ** next;
    size_t layers;
    size_t size;
};

/* a node in the BST */
struct node {
    struct node ** next; /* this property must line up with sorted_set because
                          * we cast sorted_set into node in the add function
                          *
                          * only next is used from this, though.
                          */

    char * key;
    size_t length;
    void * data;
};

#include <stdio.h>

void sorted_set_dump(struct sorted_set * sorted_set) [[gnu::nonnull(1)]]
{
    printf("digraph dump {\n");
    for (size_t layer = 0; layer < sorted_set->layers; layer++) {
        struct node * node = sorted_set->next[layer];
        printf("\"root %lu\"\n", layer);
        printf("\"tail %lu\"\n", layer);
        if (node) {
            printf("\"root %lu\" -> \"%p %lu\"\n", layer, node, layer);
            while (node) {
                if (node->next[layer]) {
                    printf("\"%p %lu\" -> \"%p %lu\"\n", node, layer, node->next[layer], layer);
                } else {
                    printf("\"%p %lu\" -> \"tail %lu\"", node, layer, layer);
                }
                node = node->next[layer];
            }
        }
    }
    printf("}\n");
}

/* create an empty sorted set */
[[nodiscard]] struct sorted_set * sorted_set_create()
{
    struct sorted_set * sorted_set = malloc(sizeof(*sorted_set));
    *sorted_set = (struct sorted_set) {
        .next = malloc(sizeof(*sorted_set->next)),
        .layers = 1
    };
    sorted_set->next[0] = NULL;
    return sorted_set;
}

/* destroy this sorted set */
void sorted_set_destroy(struct sorted_set * sorted_set) [[gnu::nonnull(1)]]
{
    if (sorted_set->layers > 0) {
        struct node * node = sorted_set->next[0];
        while (node) {
            struct node * next = node->next[0];
            free(node->key);
            free(node->next);
            free(node);
            node = next;
        }
        free(sorted_set->next);
    }
    free(sorted_set);
}

size_t sorted_set_size(struct sorted_set * sorted_set) [[gnu::nonnull(1)]]
{
    return sorted_set->size;
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

static size_t random_level()
{
    size_t level = 1;
    for (;;) {
        uint32_t x = rand();
        /* RAND_MAX is at least (1<<15)-1 */
        for (size_t i = 0; i < 15; i++) {
            if (x & 1) {
                return level;
            }
            x >>= 1;
            level++;
        }
    }
    /* you know, if you win the lottery, this could overflow the size_t.
     * it would take a long time, though.
     */
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

    size_t new_level = random_level();

    struct node * new_node = malloc(sizeof(*new_node));
    *new_node = (struct node) {
        .key = (char *)key, /* cast away the const, we're going to make our
                             * own copy of key down below if we actually keep
                             * it but for now we don't have to
                             */
        .length = length,
        .data = data
    };

    struct node ** update = malloc(sizeof(*update) * sorted_set->layers);
    for (size_t i = new_level; i < sorted_set->layers; i++) {
        update[i] = NULL;
    }

    size_t layer = sorted_set->layers - 1;
    struct node * node = (struct node *)sorted_set;

    for (;;) {
        while (node->next[layer]) {
            int compare = key_compare(new_node, node->next[layer]);
            if (compare == 0) {
                // duplicate
                free(new_node);
                free(update);
                return SORTED_SET_ADD_KEY_DUPLICATE;
            }

            if (compare > 0) {
                node = node->next[layer];
            }

            if (compare < 0) {
                break;
            }
        }

        update[layer] = node;

        if (layer == 0) {
            break;
        }
        layer--;
    }

    /* at this point we know the key isn't a duplicate */
    sorted_set->size++;

    new_node->key = malloc(length + 1);
    for (size_t i = 0; i < length; i++ ) {
        new_node->key[i] = key[i];
    }

    new_node->next = malloc(sizeof(*new_node->next) * new_level);

    if (new_level <= sorted_set->layers) {
        for (size_t i = 0; i < new_level; i++) {
            new_node->next[i] = update[i]->next[i];
            update[i]->next[i] = new_node;
        }
    } else {
        for (size_t i = 0; i < sorted_set->layers; i++) {
            new_node->next[i] = update[i]->next[i];
            update[i]->next[i] = new_node;
        }

        sorted_set->next = realloc(
                sorted_set->next,
                sizeof(*sorted_set->next) * new_level
            );

        for (size_t i = sorted_set->layers; i < new_level; i++) {
            sorted_set->next[i] = new_node;
            new_node->next[i] = NULL;
        }

        sorted_set->layers = new_level;
    }

    free(update);

    return SORTED_SET_ADD_KEY_UNIQUE;
}

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
    if (sorted_set->layers == 0) {
        return;
    }

    struct node * node = sorted_set->next[0];
    while (node) {
        fn(node->key, node->length, node->data, ptr);
        node = node->next[0];
    }
}
