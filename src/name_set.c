/* File: src/name_set.c
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
#include "name_set.h"

#include <stdlib.h>

#include "util/sorted_set.h"
#include "hash.h"
#include "card.h"

#include <uninorm.h>
#include <unicase.h>

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

void destroyer(const char * key, size_t length, void * data, void * ptr)
{
    (void)key;
    (void)length;
    (void)ptr;
    struct name * name = data;
    switch (name->type) {
        case NAME_TYPE_CARD:
            card_destroy(name->data);
            break;
        case NAME_TYPE_ABILITY:
            ability_destroy(name->data);
            break;
        case NAME_TYPE_SUBTYPE:
            subtype_destroy(name->data);
            break;
        case NAME_TYPE_PLAYER:
            break;
    }
    free(name->display_name);
    free(name);
}

/* destroy a name set and free the names its holding */
void name_set_destroy(struct name_set * name_set) [[gnu::nonnull(1)]]
{
    if (name_set->hash) {
        hash_apply(name_set->hash, &destroyer, NULL);
        hash_destroy(name_set->hash);
    }
    sorted_set_apply(name_set->uncompiled, &destroyer, NULL);
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
        const uint8_t * key,
        size_t length,
        void * data,
        enum name_type type
    ) [[gnu::nonnull(1, 2)]]
{
    /* first, transform */
    size_t size_out_transform = 0;
    uint8_t * buffer_out_transform = u8_tolower(
            key,
            length,
            uc_locale_language(),
            NULL,
            NULL,
            &size_out_transform
        );

    /* then, normalize/prepare for collation */
    size_t size_out = 0;

    char * buffer_out = u8_normxfrm(
            buffer_out_transform,
            size_out_transform,
            UNINORM_NFC,
            NULL,
            &size_out
        );

    struct name * name = malloc(sizeof(*name));
    *name = (struct name) {
        .display_name = buffer_out_transform,
        .display_name_length = size_out_transform,
        .type = type,
        .data = data
    };

    enum sorted_set_add_key_result result = sorted_set_add_key(
            name_set->uncompiled,
            buffer_out,
            size_out,
            name
        );

    if (result != SORTED_SET_ADD_KEY_UNIQUE) {
        free(buffer_out);
        free(name->display_name);
        free(name);
        return false;
    }
    return true;
}

/* remove this name from this set
 *
 * will not remove it if the name_set has been compiled and it's in the hash
 */
/*
struct name_data * name_set_remove(
        struct name_set * name_set,
        const char * name,
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    return sorted_set_remove_key(name_set->uncompiled, name, length);
}
*/

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
struct name * name_set_lookup(
        struct name_set * name_set,
        const uint8_t * key, 
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    /* first, transform */
    constexpr size_t size_buffer = 1024;
    size_t size_out_transform = size_buffer;
    static uint8_t transform_buffer[size_buffer];
    uint8_t * buffer_out_transform;

    buffer_out_transform = u8_tolower(
            key,
            length,
            uc_locale_language(),
            NULL,
            transform_buffer,
            &size_out_transform
        );

    /* then, normalize/prepare for collation */
    size_t size_out = size_buffer;
    static char normxfrm_buffer[size_buffer];

    char * buffer_out = u8_normxfrm(
            buffer_out_transform,
            size_out_transform,
            UNINORM_NFC,
            normxfrm_buffer,
            &size_out
        );

    if (buffer_out_transform != transform_buffer) {
        free(buffer_out_transform);
    }

    if (name_set->hash) {
        const struct hash_lookup_result * hash_result =
            hash_lookup(name_set->hash, buffer_out, size_out);

        if (hash_result) {
            if (buffer_out != normxfrm_buffer) {
                free(buffer_out);
            }
            return hash_result->ptr;
        }
    }

    const struct sorted_set_lookup_result * sorted_set_result =
        sorted_set_lookup(name_set->uncompiled, buffer_out, size_out);

    if (buffer_out != normxfrm_buffer) {
        free(buffer_out);
    }

    if (sorted_set_result) {
        return sorted_set_result->data;
    }

    return NULL;
}

/* used internally by name_set_apply */
struct name_set_apply_context {
    void (*fn)(struct name * name, void * ptr);
    void * ptr;
};

/* used internally by name_set_apply */
static void name_set_apply_helper(
        const char * key,
        size_t length,
        void * data,
        void * ptr
    )
{
    (void)key;
    (void)length;
    struct name_set_apply_context * context = ptr;
    struct name * name = data;
    context->fn(name, ptr);
}

/* call this function every name */
void name_set_apply(
        struct name_set * name_set,
        void (*fn)(struct name * name, void * ptr),
        void * ptr
    ) [[gnu::nonnull(1, 2)]]
{
    if (name_set->hash) {
        hash_apply(
                name_set->hash,
                &name_set_apply_helper,
                &(struct name_set_apply_context){ .fn = fn, .ptr = ptr, }
            );
    }
    sorted_set_apply(
            name_set->uncompiled,
            &name_set_apply_helper,
            &(struct name_set_apply_context){ .fn = fn, .ptr = ptr }
        );
}
