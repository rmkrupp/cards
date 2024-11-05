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

    /* these three properties must stay in this order because we cast a pointer
     * to the key field to a struct sorted_set_lookup_result pointer
     */
    char * key;
    size_t length;
    void * data;
};

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

/* destroy this sorted set without free'ing the keys */
void sorted_set_destroy_except_keys(
        struct sorted_set * sorted_set) [[gnu::nonnull(1)]]
{
    if (sorted_set->layers > 0) {
        struct node * node = sorted_set->next[0];
        while (node) {
            struct node * next = node->next[0];
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
 *
 * TODO: do we need a non-copying version?
 */
enum sorted_set_add_key_result sorted_set_add_key(
        struct sorted_set * sorted_set,
        const char * key,
        size_t length,
        void * data
    ) [[gnu::nonnull(1, 2)]]
{
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
    new_node->key[length] = '\0';

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
    ) [[gnu::nonnull(1, 2)]]
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

/* apply this function to every key in sorted order while destroying the
 * sorted set.
 *
 * the value of the key is passed as a non-const to the callback and must
 * either be retained or free'd as (unlike sorted_set_destroy) this function
 * does not free it when destroying the sorted_set.
 *
 * the ptr passed to sorted_set_apply is passed to the callback as well.
 *
 * TODO could add a "stop traversal" return from fn
 */
void sorted_set_apply_and_destroy(
        struct sorted_set * sorted_set,
        void (*fn)(char * key, size_t length, void * data, void * ptr),
        void * ptr
    ) [[gnu::nonnull(1, 2)]]
{
    if (sorted_set->layers == 0) {
        return;
    }

    struct node * node = sorted_set->next[0];
    while (node) {
        fn(node->key, node->length, node->data, ptr);

        struct node * next = node->next[0];
        free(node->next);
        free(node);

        node = next;
    }

    free(sorted_set->next);
    free(sorted_set);
}

/* find this key in the sorted set and return a const pointer to it, or NULL
 * if it's not in the set
 */
const struct sorted_set_lookup_result * sorted_set_lookup(
        struct sorted_set * sorted_set,
        const char * key,
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    size_t layer = sorted_set->layers - 1;
    struct node * node = (struct node *)sorted_set;

    for (;;) {
        while (node->next[layer]) {
            int compare = key_compare(
                    &(struct node) { .key = (char *)key, .length = length },
                    node->next[layer]
                );
            if (compare == 0) {
                return (const struct sorted_set_lookup_result *)
                    &node->next[layer]->key;
            }

            if (compare > 0) {
                node = node->next[layer];
            }

            if (compare < 0) {
                break;
            }
        }

        if (layer == 0) {
            break;
        }
        layer--;
    }

    return NULL;
}

/* a sorted_set_maker
 *
 * this allows insertion of pre-sorted keys into a sorted_set in O(1) time
 * when the number of keys is known ahead of time
 */
struct sorted_set_maker {
    struct sorted_set * sorted_set; /* the embedded sorted_set */
    struct node * next; /* where will the next added key go? */
};

/* create a sorted_set_maker that will make a sorted sorted with this number
 * of keys
 *
 * it is an error to call this with n_keys == 0
 */
[[nodiscard]] struct sorted_set_maker * sorted_set_maker_create(
        size_t n_keys)
{
    assert(n_keys > 0);

    struct sorted_set_maker * sorted_set_maker =
        malloc(sizeof(*sorted_set_maker));

    *sorted_set_maker = (struct sorted_set_maker) {
        .sorted_set = malloc(sizeof(*sorted_set_maker->sorted_set))
    };

    struct sorted_set * sorted_set = sorted_set_maker->sorted_set;

    size_t layers = 0;
    for (size_t n = n_keys; n > 1; n /= 2) {
        layers++;
    }

    size_t * landmarks = malloc(sizeof(*landmarks) * layers);
    size_t i = layers - 1;
    for (size_t n = n_keys / 2; n > 1; n /= 2) {
        landmarks[i] = n;
        i--;
    }

    sorted_set->layers = layers;
    sorted_set->next = malloc(sizeof(*sorted_set->next) * layers);
    sorted_set->size = 0;

    struct node ** update = malloc(sizeof(*update) * layers);
    for (size_t i = 0; i < layers; i++) {
        update[i] = (struct node *)sorted_set;
    }

    for (size_t i = 0; i < n_keys; i++) {
        size_t layer = 0;
        for (size_t j = layers - 1; j > 0; j--) {
            if (i % landmarks[j] == 0) {
                layer = j;
                break;
            }
        }
        layer++;

        struct node * node = malloc(sizeof(*node));
        *node = (struct node) {
            .next = malloc(sizeof(*node->next) * (layer))
        };

        for (size_t j = 0; j < layer; j++) {
            update[j]->next[j] = node;
            update[j] = node;
        }
    }

    for (size_t i = 0; i < layers; i++) {
        update[i]->next[i] = NULL;
    }

    sorted_set_maker->next = sorted_set->next[0];
    free(update);
    free(landmarks);
    return sorted_set_maker;
}

/* returns true if the number of keys added to this sorted_set_maker is equal
 * to the number of keys preallocated on its creation
 */
bool sorted_set_maker_complete(
        const struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]]
{
    return !sorted_set_maker->next;
}

/* finalize this sorted_set_maker, destroying it and returning the sorted_set
 * that was made
 *
 * this must be called after a number of keys have been added to the maker
 * equal to the number that were preallocated.
 */
struct sorted_set * sorted_set_maker_finalize(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]]
{
    assert(sorted_set_maker_complete(sorted_set_maker));
    struct sorted_set * sorted_set = sorted_set_maker->sorted_set;
    free(sorted_set_maker);
    return sorted_set;
}

/* destroy this sorted_set_maker and any partially-constructed set inside it,
 * and free any keys
 */
void sorted_set_maker_destroy(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]]
{
    struct node * node = sorted_set_maker->sorted_set->next[0];
    while (node != sorted_set_maker->next) {
        struct node * next = node->next[0];
        free(node->next);
        free(node->key);
        free(node);
        node = next;
    }
    while (node) {
        struct node * next = node->next[0];
        free(node->next);
        free(node);
        node = next;
    }
    free(sorted_set_maker->sorted_set->next);
    free(sorted_set_maker->sorted_set);
    free(sorted_set_maker);
}

/* destroy this sorted_set_maker and any partially-constructed set inside it,
 * but do not free any keys
 */
void sorted_set_maker_destroy_except_keys(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]]
{
    struct node * node = sorted_set_maker->sorted_set->next[0];
    while (node) {
        struct node * next = node->next[0];
        free(node->next);
        free(node);
        node = next;
    }
    free(sorted_set_maker->sorted_set->next);
    free(sorted_set_maker->sorted_set);
    free(sorted_set_maker);
}

/* add this key to this sorted_set_maker
 *
 * returns true if the sorted_set_maker is now complete
 *
 * it is an error to call this on a complete sorted_set_maker
 */
bool sorted_set_maker_add_key(
        struct sorted_set_maker * sorted_set_maker,
        char * key,
        size_t length,
        void * data
    ) [[gnu::nonnull(1, 2)]]
{
    sorted_set_maker->next->key = key;
    sorted_set_maker->next->length = length;
    sorted_set_maker->next->data = data;
    sorted_set_maker->next = sorted_set_maker->next->next[0];
    return !sorted_set_maker->next;
}
