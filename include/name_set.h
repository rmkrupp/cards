/* File: include/name_set.h
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
#ifndef NAME_SET_H
#define NAME_SET_H

#include <stddef.h>
#include <unitypes.h>

/* a set for looking up name tokens */
struct name_set;

/* the possible types a name can have */
enum name_type {
    NAME_TYPE_CARD,
    NAME_TYPE_ABILITY,
    NAME_TYPE_PLAYER
};

/* the result of a name lookup */
struct name {
    uint8_t * display_name;
    size_t display_name_length;
    enum name_type type;
    void * data;
};

/* create an empty name set */
[[nodiscard]] struct name_set * name_set_create();

/* destroy a name set and free the names its holding */
void name_set_destroy(struct name_set * name_set) [[gnu::nonnull(1)]];

/* add this name to this name set
 *
 * returns true if the key is added, false otherwise (because it was a
 * duplicate)
 */
bool name_set_add(
        struct name_set * name_set,
        const uint8_t * name,
        size_t length,
        void * data,
        enum name_type type
    ) [[gnu::nonnull(1, 2)]];

/* remove this name from this set
 *
 * will not remove it if the name_set has been compiled and it's in the hash
 */
/*
struct name_data * name_set_remove(
        struct name_set * name_set,
        const char * name,
        size_t length
    ) [[gnu::nonnull(1, 2)]];
*/

/* compile a name set (transforming its internal sorted_set into a hash)
 *
 * TODO: how this function works when called re: the names already
 *       in the set
 */
void name_set_compile(struct name_set * name_set) [[gnu::nonnull(1)]];

/* look up a name in this set */
const struct name * name_set_lookup(
        struct name_set * name_set,
        const uint8_t * name,
        size_t length
    ) [[gnu::nonnull(1, 2)]];

/* call this function on every name in this set, passing it ptr */
void name_set_apply(
        struct name_set * name_set,
        void (*fn)(
            struct name * name,
            void * ptr
        ),
        void * ptr
    ) [[gnu::nonnull(1, 2)]];

#endif /* NAME_SET_H */
