/* File: include/util/checksum.h
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
#ifndef UTIL_CHECKSUM_H
#define UTIL_CHECKSUM_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* calculate a checksum (a 32-character null terminated hex string) and return
 * it in freshly allocated memory
 *
 * this uses the MD5 algorithm
 */
[[nodiscard]] char * checksum_calculate(
        const uint8_t * data, size_t length) [[gnu::nonnull(1)]];

/* calculate a checksum and match it to the given one
 *
 * returns true if they match, false otherwise
 */
bool checksum_match(
        const char * checksum,
        uint8_t * data,
        size_t length
    ) [[gnu::nonnull(1, 2)]];

/* test if this string is a valid checksum (exactly 32 characters long plus
 * null terminator, each character in [0-9a-f])
 *
 * returns true if it's valid, false otherwise
 */
bool checksum_valid(const char * checksum) [[gnu::nonnull(1)]];

#endif /* UTIL_CHECKSUM_H */
