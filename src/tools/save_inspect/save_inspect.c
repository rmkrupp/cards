/* File: src/tools/save_inspect/save_inspect.c
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

#include "tools/save_inspect/args.h"
#include "util/strdup.h"
#include "util/checksum.h"
#include "util/sorted_set.h"

/* display the metadata table */
static int print_metadata(sqlite3 * db, struct arguments * args)
{
    const char statement[] = "SELECT * FROM metadata";

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

    int result;
    while((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        const unsigned char * key = sqlite3_column_text(stmt, 0);
        const unsigned char * value = sqlite3_column_text(stmt, 1);

        if (!args->key) {
            printf("%s%s%s\n", key, args->sep, value);
        } else if (args->key && strcmp(args->key, (char *)key) == 0) {
            printf("%s\n", value);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

/* display the rules table */
static int print_rules(sqlite3 * db, struct arguments * args)
{
    const char statement[] = "SELECT * FROM rules";

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

    int result;
    while((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        const unsigned char * key = sqlite3_column_text(stmt, 0);
        const unsigned char * value = sqlite3_column_text(stmt, 1);

        if (!args->key) {
            printf("%s%s%s\n", key, args->sep, value);
        } else if (args->key && strcmp(args->key, (char *)key) == 0) {
            printf("%s\n", value);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

/* display the players table */
static int print_players(sqlite3 * db, struct arguments * args)
{
    const char statement[] = "SELECT * FROM players";

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

    int result;
    while((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        int id = sqlite3_column_int(stmt, 0);
        const unsigned char * name = sqlite3_column_text(stmt, 1);
        printf("%d%s%s\n", id, args->sep, name);
    }

    sqlite3_finalize(stmt);

    return 0;
}

/* print the command log table */
static int print_log(sqlite3 * db, struct arguments * args)
{
    const char statement[] = "SELECT * FROM log";

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

    int result;
    while ((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
                fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
        const unsigned char * player = sqlite3_column_text(stmt, 0);
        const unsigned char * command = sqlite3_column_text(stmt, 1);

        if (!args->id) {
            printf("%s%s%s\n", player, args->sep, command);
        } else if (args->id && strcmp(args->id, (char *)player) == 0) {
            printf("%s\n", command);
        }
    }

    sqlite3_finalize(stmt);

    return 0;
}

/* print the cards table (and checksums for the scripts) */
static int print_cards(sqlite3 * db, struct arguments * args)
{
    const char statement[] = "SELECT * FROM cards";

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

    int result;
    while((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        const unsigned char * filename = sqlite3_column_text(stmt, 0);
        const unsigned char * script = sqlite3_column_text(stmt, 1);
        char * checksum = checksum_calculate(script, strlen((char *)script));

        if (!args->checksum && !args->filename) {
            printf("%s%s%s\n", filename, args->sep, checksum);
        } else if (args->checksum && args->filename &&
                strcmp(args->checksum, checksum) == 0 &&
                strcmp(args->filename, (char *)filename) == 0) {
            printf("%s%s%s\n", filename, args->sep, checksum);
        } else if (args->checksum && !args->filename &&
                strcmp(args->checksum, checksum) == 0) {
            printf("%s%s%s\n", filename, args->sep, checksum);
        } else if (!args->checksum && args->filename &&
                strcmp(args->filename, (char *)filename) == 0) {
            printf("%s%s%s\n", filename, args->sep, checksum);
        }

        free(checksum);
    }

    sqlite3_finalize(stmt);

    return 0;
}

/* validate a number of things about the table:
 *  - the player IDs must be contiguous
 *  - the first player ID must be zero
 *  - every player_id in the command log must be in the players table
 */
static int validate_db(sqlite3 * db) {
    const char statement[] = "SELECT id from players";
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

    size_t player_num = 0;

    int result2;
    while((result2 = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result2 == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        int id = sqlite3_column_int(stmt, 0);
        if (player_num != (size_t)id) {
            fprintf(
                    stderr,
                    "error validating, player id %d is not valid\n",
                    id
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }
        player_num++;
    }

    sqlite3_finalize(stmt);

    const char statement2[] = "SELECT player from log";

    sqlite3_stmt * stmt2 = NULL;

    if (sqlite3_prepare_v2(db, statement2, sizeof(statement2), &stmt2, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt2);
        sqlite3_close(db);
        return 1;
    }

    int result3;
    while((result3 = sqlite3_step(stmt2)) != SQLITE_DONE) {
        if (result3 == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt2);
            sqlite3_close(db);
            return 1;
        }

        int player = sqlite3_column_int(stmt2, 0);
        if (player < 0 || (size_t)player > player_num) {
            fprintf(
                    stderr,
                    "error validating, player %d is not a vaild player id\n",
                    player
                );
            sqlite3_finalize(stmt2);
            sqlite3_close(db);
            return 1;
        }
    }

    sqlite3_finalize(stmt2);

    const char statement3[] = "SELECT filename, script from cards";

    sqlite3_stmt * stmt3 = NULL;

    if (sqlite3_prepare_v2(db, statement3, sizeof(statement3), &stmt3, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt3);
        sqlite3_close(db);
        return 1;
    }

    struct sorted_set * sorted_set = sorted_set_create();

    int result4;
    while((result4 = sqlite3_step(stmt3)) != SQLITE_DONE) {
        if (result4 == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sorted_set_destroy(sorted_set);
            sqlite3_finalize(stmt3);
            sqlite3_close(db);
            return 1;
        }
        const unsigned char * filename = sqlite3_column_text(stmt3, 0);
        const unsigned char * script = sqlite3_column_text(stmt3, 1);
        int script_length = sqlite3_column_bytes(stmt3, 1);

        char * checksum = checksum_calculate(script, script_length);

        char * buffer = NULL;
        size_t n = snprintf(buffer, 0, "%s %s", filename, checksum);
        buffer = malloc(n + 1);
        snprintf(buffer, n + 1, "%s %s", filename, checksum);
        free(checksum);

        enum sorted_set_add_key_result result = sorted_set_add_key(
                sorted_set, buffer, n, NULL);

        if (result == SORTED_SET_ADD_KEY_DUPLICATE) {
            fprintf(
                    stderr,
                    "warning: cards table contains duplicate filename/checksum pair filename: %s\n",
                    filename
               );
            free(buffer);
        }
    }

    sorted_set_destroy(sorted_set);

    sqlite3_finalize(stmt3);

    return 0;

}

/* extract the bundle from the save as its own bundle file "filename" */
static int extract_bundle(sqlite3 * db, const char * filename) {

    sqlite3 * db_out;
    if (sqlite3_open(filename, &db_out)) {
        fprintf(
                stderr,
                "error opening database \"%s\": %s\n",
                filename,
                sqlite3_errmsg(db_out)
            );
        sqlite3_close(db_out);
        sqlite3_close(db);
        return 1;
    }

    char * errmsg = NULL;
    if (sqlite3_exec(
                db_out,
                "DROP TABLE IF EXISTS cards",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error dropping table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db_out);
        sqlite3_close(db);
        return 1;
    }

    if (sqlite3_exec(
                db_out,
                "CREATE TABLE IF NOT EXISTS cards (filename, script)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        sqlite3_close(db_out);
        return 1;
    }

    const char statement_out[] =
        "INSERT INTO cards (filename, script) VALUES (?, ?)";

    sqlite3_stmt * stmt_out = NULL;

    if (sqlite3_prepare_v2(db_out, statement_out, sizeof(statement_out),
                &stmt_out, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db_out)
            );
        sqlite3_finalize(stmt_out);
        sqlite3_close(db);
        sqlite3_close(db_out);
        return 1;
    }

    const char statement[] = "SELECT * FROM cards";

    sqlite3_stmt * stmt = NULL;

    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_finalize(stmt_out);
        sqlite3_close(db);
        sqlite3_close(db_out);
        return 1;
    }

    int result;
    while((result = sqlite3_step(stmt)) != SQLITE_DONE) {
        if (result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            sqlite3_close(db_out);
            return 1;
        }

        const unsigned char * filename = sqlite3_column_text(stmt, 0);
        const unsigned char * script = sqlite3_column_text(stmt, 1);

        if (sqlite3_reset(stmt_out)) {
            fprintf(
                    stderr,
                    "error resetting statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_out);
            sqlite3_close(db);
            sqlite3_close(db_out);
            return 1;
        }

        if (sqlite3_bind_text(stmt_out, 1, (char *)filename,
                    strlen((char *)filename), NULL)) {
            fprintf(
                    stderr,
                    "error binding statement: %s\n",
                    sqlite3_errmsg(db)
               );
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_out);
            sqlite3_close(db);
            sqlite3_close(db_out);
            return 1;
        }

        if (sqlite3_bind_text(stmt_out, 2, (char *)script,
                    strlen((char *)script), NULL)) {
            fprintf(
                    stderr,
                    "error binding statement: %s\n",
                    sqlite3_errmsg(db)
               );
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_out);
            sqlite3_close(db);
            sqlite3_close(db_out);
            return 1;
        }

        if (sqlite3_step(stmt_out) != SQLITE_DONE) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
               );
            sqlite3_finalize(stmt);
            sqlite3_finalize(stmt_out);
            sqlite3_close(db);
            sqlite3_close(db_out);
            return 1;
        }

    }

    sqlite3_finalize(stmt);
    sqlite3_finalize(stmt_out);
    sqlite3_close(db_out);

    return 0;
}

/* extract a json representation of this database and put it in a new JSON
 * file "filename"
 */
static int extract_json(sqlite3 * db, const char * filename)
{
    json_t * root = json_object();

    /* extract the metadata table */
    json_t * metadata = json_array();

    json_object_set_new(root, "metadata", metadata);

    const char m_statement[] = "SELECT * from metadata";

    sqlite3_stmt * m_stmt = NULL;

    if (sqlite3_prepare_v2(db, m_statement, sizeof(m_statement), &m_stmt,
                NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(m_stmt);
        sqlite3_close(db);
        return 1;
    }

    int m_result;
    while((m_result = sqlite3_step(m_stmt)) != SQLITE_DONE) {
        if (m_result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(m_stmt);
            sqlite3_close(db);
            return 1;
        }

        const char * key = (char *)sqlite3_column_text(m_stmt, 0);
        const char * value = (char *)sqlite3_column_text(m_stmt, 1);

        json_t * key_string = json_string(key);
        json_t * value_string = json_string(value);

        json_t * object = json_object();
        json_object_set_new(object, "key", key_string);
        json_object_set_new(object, "value", value_string);

        json_array_append_new(metadata, object);
    }

    sqlite3_finalize(m_stmt);

    /* extract the rules table */
    json_t * rules = json_array();

    json_object_set_new(root, "rules", rules);

    const char r_statement[] = "SELECT * from rules";

    sqlite3_stmt * r_stmt = NULL;

    if (sqlite3_prepare_v2(db, r_statement, sizeof(r_statement), &r_stmt,
                NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(r_stmt);
        sqlite3_close(db);
        return 1;
    }

    int r_result;
    while((r_result = sqlite3_step(r_stmt)) != SQLITE_DONE) {
        if (r_result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(r_stmt);
            sqlite3_close(db);
            return 1;
        }

        const char * key = (char *)sqlite3_column_text(r_stmt, 0);
        const char * value = (char *)sqlite3_column_text(r_stmt, 1);

        json_t * key_string = json_string(key);
        json_t * value_string = json_string(value);

        json_t * object = json_object();
        json_object_set_new(object, "key", key_string);
        json_object_set_new(object, "value", value_string);

        json_array_append_new(rules, object);
    }

    sqlite3_finalize(r_stmt);

    /* extract the players table */
    json_t * players = json_array();

    json_object_set_new(root, "players", players);

    const char p_statement[] = "SELECT * from players";

    sqlite3_stmt * p_stmt = NULL;

    if (sqlite3_prepare_v2(db, p_statement, sizeof(p_statement), &p_stmt,
                NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(p_stmt);
        sqlite3_close(db);
        return 1;
    }

    int p_result;
    while((p_result = sqlite3_step(p_stmt)) != SQLITE_DONE) {
        if (p_result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(p_stmt);
            sqlite3_close(db);
            return 1;
        }

        int id = sqlite3_column_int64(p_stmt, 0);
        const char * name = (char *)sqlite3_column_text(p_stmt, 1);

        json_t * id_integer = json_integer(id);
        json_t * name_string = json_string(name);

        json_t * object = json_object();
        json_object_set_new(object, "id", id_integer);
        json_object_set_new(object, "name", name_string);

        json_array_append_new(players, object);
    }

    sqlite3_finalize(p_stmt);

    /* extract the log table */
    json_t * log = json_array();

    json_object_set_new(root, "log", log);

    const char l_statement[] = "SELECT * from log";

    sqlite3_stmt * l_stmt = NULL;

    if (sqlite3_prepare_v2(db, l_statement, sizeof(l_statement), &l_stmt,
                NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(l_stmt);
        sqlite3_close(db);
        return 1;
    }

    int l_result;
    while((l_result = sqlite3_step(l_stmt)) != SQLITE_DONE) {
        if (l_result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(l_stmt);
            sqlite3_close(db);
            return 1;
        }

        int player = sqlite3_column_int64(l_stmt, 0);
        const char * command = (char *)sqlite3_column_text(l_stmt, 1);

        json_t * id_integer = json_integer(player);
        json_t * command_string = json_string(command);

        json_t * object = json_object();
        json_object_set_new(object, "player", id_integer);
        json_object_set_new(object, "command", command_string);

        json_array_append_new(log, object);
    }

    sqlite3_finalize(l_stmt);

    json_t * cards = json_array();

    json_object_set_new(root, "cards", cards);

    const char c_statement[] = "SELECT * from cards";

    sqlite3_stmt * c_stmt = NULL;

    if (sqlite3_prepare_v2(db, c_statement, sizeof(c_statement), &c_stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(c_stmt);
        sqlite3_close(db);
        return 1;
    }

    int c_result;
    while((c_result = sqlite3_step(c_stmt)) != SQLITE_DONE) {
        if (c_result == SQLITE_ERROR) {
            fprintf(
                    stderr,
                    "error stepping statement: %s\n",
                    sqlite3_errmsg(db)
                );
            sqlite3_finalize(c_stmt);
            sqlite3_close(db);
            return 1;
        }

        const char * filename = (char *)sqlite3_column_text(c_stmt, 0);
        const unsigned char * script = sqlite3_column_text(c_stmt, 1);
        int script_length = sqlite3_column_bytes(c_stmt, 1);

        char * checksum = checksum_calculate(script, script_length);

        json_t * filename_string = json_string(filename);
        json_t * checksum_string = json_string(checksum);

        json_t * object = json_object();
        json_object_set_new(object, "filename", filename_string);
        json_object_set_new(object, "checksum", checksum_string);

        json_array_append_new(cards, object);

        free(checksum);
    }

    sqlite3_finalize(c_stmt);

    json_dump_file(root, filename, JSON_INDENT(4));

    json_decref(root);

    return 0;
}


static void free_args(struct arguments * args)
{
    free(args->database_name);
    free(args->key);
    free(args->id);
    free(args->sep);
    free(args->checksum);
    free(args->filename);
    free(args->bundle_name);
    free(args->json_file);
}

int main(int argc, char ** argv)
{
    struct arguments args = {
        .sep = util_strdup(" ")
    };

    int parse_result;
    if ((parse_result = parse_args(&args, argc, argv))) {
        free_args(&args);
        return parse_result;
    }

    /* error checking for flags */
    if (args.checksum && !args.want_cards) {
        fprintf(
                stderr,
                "invalid entry, checksum option must be used with the cards table\n"
            );
        free_args(&args);
        return 1;
    }

    if (args.filename && !args.want_cards) {
        fprintf(
                stderr,
                "invalid entry, filename option must be used with the cards table\n"
            );
        free_args(&args);
        return 1;
    }

    if (args.id && !args.want_players) {
        fprintf(
                stderr,
                "invalid entry, player option must be used with the players table\n"
            );
        free_args(&args);
        return 1;
    }

    if (args.key && args.want_metadata && args.want_rules) {
        fprintf(
                stderr,
                "invalid entry, key option must be used with only one of metadata or rules tables\n"
            );
        free_args(&args);
        return 1;
    }

    if (args.key && !args.want_metadata && !args.want_rules) {
        fprintf(
                stderr,
                "invalid entry, key option must be used with either the metadata or rules tables\n"
            );
        free_args(&args);
        return 1;
    }

    /* open the save file */
    sqlite3 * db;
    if (sqlite3_open(args.database_name, &db)) {
        fprintf(
                stderr,
                "error opening database \"%s\": %s\n",
                args.database_name,
                sqlite3_errmsg(db)
            );
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    bool need_space = false;
    size_t wants = 0;
    if (args.want_metadata) wants++;
    if (args.want_rules) wants++;
    if (args.want_players) wants++;
    if (args.want_log) wants++;
    if (args.want_cards) wants++;

    size_t extract = 0;
    if (args.bundle_name) extract++;
    if (args.json_file) extract++;

    size_t validate = 0;
    if (args.validate) validate++;

    /* prints all tables if no options are given */
    if (wants == 0 && extract == 0 && validate == 0) {
        args.want_metadata = true;
        args.want_rules = true;
        args.want_players = true;
        args.want_log = true;
        args.want_cards = true;
        wants = 5;
    }

    if (args.want_metadata) {
        if (wants > 1) {
            printf("METADATA\n");
            need_space = true;
        }
        if (print_metadata(db, &args)) {
            free_args(&args);
            return 1;
        }
    }

    if (args.want_rules) {
        if (need_space) {
            printf("\n");
        }
        if (wants > 1) {
            printf("RULES\n");
            need_space = true;
        }
        if(print_rules(db, &args)) {
            free_args(&args);
            return 1;
        }
    }

    if (args.want_players) {
        if (need_space) {
            printf("\n");
        }
        if (wants > 1) {
            printf("PLAYERS\n");
            need_space = true;
        }
        if (print_players(db, &args)) {
            free_args(&args);
            return 1;
        }
    }

    if (args.want_log) {
        if (need_space) {
            printf("\n");
        }
        if (wants > 1) {
            printf("LOG\n");
            need_space = true;
        }
        if (print_log(db, &args)) {
            free_args(&args);
            return 1;
        }
    }

    if (args.want_cards) {
        if (need_space) {
            printf("\n");
        }
        if (wants > 1) {
            printf("CARDS\n");
            need_space = true;
        }
        if (print_cards(db, &args)) {
            free_args(&args);
            return 1;
        }
    }

    if (args.validate) {
        validate_db(db);
    }

    if (args.bundle_name) {
        extract_bundle(db, args.bundle_name);
    }

    if (args.json_file) {
        extract_json(db, args.json_file);
    }

    sqlite3_close(db);
    free_args(&args);
    return 0;
}
