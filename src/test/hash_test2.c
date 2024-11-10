/* File: test.hash_test2.c
 * Part of hash <github.com/rmkrupp/hash>
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

/* sainty check for hash library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util/strdup.h"

#include "hash.h"

/* transcluded from hash.c */
struct hash_input {
    char * key;
    size_t length;
    void * ptr;
};

struct hash_inputs {
    struct hash_input * inputs;
    size_t n_inputs;
    size_t capacity;
#ifdef HASH_STATISTICS
    struct hash_inputs_statistics statistics;
#endif /* HASH_STATISTICS */
};
/* ----------------------- */

struct apply_check_context {
    char ** keys;
    size_t n_keys;
};

static void apply_check(const char * key, size_t length, void * data, void * ptr)
{
    (void)data;

    struct apply_check_context * context = ptr;
    context->keys = realloc(context->keys, sizeof(*context->keys) * (context->n_keys + 1));
    context->keys[context->n_keys] = util_strndup(key, length);
    context->n_keys++;
}

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    printf("Begin test of hash...\n");
    size_t errors = 0;

    struct hash_inputs * hash_inputs1 = hash_inputs_create();

    printf("Capacity before hash_inputs_at_least\n");
    printf("Expected: 0\n");
    printf("Result: %zu\n", hash_inputs1->capacity);

    hash_inputs_at_least(hash_inputs1, 10);

    printf("Capacity after hash_inputs_at_least\n");
    printf("Expected: 10\n");
    printf("Result: %zu\n", hash_inputs1->capacity);

    if (hash_inputs1->capacity != 10) {
        errors++;
    }

    hash_inputs_add(hash_inputs1, "KEY", 3, NULL);
    hash_inputs_add(hash_inputs1, "bob", 3, NULL);
    hash_inputs_add(hash_inputs1, "good", 4, NULL);

    printf("Test of hash inputs length.\n");
    printf("Expected: 3\n");
    printf("Result: %zu\n", hash_inputs1->n_inputs);

    if (hash_inputs1->n_inputs != 3) {
        errors++;
    }

    printf("Capacity before hash_inputs_grow\n");
    printf("Expected: 10\n");
    printf("Result: %zu\n", hash_inputs1->capacity);

    hash_inputs_at_least(hash_inputs1,20);

    printf("Capacity after hash_inputs_grow\n");
    printf("Expected: 20\n");
    printf("Result: %zu\n", hash_inputs1->capacity);

    if (hash_inputs1->capacity != 20) {
        errors++;
    }

    printf("Test of hash_inputs_n_keys\n");
    printf("Expected: 3\n");
    size_t hash_inputs1_key_len = hash_inputs_n_keys(hash_inputs1);
    printf("Result: %zu\n", hash_inputs1_key_len);

    if (hash_inputs1_key_len != 3) {
        errors++;
    }

    struct hash * hash1 = hash_create(hash_inputs1);

    const struct hash_lookup_result * lookup_result = hash_lookup(hash1,"KEY",3);

    printf("Test hash_lookup\n");
    printf("Expected: KEY\n");
    printf("Result: %s\n", lookup_result->key);

    if (strcmp(lookup_result->key,"KEY")) {
        errors++;
    }

    const struct hash_lookup_result * lookup_result2 = hash_get_keys(hash1,NULL);

    size_t len = lookup_result2->length;

    printf("Test of length of hash_get_keys result\n");
    printf("Expected: 3\n");
    printf("Result: %zu\n", len);

    if (len != 3) {
        errors++;
    }

    struct hash_inputs * hash_inputs3 = hash_inputs_create();
    hash_inputs_add(hash_inputs3, "KEY", 3, NULL);
    hash_inputs_add(hash_inputs3, "bob", 3, NULL);
    hash_inputs_add(hash_inputs3, "good", 4, NULL);

    struct apply_check_context context = { };

    hash_inputs_apply(hash_inputs3, apply_check, &context);

    printf("checking hash_inputs_apply\n");
    printf("Expected: KEY bob good\n");
    printf("Result: %s %s %s\n", context.keys[0], context.keys[1], context.keys[2]);

    if (strcmp(context.keys[0], "KEY") ||
            strcmp(context.keys[1], "bob") ||
            strcmp(context.keys[2], "good")) {
        errors++;
    }

    for(size_t i = 0; i < context.n_keys; i++) {
        free(context.keys[i]);
    }
    free(context.keys);

    struct apply_check_context context2 = { };

    hash_apply(hash1, apply_check, &context2);

    printf("checking hash_inputs_apply\n");
    printf("Expected: KEY bob good\n");
    printf("Result: %s %s %s\n", context2.keys[0], context2.keys[1], context2.keys[2]);

    if (strcmp(context2.keys[0], "KEY") ||
            strcmp(context2.keys[1], "bob") ||
            strcmp(context2.keys[2], "good")) {
        errors++;
    }
    for(size_t i = 0; i < context2.n_keys; i++) {
        free(context2.keys[i]);
    }
    free(context2.keys);
    
    if (errors != 0) {
        printf("Test finished. There were errors. Number: %zu\n", errors);
    } else {
        printf("Test finished. There were no errors.\n");
    }
    hash_destroy(hash1);  
    hash_inputs_destroy(hash_inputs1);
    hash_inputs_destroy(hash_inputs3);
}
