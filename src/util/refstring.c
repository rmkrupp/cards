/* File: src/util/refstring.c
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
#include "util/refstring.h"

#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "util/strdup.h"

struct refstring {
    char * string;
    long references;
};

[[nodiscard]] struct refstring * refstring_create(const char * string)
{
    struct refstring * refstring = malloc(sizeof(*refstring));
    *refstring = (struct refstring) {
        .string = util_strdup(string),
        .references = 1
    };
    return refstring;
}

[[nodiscard]] struct refstring * refstring_createf(
        const char * format, ...) [[gnu::format(printf, 1, 2)]]
{

    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);

    int n = vsnprintf(NULL, 0, format, args1) + 1;
    char * s = malloc(sizeof(*s) * n);
    vsnprintf(s, n, format, args2);

    struct refstring * refstring = malloc(sizeof(*refstring));
    *refstring = (struct refstring) {
        .string = s,
        .references = 1
    };

    va_end(args1);
    va_end(args2);
    return refstring;
}

[[nodiscard]] struct refstring * refstring_create_from_stringn(
        const char * string, size_t n)
{
    struct refstring * refstring = malloc(sizeof(*refstring));
    *refstring = (struct refstring) {
        .string = util_strndup(string, n),
        .references = 1
    };
    return refstring;
}

void refstring_destroy(struct refstring * refstring) [[gnu::nonnull(1)]]
{
    assert(refstring->references > 0);
    refstring->references--;
    if (refstring->references == 0) {
        free(refstring->string);
        free(refstring);
    }
}

const char * refstring_string(
        struct refstring * refstring) [[gnu::nonnull(1)]]
{
    assert(refstring->references > 0);
    return refstring->string;
}

[[nodiscard]] struct refstring * refstring_dup(
        struct refstring * refstring) [[gnu::nonnull(1)]]
{
    assert(refstring->references > 0);
    refstring->references++;
    return refstring;
}
