/* File: src/tools/saves_create/saves_create.c
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
#include <errno.h>
#include <string.h>

#include <sqlite3.h>

#include "tools/saves_create/args.h"

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
        sqlite3 * db, ssize_t id, ssize_t player, const char * command)
{
    const char statement[] =
        "INSERT INTO commands (id, player, command) VALUES (?, ?, ?)";
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

    if (sqlite3_bind_int64(stmt, 2, player)) {
        fprintf(
                stderr,
                "error binding statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }


    if (sqlite3_bind_text(stmt, 3, command, strlen(command), NULL)) {
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

static void free_args(struct arguments * args)
{
    free(args->database_name);
}

int main(int argc, char ** argv)
{
    struct arguments args = (struct arguments) { };

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
                "error opening database \"%s\": %s\n",
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
                "DROP TABLE IF EXISTS commands",
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
                "CREATE TABLE IF NOT EXISTS commands (id, player, command)",
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

    if (add_metadata(db, "generator", "cards_create" VERSION)) {
        free_args(&args);
        return 1;
    }

    if (add_player(db, 0, "tina")) {
        free_args(&args);
        return 1;
    }

    if (add_player(db, 1, "ben")) {
        free_args(&args);
        return 1;
    }

    if (add_command(db, 0, 0, "PLAY \"horse\" ON ZONE 1")) {
        free_args(&args);
        return 1;
    }

    if (add_command(db, 1, 0, "END TURN")) {
        free_args(&args);
        return 1;
    }

    if (add_command(db, 2, 1, "DISCARD CARD 362")) {
        free_args(&args);
        return 1;
    }

    (void)add_rule;

    sqlite3_close(db);
    free_args(&args);
    return 0;
}

