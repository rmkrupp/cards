/* File: src/test/sorted_set_test.c
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
#include "util/sorted_set.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void printer(const char * key, size_t length, void * data, void * ptr)
{
    (void)length;
    (void)data;
    (void)ptr;
    printf("%s\n", key);
}

int main(int argc, char ** argv)
{
    struct sorted_set * sorted_set = sorted_set_create();

    char buffer[64];
    buffer[63] = '\0';
    for (size_t i = 0; i < 100000; i++) {
        for (size_t j = 0; j < 63; j++) {
            buffer[j] = rand() % 26 + 'a';
        }
        sorted_set_add_key(sorted_set, buffer, 63, NULL);
    }

    printf("%s\n", sorted_set_add_key(sorted_set, "foo", 3, NULL) ? "dup" : "ok");
    printf("%s\n", sorted_set_add_key(sorted_set, "bar", 3, NULL) ? "dup" : "ok");
    printf("%s\n", sorted_set_add_key(sorted_set, "jimothy", 7, NULL) ? "dup" : "ok");
    printf("%s\n", sorted_set_add_key(sorted_set, "foo", 3, NULL) ? "dup" : "ok");

    for (int i = 1; i < argc; i++) {
        printf("%s\n", sorted_set_add_key(sorted_set, argv[i], strlen(argv[i]), NULL) ? "dup" : "okay");
    }

    //sorted_set_apply(sorted_set, &printer, NULL);

    sorted_set_destroy(sorted_set);
}
