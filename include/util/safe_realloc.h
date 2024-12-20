/* File: src/util/safe_realloc.h
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
#ifndef UTIL_SAFE_REALLOC_H
#define UTIL_SAFE_REALLOC_H

#include <stdlib.h>

/* like realloc but free's the original ptr if the call to realloc returns NULL
 */
static inline void * safe_realloc(void * ptr, size_t size)
{
    void * new_ptr = realloc(ptr, size);
    if (!new_ptr) {
        free(ptr);
    }
    return new_ptr;
}

#endif /* UTIL_SAFE_REALLOC_H */
