/* File: src/test/hash_test.c
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
#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    srand(time(NULL));

    struct hash_inputs * hash_inputs = hash_inputs_create();
    hash_inputs_add(hash_inputs, "key", 3, NULL);
    hash_inputs_add(hash_inputs, "ball", 4, NULL);
    hash_inputs_add(hash_inputs, "jetway", 6, NULL);
    hash_inputs_add(hash_inputs, "baron", 5, NULL);

    size_t length = 8;
    char * s = malloc(length + 1);
    s[length] = '\0';
    for (size_t i = 0; i < 100; i++) {
        for (size_t j = 0; j < length; j++) {
            s[j] = 'a' + rand() % 26;
        }
        hash_inputs_add_safe(hash_inputs, s, strlen(s), NULL);
    }
    free(s);

    //hash_inputs_apply(hash_inputs, &print_all);

    struct hash * hash = hash_create(hash_inputs);
    hash_inputs_destroy(hash_inputs);

    if (!hash) {
        printf("hash is null\n");
        return 1;
    }

    const struct hash_lookup_result * result1 = hash_lookup(hash, "key", 3);
    if (result1) {
        printf("%s\n", result1->key);
    } else {
        printf("hash_lookup(\"key\") is null\n");
    }

    const struct hash_lookup_result * result2 = hash_lookup(hash, "fireball", 8);
    if (result2) {
        printf("%s\n", result2->key);
    } else {
        printf("hash_lookup(\"fireball\") is null\n");
    }

    hash_destroy(hash);
}
