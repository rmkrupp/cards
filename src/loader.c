/* File: src/loader.c
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
#include "loader.h"

#include <stdlib.h>

#include "util/sorted_set.h"
#include "hash.h"

/* TODO: get rid of this when load_bundle is not a stub */
#include <stdio.h>

/* TODO: is there a better API solution vs all the casting of consts that
 *       happen in the intersection of this, hash, and sorted_set when using
 *       the non-copy APIs?
 */

/* a set for looking up name tokens */
struct name_set {
    struct hash * hash;
    struct sorted_set * uncompiled;
};

/* create an empty name set */
[[nodiscard]] struct name_set * name_set_create()
{
    struct name_set * name_set = malloc(sizeof(*name_set));
    *name_set = (struct name_set) {
        .uncompiled = sorted_set_create()
    };
    return name_set;
}

/* destroy a name set and free the names its holding */
void name_set_destroy(struct name_set * name_set) [[gnu::nonnull(1)]]
{
    if (name_set->hash) {
        hash_destroy(name_set->hash);
    }
    sorted_set_destroy(name_set->uncompiled);
    free(name_set);
}

/* load names from a file
 *
 * TODO: implement
 */
void name_set_load_bundle(
        struct name_set * name_set,
        const char * filename
    ) [[gnu::nonnull(1, 2)]]
{
    (void)filename;
    char data[64];
    for (size_t i = 0; i < 100000; i++) {
        for (size_t j = 0; j < 64; j++) {
            data[j] = rand() % 26 + 'a';
        }
        sorted_set_add_key(name_set->uncompiled, data, 64, NULL);
    }
    sorted_set_add_key(name_set->uncompiled, "boston", 6, NULL);

    name_set_compile(name_set);

    if (!name_set->hash) {
        printf("hashing failed\n"); 
    }

    const struct name * result = name_set_lookup(name_set, "boston", 6);

    if (result) {
        printf("result: %s\n", result->name);
    } else {
        printf("no result\n");
    }
}

/* callback for the apply call in name_set_compile */
static void add_to_hash_inputs(
        const char * key, size_t length, void * data, void * ptr)
{
    struct hash_inputs * hash_inputs = ptr;
    /* discarding const on purpose (but see TODO) */
    hash_inputs_add_no_copy(hash_inputs, (char *)key, length, data);
}

/* take all the keys in uncompiled and try and put them in hash
 *
 * if there's already a hash, destroy it
 *
 * this means we need to take all the keys out of the set in the event that
 * we do succeed in making the hash (because hash_destroy would free them,
 * and because we want the set of remaining uncompiled keys to be as small as
 * possible.)
 */
void name_set_compile(struct name_set * name_set) [[gnu::nonnull(1)]]
{
    if (name_set->hash) {
        hash_destroy(name_set->hash);
    }
    
    struct hash_inputs * hash_inputs = hash_inputs_create();

    hash_inputs_at_least(hash_inputs, sorted_set_size(name_set->uncompiled));
    sorted_set_apply(name_set->uncompiled, &add_to_hash_inputs, hash_inputs);

    name_set->hash = hash_create(hash_inputs);

    hash_inputs_destroy_except_keys(hash_inputs);

    if (name_set->hash) {
        sorted_set_destroy_except_keys(name_set->uncompiled);
        name_set->uncompiled = sorted_set_create();
    }
}

/* look up a name in this set */
const struct name * name_set_lookup(
        struct name_set * name_set,
        const char * name, 
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    if (name_set->hash) {
        const struct hash_lookup_result * hash_result =
            hash_lookup(name_set->hash, name, length);

        if (hash_result) {
            return (const struct name *)hash_result;
        }
    }

    const struct sorted_set_lookup_result * sorted_set_result =
        sorted_set_lookup(name_set->uncompiled, name, length);

    if (sorted_set_result) {
        return (const struct name *)sorted_set_result;
    }

    return NULL;
}
