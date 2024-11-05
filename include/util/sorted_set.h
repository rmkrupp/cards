/* File: include/util/sorted_set.h
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
#ifndef UTIL_SORTED_SET_H
#define UTIL_SORTED_SET_H

/* the "user story" of this sorted set library:
 *
 *  - struct sorted_set * sorted_set = sorted_set_create();
 *  - add a bunch of keys with sorted_set_add_key(),
 *    performing some error action when the keys are not unique
 *  - transform the sorted_set into a hash (see hash.h)
 */

#include <stddef.h>

/* a sorted set */
struct sorted_set;

/* the result of a lookup on a set */
struct sorted_set_lookup_result {
    const char * key;
    size_t length;
    void * data;
};

/* create an empty sorted set */
[[nodiscard]] struct sorted_set * sorted_set_create();

/* destroy this sorted set */
void sorted_set_destroy(struct sorted_set * sorted_set) [[gnu::nonnull(1)]];

/* destroy this sorted set without free'ing the keys */
void sorted_set_destroy_except_keys(
        struct sorted_set * sorted_set) [[gnu::nonnull(1)]];

/* return the number of keys added to this set */
size_t sorted_set_size(struct sorted_set * sorted_set) [[gnu::nonnull(1)]];

/* the result of sorted_set_add_key() */
enum sorted_set_add_key_result {
    SORTED_SET_ADD_KEY_UNIQUE, /* the key was added because it was not already
                                * present
                                */
    SORTED_SET_ADD_KEY_DUPLICATE /* the key was not added because it was
                                  * already present
                                  */
};

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
    ) [[gnu::nonnull(1, 2)]];

/* apply this function to every key in sorted order
 *
 * the ptr passed to sorted_set_apply is passed to the callback as well
 */
void sorted_set_apply(
        struct sorted_set * sorted_set,
        void (*fn)(const char * key, size_t length, void * data, void * ptr),
        void * ptr
    ) [[gnu::nonnull(1, 2)]];

/* find this key in the sorted set and return a const pointer to it, or NULL
 * if it's not in the set
 */
const struct sorted_set_lookup_result * sorted_set_lookup(
        struct sorted_set * sorted_set,
        const char * key,
        size_t length
    ) [[gnu::nonnull(1, 2)]];

/* a sorted_set_maker
 *
 * this allows insertion of pre-sorted keys into a sorted_set in O(1) time
 * when the number of keys is known ahead of time
 */
struct sorted_set_maker;

/* create a sorted_set_maker that will make a sorted sorted with this number
 * of keys
 */
[[nodiscard]] struct sorted_set_maker * sorted_set_maker_create(
        size_t n_keys);

/* returns true if the number of keys added to this sorted_set_maker is equal
 * to the number of keys preallocated on its creation
 */
bool sorted_set_maker_complete(
        const struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]];

/* finalize this sorted_set_maker, destroying it and returning the sorted_set
 * that was made
 *
 * this must be called after a number of keys have been added to the maker
 * equal to the number that were preallocated.
 */
struct sorted_set * sorted_set_maker_finalize(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]];

/* destroy this sorted_set_maker and any partially-constructed set inside it,
 * and free any keys
 */
void sorted_set_maker_destroy(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]];

/* destroy this sorted_set_maker and any partially-constructed set inside it,
 * but do not free any keys
 */
void sorted_set_maker_destroy_except_keys(
        struct sorted_set_maker * sorted_set_maker) [[gnu::nonnull(1)]];

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
    ) [[gnu::nonnull(1, 2)]];

#endif /* UTIL_SORTED_SET_H */
