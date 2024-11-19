/* File: src/util/checksum.c
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
#include "util/checksum.h"
#include "util/strdup.h"

#include <stdlib.h>
#include <string.h>

[[nodiscard]] char * checksum_calculate(
        uint8_t * data, size_t length) [[gnu::nonnull(1)]]
{
    (void)data;
    (void)length;
    return util_strdup("<checksum>");
}

bool checksum_match(
        const char * checksum,
        uint8_t * data,
        size_t length
    ) [[gnu::nonnull(1, 2)]]
{
    char * checksum2 = checksum_calculate(data, length);
    int result = strcmp(checksum, checksum2);
    free(checksum2);
    return result == 0;
}
