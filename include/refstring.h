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

struct refstring * refstring_create(const char * string);
struct refstring * refstring_createf(
        const char * string, ...);
struct refstring * refstring_create_from_stringn(
        const char * string, size_t n);
void refstring_destroy(struct refstring * refstring);
const char * refstring_string(struct refstring * refstring);
struct refstring * refstring_dup(struct refstring * refstring);

#endif /* REFSTRING_H */
