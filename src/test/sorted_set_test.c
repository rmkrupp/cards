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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util/sorted_set.h"

static void apply_check(
        const char * key, size_t length, void * data, void * ptr)
{
    (void)key;
    (void)length;
    (void)ptr;
    int p = 15;

    *(int *)data = p;
}

struct keys_out {
    char ** keys;
    size_t n_keys;
};

static void apply_and_destroy_check(
        char * key, size_t length, void * data, void * ptr)
{
    (void)length;
    (void)data;

    struct keys_out * keys_out = ptr;

    keys_out->keys = realloc(
            keys_out->keys,
            sizeof(*keys_out->keys) * (keys_out->n_keys + 1)
        );
    keys_out->keys[keys_out->n_keys] = key;
    keys_out->n_keys++;
}

static void destroy_only_keys_check(
        const char * key, size_t length, void * data, void * ptr)
{
    (void)length;
    (void)data;

    struct keys_out * keys_out = ptr;

    keys_out->keys = realloc(
            keys_out->keys,
            sizeof(*keys_out->keys) * (keys_out->n_keys + 1)
        );
    keys_out->keys[keys_out->n_keys] = (char *)key;
    keys_out->n_keys++;

}

int main(int argc, char ** argv)
{
    (void)argc;
    (void)argv;

    printf("Sanity check sorted_set..\n");

    size_t errors = 0;

    /* create a set */
    struct sorted_set * set = sorted_set_create();

    /* make sure sorted_set_size returns zero for an empty set */
    printf("Testing empty set sorted_set_size:\n");
    printf("Expected: 0\n");
    size_t size = sorted_set_size(set);
    printf("Result: %zu\n", size);

    if (size != 0) {
        errors++;
    }

    /* data to add below */
    int x = 1;
    int y = 2;
    int z = 3;

    /* ensure a new key is added by checking the return value */
    if (sorted_set_add_key(set, strdup("KEY"), 3, &x)
            != SORTED_SET_ADD_KEY_UNIQUE) {
        printf("Unexpected. sorted_set_add_key did not add a new key.\n");
        errors++;   
    }
    
    /* add more keys */
    sorted_set_add_key(set, strdup("LOL"), 3, &y);
    sorted_set_add_key(set, strdup("best"), 4, &z);

    printf("Testing sorted_set_size post adding three keys:\n");
    printf("Expected: 3\n");
    size = sorted_set_size(set);
    printf("Result: %zu\n", size);

    if (size != 3) {
        errors++;
    }

    /* ensure a duplicate key is not added to the set */
    char * s = strdup("LOL");
    if (sorted_set_add_key(set, s, 3, &y) != SORTED_SET_ADD_KEY_DUPLICATE) {
        printf("Unexpected.  A duplicate key was entered.");
        errors++;
    }
    free(s);

    /* check that the set size did not change */
    printf("Testing sorted_set_size post rejecting a duplicate:\n");
    printf("Expected: 3\n");
    size = sorted_set_size(set);
    printf("Result: %zu\n", size);

    if (size != 3) {
        errors++;
    }

    /* test a lookup */
    const struct sorted_set_lookup_result * result =
        sorted_set_lookup(set, "KEY", 3);

    printf("Checking sorted_set_lookup has the appropriate data:\n");
    printf("Expected: 1\n");
    printf("Result: %d\n", *(int *)result->data);

    if (*(int *)result->data != 1) {
        errors++;
    }

    /* check that the set size did not change */
    printf("Testing sorted_set_size post lookup:\n");
    printf("Expected: 3\n");
    size = sorted_set_size(set);
    printf("Result: %zu\n", size);

    if (size != 3) {
        errors++;
    }

    /* make sure sorted_set_apply applies the callback appropriately */
    sorted_set_apply(set, apply_check, NULL);

    const struct sorted_set_lookup_result * result2 =
        sorted_set_lookup(set,"KEY",3);
    printf("Checking key lookup 1 post sorted_set_apply:\n");
    printf("Expected: 15\n");
    printf("Result: %d\n", *(int *)result2->data);

    if (*(int *)result2->data != 15) {
        errors++;
    }

    const struct sorted_set_lookup_result * result3 =
        sorted_set_lookup(set,"LOL",3);
    printf("Checking key lookup 2 post sorted_set_apply:\n");
    printf("Expected: 15\n");
    printf("Result: %d\n", *(int *)result3->data);

    if (*(int *)result3->data != 15) {
        errors++;
    }

    const struct sorted_set_lookup_result * result4 =
        sorted_set_lookup(set,"best",4);
    printf("Checking key lookup 3 post sorted_set_apply:\n");
    printf("Expected: 15\n");
    printf("Result: %d\n", *(int *)result4->data);

    if (*(int *)result4->data != 15) {
        errors++;
    }

    /* test sorted_set_apply_and_destroy */
    struct keys_out keys_out = { };
    sorted_set_apply_and_destroy(set, apply_and_destroy_check, &keys_out);

    printf("Testing keys_out length after using sorted_set_apply_and_destroy\n");
    printf("Expected: 3\n");
    printf("Result: %lu\n", keys_out.n_keys);

    if (keys_out.n_keys != 3) {
        errors++;
    }
    
    printf("Testing if the keys were stored in the right order\n");
    printf("Expected:\nKEY\nLOL\nbest\n");
    printf("Result:\n");

    for (size_t i = 0; i < keys_out.n_keys; i++) {
        printf("%s\n", keys_out.keys[i]);
    }

    if (strcmp(keys_out.keys[0], "KEY") ||
            strcmp(keys_out.keys[1], "LOL") ||
            strcmp(keys_out.keys[2], "best")) {
        errors++;
    }

    size_t length = keys_out.n_keys;
    for (size_t i = 0; i < length; i++) {
        free(keys_out.keys[i]);
    }
    free(keys_out.keys);

    /* create a new set to test sorted_set_destroy_except_keys */
    struct sorted_set * set2 = sorted_set_create();

    sorted_set_add_key(set2, strdup("GG"), 2, &z);
    sorted_set_add_key(set2, strdup("green"), 5, &z);
    sorted_set_add_key(set2, strdup("best"), 4, &z);
    
    struct keys_out keys = { };
    sorted_set_apply(set2, destroy_only_keys_check, &keys);

    printf("Testing if keys were extracted right with sorted_set_apply\n");
    printf("Expected: GG\n");
    printf("Result: %s\n", keys.keys[0]);

    if(strcmp(keys.keys[0], "GG")) {
        errors++;
    }

    sorted_set_destroy_except_keys(set2);

    printf("Checking if keys still exist after destroying the sorted set\n");
    printf("Expected: GG\n");
    printf("Result: %s\n", keys.keys[0]);

    if (strcmp(keys.keys[0], "GG")) {
        errors++;
    }

    size_t index = keys.n_keys;
    for (size_t i = 0; i < index; i++) {
        free(keys.keys[i]);
    }
    free(keys.keys);

    /* done */
    printf("Done.\n");

    if (errors) {
        printf("%zu errors occurred\n", errors);
    } else {
        printf("No errors occurred\n");
    }

    return errors;
}
