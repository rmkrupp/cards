/* File: include/refstring.h
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
#ifndef REFSTRING_H
#define REFSTRING_H

#include <stddef.h>

/* create a refstring with a copy of this string */
struct refstring * refstring_create(const char * string);

/* create a refstring from the result of this format string */
struct refstring * refstring_createf(
        const char * string, ...);

/* create a refstring using only n bytes of this string */
struct refstring * refstring_create_from_stringn(
        const char * string, size_t n);

/* destroy this refstring
 *
 * refstrings are reference counted (set to one on creation and increased by
 * calls to refstring dup) so this will only free the underlying memory if
 * it was the last reference
 */
void refstring_destroy(struct refstring * refstring);

/* get the string out of this refstring */
const char * refstring_string(struct refstring * refstring);

/* "duplicate" a refstring (this returns its argument, but with its reference
 * count increased by one
 */
struct refstring * refstring_dup(struct refstring * refstring);

#endif /* REFSTRING_H */
