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

/* add this name to this name set
 *
 * returns true if the key is added, false otherwise (because it was a
 * duplicate)
 */
bool name_set_add(
        struct name_set * name_set,
        const char * name,
        size_t length,
        void * data
    ) [[gnu::nonnull(1, 2)]]
{
    return sorted_set_add_key(
            name_set->uncompiled, name, length, data) ==
        SORTED_SET_ADD_KEY_UNIQUE;
}

/* remove this name from this set
 *
 * will not remove it if the name_set has been compiled and it's in the hash
 */
struct name_data * name_set_remove(
        struct name_set * name_set,
        const char * name,
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    return sorted_set_remove_key(name_set->uncompiled, name, length);
}

/* callback for the first apply call in name_set_compile */
static void add_to_hash_inputs(
        char * key, size_t length, void * data, void * ptr)
{
    struct hash_inputs * hash_inputs = ptr;
    hash_inputs_add_no_copy(hash_inputs, key, length, data);
}

/* callback for the second apply call in name_set_compile */
static void add_to_sorted_set_maker(
        char * key, size_t length, void * data, void * ptr)
{
    struct sorted_set_maker * sorted_set_maker = ptr;
    sorted_set_maker_add_key(sorted_set_maker, key, length, data);
}

/* take all the keys in uncompiled and try and put them in hash
 *
 * 1. create an empty hash_inputs and make sure it has space to store the
 *    whole sorted_set
 *
 * 2. move every key from the sorted_set into it, and destroy the sorted set
 *
 * 3. attempt to create the hash
 *
 * 4. (if 3 succeeds) destroy the now-empty hash_inputs and create a fresh
 *    name_set->uncompiled sorted_set
 *
 * 4. (if 3 fails) using a sorted_set_maker, recreate the original sorted_set
 *    in O(n) time and put the copy in name_set->uncompiled
 *
 * note: if we destroy the sorted_set as we put its keys into the
 *       hash_inputs (i.e. sorted_set_apply_and_destroy) then the hash-success
 *       path is faster because we don't need a separate traversal of the
 *       sorted_set to free it. This traversal is O(n).
 *
 *       it makes the fail path slower, though, because we need to put the keys
 *       back into the sorted set. This is O(n log(n)) if we do it the naive
 *       way, O(n) if we make a special API that allows it to add a bunch of
 *       pre-sorted keys at once. Theoretically, this could also let it ensure
 *       it's using the "perfect" version of itself where the node heights are
 *       distributed at exact multiples of 2.
 *
 *       update: this is now what we do, using a sorted_set_maker
 */
void name_set_compile(struct name_set * name_set) [[gnu::nonnull(1)]]
{
    /* TODO: is this the desired behavior? */
    if (name_set->hash) {
        hash_destroy(name_set->hash);
    }

    struct hash_inputs * hash_inputs = hash_inputs_create();

    hash_inputs_at_least(hash_inputs, sorted_set_size(name_set->uncompiled));
    sorted_set_apply_and_destroy(
            name_set->uncompiled, &add_to_hash_inputs, hash_inputs);

#if defined(HASH_SIMULATE_FAILURE)
    name_set->hash = NULL;
#else
    name_set->hash = hash_create(hash_inputs);
#endif /* HASH_SIMULATE_FAILURE */

    if (name_set->hash) {
        /* no need for _except_keys, the hash_inputs is empty */
        hash_inputs_destroy(hash_inputs);
        name_set->uncompiled = sorted_set_create();
    } else {
        /* the keys are still in the hash_inputs since making the hash
         * failed
         *
         * we need to put them back in a sorted_set
         */
        struct sorted_set_maker * sorted_set_maker =
            sorted_set_maker_create(hash_inputs_n_keys(hash_inputs));
        hash_inputs_apply_and_destroy(
                hash_inputs, &add_to_sorted_set_maker, sorted_set_maker);
        name_set->uncompiled = sorted_set_maker_finalize(sorted_set_maker);
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

/* call this function every name */
void name_set_apply(
        struct name_set * name_set,
        void (*fn)(
            const char * name,
            size_t length,
            struct name_data * data,
            void * ptr
        ),
        void * ptr
    ) [[gnu::nonnull(1, 2)]]
{
    if (name_set->hash) {
        hash_apply(
                name_set->hash,
                (void (*)(const char *, size_t, void *, void *))fn,
                ptr
            );
    }
    sorted_set_apply(
            name_set->uncompiled,
            (void (*)(const char *, size_t, void *, void *))fn,
            ptr
        );
}
