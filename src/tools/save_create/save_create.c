/* File: src/tools/save_create/save_create.c
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
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>
#include <jansson.h>

#include "tools/save_create/args.h"
#include "util/checksum.h"
#include "util/sorted_set.h"

struct bundle_data {
    void * script;
    size_t script_length;
};

/* returns 0 on success, 1 on failure. if it failed, the db is closed */
static int add_metadata(sqlite3 * db, const char * key, const char * value)
{
    const char statement[] =
        "INSERT INTO metadata (key, value) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_reset(stmt)) {
        fprintf(
                stderr,
                "error resetting statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 1, key, strlen(key), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 2, value, strlen(value), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
           );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

/* returns 0 on success, 1 on failure. if it failed, the db is closed */
static int add_rule(sqlite3 * db, const char * key, const char * value)
{
    const char statement[] =
        "INSERT INTO rules (key, value) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_reset(stmt)) {
        fprintf(
                stderr,
                "error resetting statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 1, key, strlen(key), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 2, value, strlen(value), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
           );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

/* returns 0 on success, 1 on failure. if it failed, the db is closed */
static int add_command(
        sqlite3 * db, ssize_t player, const char * command)
{
    const char statement[] =
        "INSERT INTO log (player, command) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_reset(stmt)) {
        fprintf(
                stderr,
                "error resetting statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }


    if (sqlite3_bind_int64(stmt, 1, player)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }


    if (sqlite3_bind_text(stmt, 2, command, strlen(command), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
           );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}


/* returns 0 on success, 1 on failure. if it failed, the db is closed */
static int add_player(sqlite3 * db, ssize_t id, const char * name)
{
    const char statement[] =
        "INSERT INTO players (id, name) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_reset(stmt)) {
        fprintf(
                stderr,
                "error resetting statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_int64(stmt, 1, id)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 2, name, strlen(name), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
           );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;
}

/* returns 0 on success, 1 on failure. if it failed, the db is closed */
static int add_card(
        sqlite3 * db,
        const char * filename,
        const struct bundle_data * data
    )
{
    const char statement[] =
        "INSERT INTO cards (filename, script) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_reset(stmt)) {
        fprintf(
                stderr,
                "error resetting statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 1, filename, strlen(filename), NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_bind_text(stmt, 2, data->script, data->script_length, NULL)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    sqlite3_finalize(stmt);
    return 0;

}

/* callback for create_save and build_sorted_set to destroy keys/data */
static void destroy_callback(
        char * key, size_t length, void * data, void * ptr)
{
    (void)length;
    (void)ptr;

    struct bundle_data * bundle_data = data;
    free(key);
    free(bundle_data->script);
    free(bundle_data);
}

/* build a sorted set from these bundles, where the key field is made by
 * joining the filename property in the bundle with the checksum of the script,
 * separated by a space
 *
 * the data in the sorted set will be a pointed to a struct bundle_data object,
 * and must be free'd before the set is destroyed
 *
 * returns NULL on error, the sorted set otherwise
 */
static struct sorted_set * build_sorted_set(
        const char ** bundle_names, size_t n_bundles)
{
    struct sorted_set * sorted_set = sorted_set_create();

    for(size_t i = 0; i < n_bundles; i++) {
        sqlite3 * db;
        sqlite3_open(bundle_names[i], &db);

        const char statement[] = "SELECT filename, script from cards";
        sqlite3_stmt * stmt = NULL;
        if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
            fprintf(
                    stderr,
                    "error preparing statement: %s\n",
                    sqlite3_errmsg(db)
                   );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return NULL;
        }

        int result;
        while ((result = sqlite3_step(stmt)) != SQLITE_DONE) {
            if (result == SQLITE_ERROR) {
                fprintf(
                        stderr,
                        "error stepping statement: %s\n",
                        sqlite3_errmsg(db)
                    );
                sorted_set_apply_and_destroy(
                        sorted_set, &destroy_callback, NULL);
                sqlite3_finalize(stmt);
                sqlite3_close(db);
                return NULL;
            }

            const unsigned char * filename = sqlite3_column_text(stmt, 0);
            const void * script = sqlite3_column_blob(stmt, 1);
            int script_length = sqlite3_column_bytes(stmt, 1);

            char * checksum = checksum_calculate(script, script_length);

            struct bundle_data * bundle_data = malloc(sizeof(*bundle_data));

            bundle_data->script = malloc(script_length + 1);
            memcpy(bundle_data->script, script, script_length);
            ((char *)bundle_data->script)[script_length] = '\0';

            char * buffer = NULL;
            size_t n = snprintf(buffer, 0, "%s %s", filename, checksum);
            buffer = malloc(n + 1);
            snprintf(buffer, n + 1, "%s %s", filename, checksum);
            free(checksum);

            enum sorted_set_add_key_result result = sorted_set_add_key(
                    sorted_set, buffer, n, (void *)bundle_data);

            if (result == SORTED_SET_ADD_KEY_DUPLICATE) {
                fprintf(
                        stderr,
                        "warning: bundle '%s' contains duplicate key '%s' from earlier bundle\n",
                        bundle_names[i],
                        filename
                    );
                free(buffer);
                free(bundle_data->script);
                free(bundle_data);
            }
        }
        sqlite3_finalize(stmt);
        sqlite3_close(db);

    }
    return sorted_set;
}

/* lookup this card in the sorted_set created from all the bundles
 *
 * returns the a reference to the associated struct bundle_data (i.e. the
 * script and it's length) on success, NULL otherwise
 */
static const struct bundle_data * check_card_against_sorted_set(
        struct sorted_set * sorted_set,
        const char * filename,
        const char * checksum
    )
{
    if (!checksum_valid(checksum)) {
        fprintf(
                stderr,
                "card '%s' has a malformed checksum '%s'\n",
                filename,
                checksum
            );
        return NULL;
    }

    char * buffer = NULL;
    size_t n = snprintf(buffer, 0, "%s %s", filename, checksum);
    buffer = malloc(n + 1);
    snprintf(buffer, n + 1, "%s %s", filename, checksum);

    const struct sorted_set_lookup_result * result = sorted_set_lookup(
            sorted_set, buffer, n);

    if (result == NULL) {
        fprintf(
                stderr,
                "card '%s' (checksum %s) does not match any cards in the provided bundles\n",
                filename,
                checksum
            );
        free(buffer);
        return NULL;
    } else {
        free(buffer);
        return (struct bundle_data *)result->data;
    }

}
/* create and populate a save database file from this JSON manifest and list
 * of bundles
 *
 * returns 0 on success, 1 otherwise
 */
static int create_save(
        sqlite3 * db,
        const char * json_filename,
        const char ** bundle_filenames,
        size_t n_bundles
    )
{
    json_error_t error;

    json_t * root = json_load_file(json_filename, 0, &error);

    if (!root) {
        fprintf(
                stderr,
                "syntax error in JSON file '%s' (line %d): error %s\n",
                json_filename,
                error.line,
                error.text
            );
        return 1;
    }

    if(!json_is_object(root)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': root must be an object\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    /* parse the rules array */
    json_t * rules = json_object_get(root, "rules");

    if (!json_is_array(rules)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': 'rules' field must contain an array\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    size_t length = json_array_size(rules);

    for (size_t i = 0; i < length; i++) {
        json_t * obj = json_array_get(rules, i);

        if (!json_is_object(obj)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': 'rules' array must contain only objects\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * key = json_object_get(obj, "key");
        json_t * value = json_object_get(obj, "value");

        if (!json_is_string(key) || !json_is_string(value)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': the 'key' and 'value' fields of rule objects must contain strings\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        if (json_object_size(obj) != 2) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': rule objects must not have fields beyond 'key' and 'value'\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        const char * key_value = json_string_value(key);
        const char * value_value = json_string_value(value);
        add_rule(db, key_value, value_value);
    }

    /* parse the players array */
    json_t * players = json_object_get(root, "players");

    if (!json_is_array(players)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': 'players' field must contain an array\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    size_t n_players = json_array_size(players);

    if (n_players > 0 && json_integer_value(
                json_object_get(json_array_get(players, 0), "id")) != 0) {
        fprintf(
                stderr,
                "malformed JSON file '%s': the ID of the first player must be 0\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    for (size_t i = 0; i < n_players; i++) {
        json_t * obj = json_array_get(players, i);

        if (!json_is_object(obj)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': 'players' array must contain only objects\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * id = json_object_get(obj, "id");

        if (!json_is_integer(id)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': 'id' field of player objects must be an integer\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        ssize_t id_value = (ssize_t)json_integer_value(id);
        if (id_value != (ssize_t)i) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': each player's ID must match its position in the array\n",
                    json_filename
                );
            json_decref(root);
            return -1;
        }

        json_t * name = json_object_get(obj, "name");

        if (!json_is_string(name)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': 'name' field of player objects must be a string\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        if (json_object_size(obj) != 2) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': player objects must not have fields beyond 'id' and 'name'\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        const char * name_value = json_string_value(name);
        add_player(db, id_value, name_value);
    }

    /* parse the metadata array */
    json_t * metadata = json_object_get(root, "metadata");

    if (!json_is_array(metadata)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': metadata field must contain an array\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    length = json_array_size(metadata);

    for (size_t i = 0; i < length; i++) {
        json_t * obj = json_array_get(metadata, i);

        if (!json_is_object(obj)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': 'metadata' array must contain only objects\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * key = json_object_get(obj, "key");
        json_t * value = json_object_get(obj, "value");

        if (!json_is_string(key) || !json_is_string(value)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': the 'key' and 'value' fields of metadata objects must contain strings\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }
        const char * key_value = json_string_value(key);
        const char * value_value = json_string_value(value);
        add_metadata(db, key_value, value_value);
    }

    /* parse the command log */
    json_t * log = json_object_get(root, "log");

    if (!json_is_array(log)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': log field must contain an array\n",
                json_filename
            );
        json_decref(root);
        return 1;
    }

    length = json_array_size(log);

    for (size_t i = 0; i < length; i++) {
        json_t * obj = json_array_get(log, i);

        if (!json_is_object(obj)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': log array must contain only objects\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * player_id = json_object_get(obj, "player_id");

        if (!json_is_integer(player_id)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': player_id field of log objects must be an integer\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        ssize_t player_id_value  = (ssize_t)json_integer_value(player_id);

        if (player_id_value < 0 || player_id_value >= (ssize_t)n_players) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': player_id field of log objects must match the ID of a player in the players array\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * command = json_object_get(obj, "command");

        if (!json_is_string(command)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': command field of log objects must be a string\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        const char * command_value = json_string_value(command);
        add_command(db, player_id_value, command_value);
    }

    json_t * cards = json_object_get(root, "cards");

    if (!json_is_array(cards)) {
        fprintf(
                stderr,
                "malformed JSON file '%s': cards field must contain an array\n",
                json_filename
               );
        json_decref(root);
        return 1;
    }

    /* construct a set out of every card/script (with checksums) in the list
     * of bundles
     */
    struct sorted_set * set =
        build_sorted_set(bundle_filenames, n_bundles);

    length = json_array_size(cards);

    /* check every card against the bundle set for its associated script data,
     * matching both by original filename and by checksum
     */
    size_t missing_cards = 0;
    for (size_t i = 0; i < length; i++) {
        json_t * obj = json_array_get(cards, i);

        if (!json_is_object(obj)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': cards array must contain only objects\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        json_t * filename = json_object_get(obj, "filename");
        json_t * checksum = json_object_get(obj, "checksum");

        if (!json_is_string(filename) || !json_is_string(checksum)) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': the 'filename' and 'checksum' fields of cards objects must contain strings\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        if (json_object_size(obj) != 2) {
            fprintf(
                    stderr,
                    "malformed JSON file '%s': cards objects must not have fields beyond 'filename' and 'checksum'\n",
                    json_filename
                );
            json_decref(root);
            return 1;
        }

        const char * filename_value  = json_string_value(filename);
        const char * checksum_value = json_string_value(checksum);

        const struct bundle_data * result =
            check_card_against_sorted_set(set, filename_value, checksum_value);

        if (result) {
            add_card(db, filename_value, result);
        } else {
            /* an error has already been displayed */
            missing_cards++;
        }
    }

    sorted_set_apply_and_destroy(set, &destroy_callback, NULL);
    json_decref(root);

    if (missing_cards > 0) {
        return 1;
    }

    return 0;
}

static void free_args(struct arguments * args)
{
    free(args->database_name);
    free(args->json_name);
    for (size_t i = 0; i < args->n_filenames; i++) {
        free(args->filenames[i]);
    }
    free(args->filenames);
}


int main(int argc, char ** argv)
{
    struct arguments args = { };

    int parse_result;
    if ((parse_result = parse_args(&args, argc, argv))) {
        free_args(&args);
        return parse_result;
    }

    char * errmsg = NULL;
    sqlite3 * db;
    if (sqlite3_open(args.database_name, &db)) {
        fprintf(
                stderr,
                "error opening database '%s': %s\n",
                args.database_name,
                sqlite3_errmsg(db)
            );
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "DROP TABLE IF EXISTS metadata",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "DROP TABLE IF EXISTS log",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "DROP TABLE IF EXISTS rules",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "DROP TABLE IF EXISTS players",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "DROP TABLE IF EXISTS cards",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS metadata (key, value)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS rules (key, value)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }


    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS log (player, command)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }


    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS players (id, name)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS cards "
                "(filename, checksum, script)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    int result = create_save(
            db,
            args.json_name,
            (const char **)args.filenames,
            args.n_filenames
        );

    sqlite3_close(db);
    free_args(&args);
    return result;
}
