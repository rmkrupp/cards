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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static constexpr unsigned int S[64] = {
    7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
    5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
    4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
    6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
};

static constexpr uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static inline uint32_t rotate_left (uint32_t n, unsigned int c)
{
    c &= 31;
    return (n << c) | (n >> ((-c) & 31));
}

[[nodiscard]] char * checksum_calculate(
        const uint8_t * data, uint64_t length) [[gnu::nonnull(1)]]
{
    uint32_t a0 = 0x67452301,
             b0 = 0xefcdab89,
             c0 = 0x98badcfe,
             d0 = 0x10325476;

    /* as many full 512-bit (64 byte) chunks as possible */
    size_t i;
    for (i = 0; (i + 64) <= length; i += 64) {
        uint32_t * m = (uint32_t *)&data[i];
        uint32_t a = a0,
                 b = b0,
                 c = c0,
                 d = d0;

        for (size_t j = 0; j < 16; j++) {
            uint32_t f = (b & c) | ((~b) & d);
            uint32_t g = j;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        for (size_t j = 16; j < 32; j++) {
            uint32_t f = (d & b) | ((~d) & c);
            uint32_t g = (5 * j + 1) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        for (size_t j = 32; j < 48; j++) {
            uint32_t f = b ^ c ^ d;
            uint32_t g = (3 * j + 5) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }
        
        for (size_t j = 48; j < 64; j++) {
            uint32_t f = c ^ (b | (~d));
            uint32_t g = (7 * j) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        a0 += a;
        b0 += b;
        c0 += c;
        d0 += d;
    }

    /* remaining data */
    uint64_t padding[16];
    uint8_t * padding_u8 = (uint8_t *)padding;
    size_t k = 0;

    if (i < length) {
        /* partial chunk */
        for (; i < length; i++, k++) {
            padding_u8[k] = data[i];
        }
    }

    /* append a 1 bit */
    padding_u8[k] |= 0x80;
    k++;

    /* pad out to a multiple of 64 bytes */
    if (k < 56) {
        for (; k < 56; k++) {
            padding_u8[k] = 0;
        }
    } else if (k > 56) {
        for (; k < 120; k++) {
            padding_u8[k] = 0;
        }
    }

    /* append length in bits */
    padding[k / 8] = length * 8;
    k += 8;

    /* one or two last loop iteration(s) */
    for (size_t i = 0; i < k; i += 64) {
        uint32_t * m = (uint32_t *)&padding[i];
        uint32_t a = a0,
                 b = b0,
                 c = c0,
                 d = d0;

        for (size_t j = 0; j < 16; j++) {
            uint32_t f = (b & c) | ((~b) & d);
            uint32_t g = j;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        for (size_t j = 16; j < 32; j++) {
            uint32_t f = (d & b) | ((~d) & c);
            uint32_t g = (5 * j + 1) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        for (size_t j = 32; j < 48; j++) {
            uint32_t f = b ^ c ^ d;
            uint32_t g = (3 * j + 5) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }
        
        for (size_t j = 48; j < 64; j++) {
            uint32_t f = c ^ (b | (~d));
            uint32_t g = (7 * j) % 16;
            f = f + a + K[j] + m[g];
            a = d;
            d = c;
            c = b;
            b = b + rotate_left(f, S[j]);
        }

        a0 += a;
        b0 += b;
        c0 += c;
        d0 += d;
    }

    /* extract the checksum */
    char * buffer = malloc(33);
    uint32_t result[] = { a0, b0, c0, d0 };
    uint8_t * bytes = (uint8_t *)result;
    for (size_t k = 0; k < 16; k++) {
        snprintf(&buffer[2 * k], 3, "%02x", bytes[k]);
    }
    buffer[32] = '\0';

    return buffer;
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

bool checksum_valid(const char * checksum) [[gnu::nonnull(1)]]
{
    size_t i;
    for (i = 0; checksum[i]; i++) {
        if (i > 31) {
            return false;
        }
        if (checksum[i] >= '0' && checksum[i] <= '9') {
            continue;
        }
        if (checksum[i] >= 'a' && checksum[i] <= 'f') {
            continue;
        }
        return false;
    }
    if (i != 32) {
        return false;
    }
    return true;
}
